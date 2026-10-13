#pragma once

#include <vector>
#include <string_view>


enum class TokenType
{
    Mnemonic,
    Label,
    Register,
    HexLiteral,
    BinaryLiteral,
    Identifier,
    Unknown
};

struct Token
{
    TokenType type;
    std::string_view value;
    size_t line;
};

std::vector<Token> lex(std::string_view src);
