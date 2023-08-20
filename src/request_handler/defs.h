#pragma once

#include <string_view>
#include <string>

namespace defs {
using namespace std::literals;
enum class TypeRequest {
    None,
    StaticFiles,
    Maps,
    Map,
    Join,
    Players,
    State,
    Action,
    BadVersion,
    Tick
};

struct ServerMessage
{
    static inline constexpr std::string_view START = "Server has started..."sv;
    static inline constexpr std::string_view EXIT = "server exited"sv;
};

struct ServerAction
{
    static inline constexpr std::string_view READ = "read"sv;
    static inline constexpr std::string_view WRITE = "write"sv;
    static inline constexpr std::string_view ACCEPT = "accept"sv;
};

struct JsonField
{
    static inline const std::string CODE = "code"s;
    static inline const std::string MESSAGE = "message"s;
};

struct ServerParam
{
    static inline constexpr std::string_view ADDR = "0.0.0.0"sv;
    static inline const uint32_t PORT = 8080;
};

struct ErrorCode
{
    static inline constexpr std::string_view BAD_REQUEST = "badRequest"sv;
    static inline constexpr std::string_view INVALID_METHOD = "invalidMethod"sv;
    static inline constexpr std::string_view INVALID_ARGUMENT = "invalidArgument"sv;
    static inline constexpr std::string_view INVALID_TOKEN = "invalidToken"sv;
    static inline constexpr std::string_view UNKNOWN_TOKEN = "unknownToken"sv;
    static inline constexpr std::string_view MAP_NOT_FOUND = "mapNotFound"sv;
};

struct ErrorMessage
{
    static inline constexpr std::string_view BAD_REQUEST = "Bad request"sv;
    static inline constexpr std::string_view INVALID_ENDPOINT = "Invalid endpoint"sv;
    static inline constexpr std::string_view POST_IS_EXPECTED = "Only POST method is expected"sv;
    static inline constexpr std::string_view GET_IS_EXPECTED = "Only GET method is expected"sv;
    static inline constexpr std::string_view INVALID_TOKEN = "Authorization header is missing"sv;
    static inline constexpr std::string_view UNKNOWN_TOKEN = "Player token has not been found"sv;
    static inline constexpr std::string_view MAP_NOT_FOUND = "Map not found"sv;
    static inline constexpr std::string_view JOIN_GAME_PARSE = "Join game request parse error"sv; 
    static inline constexpr std::string_view INVALID_NAME = "Invalid name"sv;
    static inline constexpr std::string_view FAIL_PARSE_ACTION = "Failed to parse action"sv;
    static inline constexpr std::string_view FAIL_PARSE_TICK_JSON = "Failed to parse tick request JSON"sv;
};

struct ThrowMessage {
    static inline const char* ERROR_CONVERT_JSON = "Error convert json body to json value\0";
    static inline const char* NOT_UINT64 = "not an uint64_t\0";
    static inline const char* TIME_NOT_ZERO = "The time should not be zero\0";
};

struct MiscDefs
{
    static inline constexpr std::string_view NO_CACHE = "no-cache"sv;
};

struct MiscMessage
{
    static inline constexpr std::string_view ALLOWED_POST_METHOD = "POST"sv;
    static inline constexpr std::string_view ALLOWED_GET_HEAD_METHOD = "GET, HEAD"sv;
};

struct Endpoint
{
    static inline constexpr std::string_view API            = "/api/"sv;
    static inline constexpr std::string_view CHECK_VERSION  = "/api/v1/"sv;
    static inline constexpr std::string_view MAPS           = "/api/v1/maps"sv;
    static inline constexpr std::string_view GAME           = "/api/v1/game/"sv;
    static inline constexpr std::string_view GAME_TICK      = "/api/v1/game/tick"sv;
    static inline constexpr std::string_view JOIN_GAME      = "/api/v1/game/join"sv;
    static inline constexpr std::string_view PLAYERS_LIST   = "/api/v1/game/players"sv;
    static inline constexpr std::string_view GAME_STATE     = "/api/v1/game/state"sv;
    static inline constexpr std::string_view RECORDS        = "/api/v1/game/records"sv;
    static inline constexpr std::string_view GAME_ACTION    = "/api/v1/game/player/action"sv;
};

struct Param {
    static inline constexpr int START     = 0;
    static inline constexpr int MAX_ITEMS = 100;
};

} // namespace defs
