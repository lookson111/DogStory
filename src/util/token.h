#pragma once

#include <string>
#include <optional>
#include "tagged.h"

namespace security
{
using namespace std::literals;

namespace detail {
    struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

static inline std::string_view GetToken(std::string_view autorization_text) {
    std::string_view bearer = "Bearer "sv;
    std::string_view ex_token = "6516861d89ebfff147bf2eb2b5153ae1"sv;
    std::string_view nullstr = autorization_text.substr(0, 0);
    if (autorization_text.substr(0, bearer.size()) != bearer)
        return nullstr;
    if (autorization_text.size() < (bearer.size() + ex_token.size()))
        return nullstr;
    size_t begin = bearer.size();
    size_t end = autorization_text.size() - 1;
    for (; end > begin && autorization_text[end] == ' '; end--);
    std::string_view out = autorization_text.substr(begin, end);
    if (out.size() != ex_token.size())
        return nullstr;
    return out;
}

static inline std::optional<Token> ExtractTokenFromStringViewAndCheckIt(std::string_view body)
{
    if (body.empty())
        return std::nullopt;
    std::string token{GetToken(body)};
    if (token.empty())
        return std::nullopt;
    return {Token(token)};
}

}
