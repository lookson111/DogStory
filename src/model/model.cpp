#include <stdexcept>
#include <boost/random.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <ctime>
#include <random>

#include "model.h"
#include "collision_detector.h"

#define MAX_ROADS_TO_FOUND 1000

namespace model {
using namespace std::literals;

static constexpr double LOOT_WIDTH = 0.0;
static constexpr double DOG_WIDTH = 0.6/2;
static constexpr double OFFICE_WIDTH = 0.5/2;

void Map::AddOffice(const Office &office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }
    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_mapS
        offices_.pop_back();
        throw std::bad_alloc(); // "failed to allocate memory for Office"
    }
}

void Game::AddMap(const Map &map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw std::bad_alloc(); // "failed to allocate memory for Map"
        }
    }
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::FindGameSession(const Map::Id& id) noexcept {
    if (auto it = map_id_to_game_sessions_index_.find(id);
        it != map_id_to_game_sessions_index_.end()) {
        return &game_sessions_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::AddGameSession(const Map::Id& id) {
    auto map = FindMap(id);
    if (map == nullptr)
        throw std::invalid_argument("Bad id, map not found");
    GameSession gs{ map , randomize_spawn_points_, 
        GetLootGeneratorConfig()};
    const size_t index = game_sessions_.size();
    game_sessions_.emplace_back(std::move(gs));
    try {
        map_id_to_game_sessions_index_.emplace(id, index);
    }
    catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        game_sessions_.pop_back();
        throw std::bad_alloc(); // "failed to allocate memory for GameSession"
    }
    return &game_sessions_.back();
}

Game::GameSessions& Game::GetGameSessions() {
    return game_sessions_;
}

void Game::SetGameSessions(const Game::GameSessions& game_sessions) {
    for (const auto& game_session: game_sessions) {
        auto gs = game_session;
        auto id = game_session.MapId();
        const size_t index = game_sessions_.size();
        game_sessions_.emplace_back(std::move(gs));
        try {
            map_id_to_game_sessions_index_.emplace(id, index);
        }
        catch (...) {
            // Удаляем офис из вектора, если не удалось вставить в unordered_map
            game_sessions_.pop_back();
            throw std::bad_alloc(); // "failed to allocate memory for GameSession"
        }
    }
}

void Game::Tick(milliseconds time_delta_ms) {
    for (auto &game_session : game_sessions_) {
        game_session.Tick(time_delta_ms);
    }
    tick_signal_(time_delta_ms);
}

Dog* GameSession::AddDog(std::string_view nick_name) {
    Point2D coord;
    if (randomize_spawn_points_)
        coord = GetRandomRoadCoord();
    auto dog_id = GetNextDogId();
    Dog dog = Dog(dog_id, nick_name.data(), coord);
    MoveDogToContainer(std::move(dog));
    loots_.push_back(std::move(Loot{
        .id = GetNextLootId(),
        .type = GetRandomInt(0, static_cast<int>(map_->GetLootsParam().size() - 1)),
        .pos = GetRandomRoadCoord() }));
    return &dogs_.at(dog_id);
}

void GameSession::DeleteDog(const Dog::Id& dog_id)
{
    if (dogs_.contains(dog_id)) {
        dogs_.erase(dog_id);
    }
}

void GameSession::MoveDogToContainer(Dog &&dog) {
    try {
        auto id = dog.GetId();
        dogs_.emplace(id, std::move(dog));
    }
    catch (...) {
        throw std::bad_alloc(); //"failed to allocate memory for Dog"
    }
}

void GameSession::SetDogs(const Dogs& dogs) {
    for (auto it = dogs.begin(); it != dogs.end(); ++it) {
        auto [id, dog] = *it;
        MoveDogToContainer(std::move(dog));
    }
}

const GameSession::Dogs& GameSession::GetDogs() const {
    return dogs_;
}

void GameSession::SetLoots(const Loots& loots) {
    loots_ = loots;
}

const Loots& GameSession::GetLoots() const {
    return loots_;
}
/*
Dog* GameSession::FindDog(std::string_view nick_name) {
    for (auto &[dog_id, dog] : dogs_) {
        if (dog.GetName() == nick_name)
            return &dog;
    }
    return nullptr;
}*/

Dog* GameSession::FindDog(const Dog::Id& dog_id)
{
    if (dogs_.contains(dog_id)) {
        return &dogs_.at(dog_id);
    }
    return {};
}

double GameSession::GetRandomDouble(double min, double max) {
    std::time_t now = std::time(nullptr);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    auto random_double = [&](auto x1, auto x2) {
        if (x2 < x1)
            std::swap(x1, x2);
        std::uniform_real_distribution<double> ur(x1, x2);
        return ur(gen);
    };
    return random_double(min, max);
}
LostObjectType GameSession::GetRandomInt(int min, int max) {
    std::time_t now = std::time(nullptr);
    boost::random::mt19937 gen{static_cast<std::uint32_t>(now)};
    boost::random::uniform_int_distribution<int> dist{min, max};
    return dist(gen);
}

Point2D GameSession::GetRandomRoadCoord() {
    auto &roads = map_->GetRoads();
    if (roads.size() == 0)
        return Point2D();
    auto &road = roads[static_cast<size_t>(GetRandomInt(0, 
        static_cast<int>(roads.size() - 1)))];
    Point2D coord;
    if (road.IsHorizontal()) {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();
        coord.x = GetRandomDouble(start_point.x, end_point.x);
        coord.y = end_point.y;
    }
    else {
        auto start_point = road.GetStart();
        auto end_point = road.GetEnd();
        coord.y = GetRandomDouble(start_point.y, end_point.y);
        coord.x = end_point.x;
    }
    return coord;
}

void GameSession::LoadRoadMap() {
    const auto &roads = map_->GetRoads();
    for (const auto& road : roads) {
        auto start = road.GetStart();
        auto end = road.GetEnd();
        if (road.IsHorizontal()) {
            if (start.x > end.x)
                std::swap(start, end);
            for (auto x = start.x; x <= end.x; x++) {
                road_map.emplace(Point{.x = x, .y = start.y}, &road);
            }
        }
        if (road.IsVertical()) {
            if (start.y > end.y)
                std::swap(start, end);
            for (auto y = start.y; y <= end.y; y++) {
                road_map.emplace(Point{ .x = start.x, .y = y }, &road);
            }
        }
    }
}

void GameSession::MoveDog(const Dog::Id& id, Move move) {
    if (auto it_dog = dogs_.find(id); it_dog != dogs_.end()) {
        it_dog->second.Diraction(move, map_->GetDogSpeed());
    }
}

Point2D GameSession::MoveDog(Point2D start_pos, Point2D end_pos) {
    auto pos_round = [](auto &pos) {
        Point point_pos{ .x = static_cast<Coord>(std::round(pos.x)),
            .y = static_cast<Coord>(std::round(pos.y))};
        return point_pos;
    };
    if (start_pos == end_pos)
        return start_pos;    
    auto road_offset = map_->GetRoadOffset();
    auto prev_pos = start_pos;
    int protect = 0;
    do {
        Point dog_cell = pos_round(start_pos);
        auto roads = road_map.equal_range(dog_cell);
        if (PosInRoads(roads, end_pos)) {
            return end_pos;
        }
        prev_pos = start_pos;
        start_pos = GetExtremePos(roads, end_pos);
        if (++protect >= MAX_ROADS_TO_FOUND)
            throw std::logic_error("Error, not found end cell in roads"s);
    } while (prev_pos != start_pos);
    return start_pos;
}

void GameSession::MoveDogsInMap(milliseconds time_delta_ms) {
    for (auto& [dog_id, dog] : dogs_) {
        dog.IncLifetime(time_delta_ms);
        if (dog.IsStanding())
            continue;
        auto start_pos = dog.GetPoint();
        auto end_pos = dog.GetEndPoint(time_delta_ms);
        auto move_pos = MoveDog(start_pos, end_pos);
        dog.SetPoint(move_pos);
        // if the dog is on the edge, it is necessery to stop him
        if (move_pos != end_pos)
            dog.Stop();
    }
}

void GameSession::CollectAndReturnLoots() {
    namespace cd = collision_detector;
    cd::ItemGatherer item_gatherer;
    std::map<size_t, model::Dog*> dog_numb_to_gather;
    std::map<size_t, decltype(loots_.begin())> loot_numb_to_item;
    size_t dog_idx = 0;
    size_t loot_idx = 0;
    for (auto& [dog_id, dog] : dogs_) {
        dog_numb_to_gather[dog_idx++] = &dog;
        item_gatherer.Add(cd::Gatherer{ 
            .start_pos = dog.GetPrevPoint(), 
            .end_pos = dog.GetPoint(), .width = DOG_WIDTH});
    }
    for (auto it = loots_.begin(); it != loots_.end(); ++it) {
        loot_numb_to_item[loot_idx++] = it;
        item_gatherer.Add(cd::Item{ .position = it->pos, .width = LOOT_WIDTH });
    }
    for (const auto& office : map_->GetOffices()) {
        auto pos = office.GetPosition();
        Point2D pos2d{ static_cast<Dimension2D>(pos.x), static_cast<Dimension2D>(pos.y) };
        item_gatherer.Add(cd::Item{ .position = pos2d, .width = OFFICE_WIDTH });
    }
    auto gathering_events = cd::FindGatherEvents(item_gatherer);
    size_t sz_loots = loots_.size();
    for (const auto& ge : gathering_events) {
        // if is office
        if (ge.item_id >= sz_loots) {
            // Return all items to the base
            dog_numb_to_gather[ge.gatherer_id]->LootsReturn(map_->GetLootsParam());
            continue;
        }
        // if is loot
        if (loot_numb_to_item.contains(ge.item_id)) {
            // if the bag is full, then we do not collect loot
            if (dog_numb_to_gather[ge.gatherer_id]->GetLoots().size() >= map_->GetBagCapacity())
                continue;
            dog_numb_to_gather[ge.gatherer_id]->PutTheLoot(
                loots_, loot_numb_to_item[ge.item_id]);
            loot_numb_to_item.erase(ge.item_id);
        }
    }
}

void GameSession::PushLootsToMap(milliseconds time_delta_ms) {
    auto cnt_loot = loot_generator_.Generate(time_delta_ms,
        static_cast<int>(loots_.size()), static_cast<int>(dogs_.size()));
    for (unsigned i = 0; i < cnt_loot; i++) {
        loots_.push_back(std::move(Loot{
            .id = GetNextLootId(),
            .type = GetRandomInt(0, static_cast<int>(map_->GetLootsParam().size() - 1)),
            .pos = GetRandomRoadCoord() }));
    }
}

Loot::Id GameSession::GetNextLootId() {
    return Loot::Id{(*loot_id_)++};
}
Dog::Id GameSession::GetNextDogId() {
    return Dog::Id{(*dog_id_)++};
}

const Loot::Id& GameSession::GetLastLootId() const {
    return loot_id_;
}

void GameSession::SetLastLootId(const Loot::Id& loot_id) {
    loot_id_ = loot_id;
}

const Dog::Id& GameSession::GetLastDogId() const {
    return dog_id_;
}

void GameSession::SetLastDogId(const Dog::Id& dog_id) {
    dog_id_ = dog_id;
}

void GameSession::Tick(milliseconds time_delta_ms) {
    MoveDogsInMap(time_delta_ms);
    PushLootsToMap(time_delta_ms);
    CollectAndReturnLoots();

}
//    |  ______________
// y1 | |              |
//    | |              |
//    | |              |
//    | |              |
// y0 | |______________|
//    |____________________
//     x0             x1
//
bool GameSession::PosInRoads(RoadMapIter roads, Point2D pos) {
    for (auto it = roads.first; it != roads.second; ++it) {
        auto &road = *it->second;
        auto [x0, x1, y0, y1] = road.GetRectangle();
        if (pos.x >= x0 && pos.x <= x1 && pos.y >= y0 && pos.y <= y1)
            return true;
    }
    return false;
}
Point2D GameSession::GetExtremePos(RoadMapIter roads, Point2D pos) {
    typedef std::numeric_limits<Dimension2D> dbl; 
    Point2D min;
    Dimension2D min_d = dbl::max(), distance;
    for (auto it = roads.first; it != roads.second; ++it) {
        auto& road = *it->second;
        auto [x0, x1, y0, y1] = road.GetRectangle();
        if (pos.x >= x0 && pos.x <= x1) {
            if (pos.y <= y0) {
                if ((distance = std::abs(y0 - pos.y)) < min_d) {
                    min_d = distance;
                    min.y = y0;
                    min.x = pos.x;
                    continue;
                }
            }
            if (pos.y >= y1) {
                if ((distance = std::abs(y1 - pos.y)) < min_d) {
                    min_d = distance;
                    min.y = y1;
                    min.x = pos.x;
                    continue;
                }
            }
        }
        if (pos.y >= y0 && pos.y <= y1) {
            if (pos.x <= x0) {
                if ((distance = std::abs(x0 - pos.x)) < min_d) {
                    min_d = distance;
                    min.x = x0;
                    min.y = pos.y;
                    continue;
                }   
            }
            if (pos.x >= x1) {
                if ((distance = std::abs(x1 - pos.x)) < min_d) {
                    min_d = distance;
                    min.x = x1;
                    min.y = pos.y;
                    continue;
                }
            }
        }
    }
    return min;
}
}  // namespace model
