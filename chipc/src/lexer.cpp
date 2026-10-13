#include "lexer.hpp"

#include <ctre.hpp>

std::vector<Token> lex(std::string_view src)
{
    std::vector<Token> tokens;
    size_t currentLine = 1;

    while(!src.empty()) {

        // skip whitespace and commas
        if(auto match = ctre::starts_with<"^[ \\t\\r\\n,]+">(src)) {
            auto view = match.to_view();
            currentLine += std::count(view.begin(), view.end(), '\n');
            src.remove_prefix(match.size());
            continue;
        }

        // registers
        if (auto match = ctre::starts_with<"^[vV][0-9a-fA-F]">(src)) {
            tokens.push_back({TokenType::Register, match.to_view(), currentLine});
            src.remove_prefix(match.size());
            continue;
        }

        // hex literals
        if (auto match = ctre::starts_with<"^0[xX][0-9a-fA-F]+">(src)) {
            tokens.push_back({TokenType::HexLiteral, match.to_view(), currentLine});
            src.remove_prefix(match.size());
            continue;
        }

        // binary literals
        if (auto match = ctre::starts_with<"^0[bB][01]+">(src)) {
            tokens.push_back({TokenType::BinaryLiteral, match.to_view(), currentLine});
            src.remove_prefix(match.size());
            continue;
        }

        // labels
        if(auto match = ctre::starts_with<"^[a-zA-Z_][a-zA-Z0-9_]+:">(src)) {
            tokens.push_back({TokenType::Label, match.to_view().substr(0, match.size() - 1), currentLine});
            src.remove_prefix(match.size());
            continue;
        }

        // mnemonics
        if(auto match = ctre::starts_with<"^[a-zA-Z_][a-zA-Z0-9_]*">(src)) {
            tokens.push_back({TokenType::Mnemonic, match.to_view(), currentLine});
            src.remove_prefix(match.size());
            continue;
        }

        // comments
        if(auto match = ctre::starts_with<"^;[^\\n\\r]*">(src)) {
            src.remove_prefix(match.size());
            continue;
        }

        tokens.push_back({TokenType::Unknown, src.substr(0, 1), currentLine});
        src.remove_prefix(1);
    }
    return tokens;
}
