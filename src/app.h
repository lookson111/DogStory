#pragma once
#include "sdk.h"
#include <boost/json.hpp>
#include <boost/asio/strand.hpp>
#include <string>
#include <random>
#include "model/model.h"
#include "util/token.h"
#include "ticker.h"
#include "player.h"
#include "db/postgres.h"

namespace app {
namespace js = boost::json;
namespace net = boost::asio;
namespace sys = boost::system;
using milliseconds = std::chrono::milliseconds;
using Token = security::Token;

enum class JoinError {
    None,
    BadJson,
    MapNotFound,
    InvalidName
};
enum class error_code {
    None,
    InvalidToken,
    UnknownToken,
    InvalidArgument
};

struct AppConfig {
    size_t capacity;
    std::string db_url;
};

class ModelToJson {
public:
    explicit ModelToJson(const model::Game& game)
        : game_{ game } {
    }
    std::string GetMaps() const;
    std::string GetMap(std::string_view nameMap) const;
private:
    const model::Game& game_;
    static js::array GetRoads(const model::Map::Roads& roads);
    static js::array GetBuildings(const model::Map::Buildings& buildings);
    static js::array GetOffice(const model::Map::Offices& offices);
    static js::value ToJsonValue(std::string_view json_body);
};

std::string JsonMessage(std::string_view code, std::string_view message);

class App
{
public:
    explicit App(model::Game& game, const AppConfig& config)
        : game_{ game }
        , db_{ config.capacity, config.db_url } {
    }
    model::Game& GetGameModel();
    const Players& GetPlayers() const;
    const PlayerTokens& GetPlayerTokens() const;
    Players& EditPlayers();
    PlayerTokens& EditPlayerTokens();
    void RetirPlayers(milliseconds delta);

    std::pair<std::string, bool> GetMapBodyJson(std::string_view requestTarget) const;
    std::pair<std::string, JoinError> ResponseJoin(std::string_view jsonBody);
    std::pair<std::string, error_code> ActionMove(
        const Token& token, std::string_view jsonBody);
    std::pair<std::string, error_code> Tick(std::string_view jsonBody);
    std::pair<std::string, error_code> GetPlayers(const Token& token) const;
    std::pair<std::string, error_code> GetState(const Token& token) const;
    std::pair<std::string, error_code> CheckToken(const Token& token) const;
    std::pair<std::string, error_code> GetRecords(unsigned start, unsigned max_items);

private:
    model::Game& game_;
    Players players_;
    PlayerTokens player_tokens_;
    postgres::Database db_;
    Player* GetPlayer(const Token& token) const;
    Player* GetPlayer(std::string_view nick, std::string_view mapId);
    auto GetJsonDogBag(const model::Dog& dog) const;
};
}

