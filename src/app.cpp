#include "app.h"
#include <boost/format.hpp>
#include "log.h"
#include "request_handler/defs.h"

std::string app::JsonMessage(std::string_view code, std::string_view message) {
    js::object msg;
    msg["code"] = code.data();
    msg["message"] = message.data();
    return serialize(msg);
}

namespace app {
using namespace std::literals;
using namespace defs;

static auto to_booststr = [](std::string_view str) {
    return boost::string_view(str.data(), str.size());
};

std::string ModelToJson::GetMaps() const {
    const auto& maps = game_.GetMaps();
    js::array obj;
    for (auto& map : maps) {
        js::object mapEl;
        mapEl["id"] = *map.GetId();
        mapEl["name"] = map.GetName();
        obj.emplace_back(mapEl);
    }
    return serialize(obj);
}

std::string ModelToJson::GetMap(std::string_view nameMap) const {
    model::Map::Id idmap{nameMap.data()};
    js::object mapEl;
    if (auto map = game_.FindMap({ idmap }); map != nullptr) {
        mapEl["id"] = *map->GetId();
        mapEl["name"] = map->GetName();
        mapEl["roads"] = GetRoads(map->GetRoads());
        mapEl["buildings"] = GetBuildings(map->GetBuildings());
        mapEl["offices"]   = GetOffice(map->GetOffices());
        mapEl["lootTypes"] = ToJsonValue(game_.GetLootTypes(map->GetId()));
    }
    return serialize(mapEl);
}

js::array ModelToJson::GetRoads(const model::Map::Roads& roads) {
    js::array arr;
    for (auto& r : roads) {
        js::object road;
        road["x0"] = r.GetStart().x;
        road["y0"] = r.GetStart().y;
        if (r.IsHorizontal()) {
            road["x1"] = r.GetEnd().x;
        }
        else {
            road["y1"] = r.GetEnd().y;
        }
        arr.emplace_back(std::move(road));
    }
    return arr;
}

js::array ModelToJson::GetBuildings(const model::Map::Buildings& buildings) {
    js::array arr;
    for (auto& b : buildings) {
        js::object building;
        building["x"] = b.GetBounds().position.x;
        building["y"] = b.GetBounds().position.y;
        building["w"] = b.GetBounds().size.width;
        building["h"] = b.GetBounds().size.height;
        arr.emplace_back(std::move(building));
    }
    return arr;
}

js::array ModelToJson::GetOffice(const model::Map::Offices& offices) {
    js::array arr;
    for (auto& o : offices) {
        js::object office;
        office["id"] = *o.GetId();
        office["x"] = o.GetPosition().x;
        office["y"] = o.GetPosition().y;
        office["offsetX"] = o.GetOffset().dx;
        office["offsetY"] = o.GetOffset().dy;
        arr.emplace_back(std::move(office));
    }
    return arr;
}

js::value ModelToJson::ToJsonValue(std::string_view json_body)
{
    js::error_code ec;
    js::string_view jb{json_body.data(), json_body.size()};
    js::value const jv = js::parse(jb, ec);
    if (ec)
        throw std::logic_error(ThrowMessage::ERROR_CONVERT_JSON);
    return jv;
}

model::Game& App::GetGameModel() {
    return game_;
}

const Players& App::GetPlayers() const {
    return players_;
}

Players& App::EditPlayers() {
    return players_;
}

PlayerTokens& App::EditPlayerTokens() {
    return player_tokens_;
}

void App::RetirPlayers(milliseconds delta) {
    auto players = players_.GetPlayers();
    std::vector<PlayerId> to_delete;
    for (auto it = players.begin(); it != players.end(); ++it) {
        auto player = *it->second;
        auto dog = *player.GetDog();
        if (dog.GetStayTime() >= game_.GetDogRetirementTime()) {
            RetiredPlayer retired_player{player.GetId(), dog.GetName(), 
                dog.GetScore(), dog.GetLifetime() };
            db_.GetRetiredPlayers().Save(retired_player);
            to_delete.push_back(player.GetId());
        }
    }
    for (auto& id : to_delete) {
        for (auto& session : game_.GetGameSessions()) {
            auto dog_id = players_.FindPlayer(id)->GetDog()->GetId();
            session.DeleteDog(dog_id);
        }
        player_tokens_.DeleteToken(id);
        players_.DeletePlayer(id);
    }
}

const PlayerTokens& App::GetPlayerTokens() const {
    return player_tokens_;
}

std::pair<std::string, bool>
App::GetMapBodyJson(std::string_view mapName) const {
    ModelToJson jmodel(game_);
    std::string body;
    if (mapName.empty()) {
        body = jmodel.GetMaps();
    }
    else {
        // if map not found
        body = jmodel.GetMap(mapName);
        if (body.size() == 2) { 
            // if body is "" (empty)
            return std::make_pair(JsonMessage(ErrorCode::MAP_NOT_FOUND, 
                ErrorMessage::MAP_NOT_FOUND), false);
        }
    }
    return std::make_pair(std::move(body), true);
}
//
std::pair<std::string, JoinError>
App::ResponseJoin(std::string_view jsonBody) {
    auto parseError = std::make_pair(
        JsonMessage(ErrorCode::INVALID_ARGUMENT, ErrorMessage::JOIN_GAME_PARSE),
        JoinError::BadJson
    );
    js::error_code ec;
    js::string_view jb{jsonBody.data(), jsonBody.size()};
    std::string resp;
    std::string userName;
    std::string mapId;
    js::value const jv = js::parse(jb, ec);
    if (ec)
        return parseError;
    try {
        userName = jv.at("userName").as_string();
        mapId = jv.at("mapId").as_string();
    }
    catch (const std::system_error&) {
        return parseError;
    }
    catch (const std::out_of_range&) {
        return parseError;
    }
    if (userName.empty())
        return std::make_pair(
            JsonMessage(ErrorCode::INVALID_ARGUMENT, ErrorMessage::INVALID_NAME),
            JoinError::InvalidName
        );
    model::Map::Id idmap{mapId.data()};
    auto map = game_.FindMap({ idmap });
    if (map == nullptr)
        return std::make_pair(
            JsonMessage(ErrorCode::MAP_NOT_FOUND, ErrorMessage::MAP_NOT_FOUND),
            JoinError::MapNotFound
        );
    js::object msg;
    auto player = GetPlayer(userName, mapId);
    std::string token = *player_tokens_.AddPlayer(player);
    auto id = *player->GetDog()->GetId();
    msg["authToken"] = token;
    msg["playerId"]  = id;
    return std::make_pair(
        std::move(serialize(msg)),
        JoinError::None
    );
}

std::pair<std::string, error_code>
App::ActionMove(const Token& token, std::string_view jsonBody) {
    std::string move;
    try {
        js::value const jv = js::parse(to_booststr(jsonBody));
        move = jv.at("move").as_string();
    }
    catch (...) {
        return std::make_pair(
            JsonMessage(ErrorCode::INVALID_ARGUMENT, ErrorMessage::FAIL_PARSE_ACTION),
            error_code::InvalidArgument
        );
    }    
    Player* player = player_tokens_.FindPlayer(token);
    player->Move(move);
    js::object msg;
    return std::make_pair(
        std::move(serialize(msg)),
        error_code::None        
    );
}

std::pair<std::string, error_code> 
App::Tick(std::string_view jsonBody) {
    milliseconds time_delta_mc;
    try {
        auto jv = js::parse(to_booststr(jsonBody));
        auto jv_time_delta = jv.at("timeDelta");
        uint64_t mc;
        if (const auto* n = jv_time_delta.if_int64(); n && *n > 0) { // [ 1 .. 2^63 - 1 ]
            mc = *n;
        } else if (const auto* n = jv_time_delta.if_uint64()) { // [ 2^63 .. 2^64 - 1 ]
            mc = *n;
        } else {
            throw std::out_of_range{ThrowMessage::NOT_UINT64};
        }
        if (mc == 0)
            throw std::out_of_range{ThrowMessage::TIME_NOT_ZERO};
        time_delta_mc = milliseconds(mc);
    }
    catch (...) {
        return std::make_pair(
            JsonMessage(ErrorCode::INVALID_ARGUMENT, 
                ErrorMessage::FAIL_PARSE_TICK_JSON),
            error_code::InvalidArgument
        );
    }    
    game_.Tick(time_delta_mc);
    js::object msg;
    return std::make_pair(
        std::move(serialize(msg)),
        error_code::None        
    );
}

std::pair<std::string, error_code> 
App::GetPlayers(const Token& token) const {
    js::object msg;
    Player* player = player_tokens_.FindPlayer(token);
    auto session = player->GetSession();
    const auto &dogs = session->GetDogs();
    for (const auto& [dog_id, dog] : dogs) {
        js::object jname;
        jname["name"] = to_booststr(dog.GetName());
        msg[std::to_string(*dog.GetId())] = jname;
    }
    return std::make_pair(
        std::move(serialize(msg)),
        error_code::None
    );
}

auto App::GetJsonDogBag(const model::Dog &dog) const {
    js::array jarr;
    for (const auto& loot : dog.GetLoots()) {
        js::object val;
        val["id"] = *loot.id;
        val["type"] = loot.type;
        jarr.emplace_back(std::move(val));
    }
    return jarr;
}

std::pair<std::string, error_code> 
App::GetState(const Token& token) const {
    auto put_array = [](const auto &x, const auto &y) {
        js::array jarr;
        jarr.emplace_back(x);
        jarr.emplace_back(y);
        return jarr;
    };
    Player* player = player_tokens_.FindPlayer(token);
    js::object state;
    auto session = player->GetSession();
    const auto &dogs = session->GetDogs();
    for (const auto &[dog_id, dog] : dogs) {
        js::object dog_param;
        dog_param["pos"] = put_array(dog.GetPoint().x, dog.GetPoint().y);
        dog_param["speed"] = put_array(dog.GetSpeed().x, dog.GetSpeed().y);
        dog_param["dir"] = dog.GetDirection();
        dog_param["bag"] = GetJsonDogBag(dog);
        dog_param["score"] = dog.GetScore();
        state[std::to_string(*dog.GetId())] = dog_param;
    }
    js::object players;
    players["players"] = state;
    js::object js_loot_type;
    const auto& loots = session->GetLoots();
    for (const auto& loot : loots) {
        js::object object;
        object["type"] = loot.type; 
        object["pos"] = put_array(loot.pos.x, loot.pos.y);
        js_loot_type[std::to_string(*loot.id)] = object;
    }
    players["lostObjects"] = js_loot_type;
    return std::make_pair(
        std::move(serialize(players)),
        error_code::None
    );
}

std::pair<std::string, error_code> 
App::CheckToken(const Token& token) const {
    Player* player = player_tokens_.FindPlayer(token);
    if (player == nullptr)
        return std::make_pair(
            std::move(app::JsonMessage(ErrorCode::UNKNOWN_TOKEN, 
                ErrorMessage::UNKNOWN_TOKEN)),
            error_code::UnknownToken
        );
    return std::make_pair(
        "",
        error_code::None
    );
}

std::pair<std::string, error_code> 
App::GetRecords(unsigned start, unsigned max_items) {
    static const double ms_to_seconds = 1000;
    RetiredPlayers retired_players;
    try {
        retired_players = db_.GetRetiredPlayers().Get(start, max_items);
    } catch (std::exception&){
        return std::make_pair("", error_code::InvalidArgument);
    }
    js::array jarr;
    for (const auto& retired_player : retired_players) {
        js::object val;
        val["name"]     = retired_player.GetName();
        val["score"]    = retired_player.GetScore();
        auto play_time = static_cast<double>(retired_player.GetPlayTime().count());
        val["playTime"] = play_time / ms_to_seconds;
        jarr.emplace_back(std::move(val));
    }
    return std::make_pair(
        std::move(serialize(jarr)),
        error_code::None
    );
}


Player* App::GetPlayer(const Token& token) const {
    Player* player = player_tokens_.FindPlayer(token);
    return player;
}

Player* App::GetPlayer(std::string_view nickName, std::string_view mapId) {
    model::Map::Id map_id{mapId.data()};
    auto session = game_.FindGameSession(map_id);
    if (session == nullptr)
        session = game_.AddGameSession(map_id);
    model::Dog* dog = session->AddDog(nickName);
    auto player = players_.Add(app::PlayerId::New(), dog, session);
    return player;
}

};
