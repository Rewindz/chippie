#pragma once

#include <vector>
#include <string_view>
#include <string>
#include <charconv>
#include <stdexcept>
#include <utility>
#include <format>
#include <unordered_map>
#include <cctype>
#include <algorithm>

#include "lexer.hpp"

class Parser
{
    public:

        Parser(std::vector<Token> tokens)
            : m_tokens(std::move(tokens))
        {}

        std::vector<uint8_t> parse()
        {
            passOne();
            std::vector<uint8_t> rom;

            auto pushOpcode = [&rom](uint16_t opcode) {
                rom.push_back(static_cast<uint8_t>((opcode >> 8) & 0xFF));
                rom.push_back(static_cast<uint8_t>(opcode & 0xFF));
            };

            if(!m_labels.contains("_start"))
                throw std::runtime_error("Missing '_start' label for entry point");

            uint16_t entryAddr = m_labels["_start"];
            pushOpcode(0x1000 | (entryAddr & 0x0FFF));

            while(!isAtEnd()) {
                if(peekType<TokenType::Label>()) {
                    consume(TokenType::Label, "");
                    continue;
                }
                Token t = consume(TokenType::Mnemonic, "Expected instruction");

                auto teq = [&t, this](std::string_view a) -> bool {
                    return this->iequal(t.value, a);
                };

                if(teq("LD")) {
                    if(peekType<TokenType::Mnemonic>()) {
                        Token dest = consume(TokenType::Mnemonic, "");
                        auto deq = [&dest, this](std::string_view a) {
                            return this->iequal(dest.value, a);
                        };

                        if(deq("I")) {
                            if(peekType<TokenType::Register>()) {
                                auto rx = parseRegister();
                                pushOpcode(0xF055 | ((rx << 8) & 0x0F00));
                            } else {
                                auto addr = parseAddress();
                                pushOpcode(0xA000 | (addr & 0x0FFF));
                            }
                        } else if(deq("DT")) {
                            auto rx = parseRegister();
                            pushOpcode(0xF015 | ((rx << 8) & 0x0F00));
                        } else if(deq("ST")) {
                            auto rx = parseRegister();
                            pushOpcode(0xF018 | ((rx << 8) & 0x0F00));
                        } else if(deq("F")) {
                            auto rx = parseRegister();
                            pushOpcode(0xF029 | ((rx << 8) & 0x0F00));
                        } else if(deq("B")) {
                            auto rx = parseRegister();
                            pushOpcode(0xF033 | ((rx << 8) & 0x0F00));
                        } else {
                            throw std::runtime_error(std::format("Invalid LD destination: {}", dest.value));
                        }
                    } else if (peekType<TokenType::Register>()) {
                        auto rx = parseRegister();
                        if(peekType<TokenType::HexLiteral, TokenType::BinaryLiteral>()) {
                            auto val = parseByteLiteral();
                            pushOpcode(0x6000 | ((rx << 8) & 0x0F00) | (val & 0x00FF));
                        } else if(peekType<TokenType::Register>()) {
                            auto ry = parseRegister();
                            pushOpcode(0x8000 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0));
                        } else if(peekType<TokenType::Mnemonic>()) {
                            Token src = consume(TokenType::Mnemonic, "");
                            auto seq = [&src, this](std::string_view a){
                                return this->iequal(src.value, a);
                            };

                            if(seq("DT"))
                                pushOpcode(0xF007 | ((rx << 8) & 0x0F00));
                            else if (seq("K"))
                                pushOpcode(0xF00A | ((rx << 8) & 0x0F00));
                            else if (seq("I"))
                                pushOpcode(0xF065 | ((rx << 8) & 0x0F00));
                            else throw std::runtime_error(std::format("Invalid LD source: {}", src.value));
                        }
                    }
                } else if(teq("JP")) {
                    if(peekType<TokenType::Register>()) {
                        auto rx = parseRegister();
                        if(rx != 0)
                            throw std::runtime_error("JP can only use V0 as the offset register");
                        auto addr = parseAddress();
                        pushOpcode(0xB000 | (addr & 0x0FFF));
                    } else {
                        auto addr = parseAddress();
                        pushOpcode(0x1000 | (addr & 0x0FFF));
                    }
                } else if(teq("DB")) {
                    rom.push_back(parseByteLiteral());
                    while(!isAtEnd() &&
                        (peekType<TokenType::HexLiteral, TokenType::BinaryLiteral>()))
                    {
                        rom.push_back(parseByteLiteral());
                    }
                } else if(teq("CLS")) {
                    pushOpcode(0x00E0);
                } else if(teq("RET")) {
                    pushOpcode(0x00EE);
                } else if(teq("CALL")) {
                    auto addr = parseAddress();
                    pushOpcode(0x2000 | (addr & 0x0FFF));
                } else if(teq("SE") || teq("SNE")) {
                    auto rx = parseRegister();
                    if(peekType<TokenType::Register>()) {
                        auto ry = parseRegister();
                        pushOpcode( (teq("SE") ? 0x5000 : 0x9000)
                            | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0));
                    } else if(peekType<TokenType::HexLiteral, TokenType::BinaryLiteral>()) {
                        auto byte = parseByteLiteral();
                        pushOpcode( (teq("SE") ? 0x3000 : 0x4000)
                            | ((rx << 8) & 0x0F00) | (byte & 0x00FF));
                    } else {
                        throw std::runtime_error(std::format("Invalid operand for {} instruction", t.value));
                    }
                } else if(teq("ADD")) {
                    if(peekType<TokenType::Mnemonic>() && peekI()) {
                        consume(TokenType::Mnemonic, "");
                        auto rx = parseRegister();
                        pushOpcode(0xF01E | ((rx << 8) & 0x0F00));
                    } else {
                        auto rx = parseRegister();
                        if(peekType<TokenType::Register>()) {
                            auto ry = parseRegister();
                            pushOpcode(0x8004 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0));
                        } else if(peekType<TokenType::HexLiteral, TokenType::BinaryLiteral>()) {
                            auto kk = parseByteLiteral();
                            pushOpcode(0x7000 | ((rx << 8) & 0x0F00) | (kk & 0x00FF));
                        } else {
                            throw std::runtime_error("Invalid operand for AND instruction");
                        }
                    }
                } else if(teq("SUB") || teq("SUBN")) {
                    auto rx = parseRegister();
                    auto ry = parseRegister();
                    pushOpcode(0x8000 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0) |
                        (teq("SUB") ? 0x5 : 0x7) );
                } else if(teq("OR")) {
                    auto rx = parseRegister();
                    auto ry = parseRegister();
                    pushOpcode(0x8001 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0));
                } else if(teq("AND")) {
                    auto rx = parseRegister();
                    auto ry = parseRegister();
                    pushOpcode(0x8002 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0));
                } else if(teq("XOR")) {
                    auto rx = parseRegister();
                    auto ry = parseRegister();
                    pushOpcode(0x8003 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0));
                } else if(teq("SHR") || teq("SHL")) {
                    auto rx = parseRegister();
                    auto ry = rx;
                    if(peekType<TokenType::Register>()) {
                        ry = parseRegister();
                    }
                    pushOpcode(0x8000 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0) |
                        (teq("SHR") ? 0x6 : 0xE) );
                } else if(teq("RND")) {
                    auto rx = parseRegister();
                    auto kk = parseByteLiteral();
                    pushOpcode(0xC000 | ((rx << 8) & 0x0F00) | (kk & 0x00FF));
                } else if(teq("SKP") || teq("SKNP")) {
                    auto rx = parseRegister();
                    pushOpcode(0xE000 | ((rx << 8) & 0x0F00) |
                        (teq("SKP") ? 0x9E : 0xA1) );
                } else if(teq("DRW")) {
                    auto rx = parseRegister();
                    auto ry = parseRegister();
                    auto n = parseByteLiteral() & 0xF;
                    pushOpcode(0xD000 | ((rx << 8) & 0x0F00) | ((ry << 4) & 0x00F0) | n);
                }
                else {
                    throw std::runtime_error(std::format("Unknown instruction: {}", t.value));
                }
            }
            return rom;
        }

    private:
        std::vector<Token> m_tokens;
        std::unordered_map<std::string_view, uint16_t> m_labels;
        size_t m_pos{0};

        const Token& peek() const { return m_tokens[m_pos]; }

        bool peekI() const
        {
            if(isAtEnd()) return false;
            return iequal(peek().value, "I");
        }

        template<TokenType... Types>
        bool peekType() const
        {
            if(isAtEnd()) return false;
            return (... || (peek().type == Types));
        }

        auto iequal (std::string_view a, std::string_view b) const -> bool
        {
            return std::ranges::equal(a, b, [](char c1, char c2) {
                return std::toupper(static_cast<unsigned char>(c1)) ==
                        std::toupper(static_cast<unsigned char>(c2));
            });
        }

        bool isAtEnd() const { return m_pos >= m_tokens.size(); }

        Token consume(TokenType expected, std::string_view errMsg)
        {
            if (isAtEnd() || m_tokens[m_pos].type != expected) {
                throw std::runtime_error(std::string(errMsg));
            }
            return m_tokens[m_pos++];
        }

        uint8_t parseRegister()
        {
            Token t = consume(TokenType::Register, "Expected register (e.g., V0)");
            char c = t.value[1];
            if(c >= 'a' && c <= 'f') return c - 'a' + 10;
            if(c >= 'A' && c <= 'F') return c - 'A' + 10;
            return c - '0';
        }

        uint16_t parseAddress()
        {
            if (peekType<TokenType::HexLiteral>()) {
                return parseHex();
            }
            Token t = consume(TokenType::Mnemonic, "Expected hex address or label");
            if(m_labels.contains(t.value)) {
                return m_labels[t.value];
            }
            throw std::runtime_error(std::format("Undefined label: {}", t.value));
        }

        uint16_t parseHex()
        {
            Token t = consume(TokenType::HexLiteral, "Expected hex literal (e.g., 0xFF)");
            uint16_t result = 0;

            auto [ptr, ec] = std::from_chars(t.value.data() + 2, t.value.data() + t.value.size(), result, 16);

            if(ec != std::errc()) {
                throw std::runtime_error("Invalid hex literal");
            }
            return result;
        }

        uint8_t parseByteLiteral()
        {
            if(peekType<TokenType::HexLiteral>()) {
                return static_cast<uint8_t>(parseHex());
            }

            Token t = consume(TokenType::BinaryLiteral, "Expected hex or binary literal");
            uint8_t result = 0;
            auto [ptr, ec] = std::from_chars(t.value.data() + 2, t.value.data() + t.value.size(), result, 2);
            if(ec != std::errc())
                throw std::runtime_error("Invalid binary literal");
            return result;
        }

        void passOne()
        {
            uint16_t address = 0x202;
            size_t savedPos = m_pos;
            m_pos = 0;

            auto isInstruction = [this](std::string_view val) {
                constexpr std::string_view instrs[] = {
                    "LD", "JP", "DB", "CLS", "RET", "CALL", "SE", "SNE",
                    "ADD", "SUB", "SUBN", "OR", "AND", "XOR", "SHR", "SHL",
                    "RND", "SKP", "SKNP", "DRW"
                };
                for(const auto& inst : instrs) {
                    if (this->iequal(val, inst)) return true;
                }
                return false;
            };

            while(!isAtEnd()) {
                Token t = m_tokens[m_pos++];
                if(t.type == TokenType::Label) {
                    m_labels[t.value] = address;
                } else if (t.type == TokenType::Mnemonic) {
                    if(isInstruction(t.value)) {
                        if (iequal(t.value, "DB")) {
                            while(!isAtEnd() &&
                                (peekType<TokenType::HexLiteral, TokenType::BinaryLiteral>()))
                            {
                                address += 1;
                                m_pos++;
                            }
                        } else {
                            address += 2;
                        }
                    }
                }
            }
            m_pos = savedPos;
        }

};
