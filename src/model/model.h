#pragma once 
#include "../sdk.h"
#include <boost/signals2.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <atomic>
#include <map>
#include <memory>
#include <list>
#include "dog.h"
#include "extra_data.h"
#include "loot_generator.h"

namespace model {

namespace sig = boost::signals2;
using milliseconds = std::chrono::milliseconds;

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class RoadRectangle {
    Dimension2D x0, x1, y0, y1;
public:
    RoadRectangle(Point start, Point end, Dimension2D road_offset) noexcept {
        x0 = static_cast<Dimension2D>(std::min(start.x, end.x)) -
            road_offset;
        x1 = static_cast<Dimension2D>(std::max(start.x, end.x)) +
            road_offset;
        y0 = static_cast<Dimension2D>(std::min(start.y, end.y)) -
            road_offset;
        y1 = static_cast<Dimension2D>(std::max(start.y, end.y)) +
            road_offset;
    }
    auto Get() const noexcept {
        return std::make_tuple(x0, x1, y0, y1);
    }
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x, Dimension2D offset) noexcept
        : start_{start}
        , end_{end_x, start.y}
        , road_rectangle_{start_, end_, offset} {
    }

    Road(VerticalTag, Point start, Coord end_y, Dimension2D offset) noexcept
        : start_{start}
        , end_{start.x, end_y} 
        , road_rectangle_{start_, end_, offset} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }
    auto GetRectangle() const noexcept {
        return road_rectangle_.Get();
    }
private:
    Point start_;
    Point end_;
    RoadRectangle road_rectangle_;
};



class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};


struct DefaultMapParam {
    Dimension2D dog_speed;
    size_t bag_capacity;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, 
        const DefaultMapParam &default_param) noexcept
        : id_(std::move(id)), name_(std::move(name))
        , default_param_(default_param) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    const auto& GetLootsParam() const noexcept {
        return loots_param_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(const Office &office);

    void AddLoot(LootParam loot_param) {
        loots_param_.push_back(loot_param);
    }

    Dimension2D GetDogSpeed() const {
        return default_param_.dog_speed;
    }
    Dimension2D GetRoadOffset() const {
        return road_offset_;
    }
    auto GetBagCapacity() const {
        return default_param_.bag_capacity;
    }
private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    Dimension2D road_offset_ = 0.4;
    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    DefaultMapParam default_param_; 
    LootsParam loots_param_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

};

struct LootGeneratorConfig {
    milliseconds period{0};
    double probability = 500.0;
};

struct PointKeyHash {
    std::size_t operator()(const Point& k) const {
        return std::hash<Coord>()(k.x) ^ (std::hash<Coord>()(k.y) << 1);
    }
};
struct PointKeyEqual {
    bool operator()(const Point& lhs, const Point& rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

class GameSession {
private:
    using RoadMap = std::unordered_multimap<Point, const Road*, PointKeyHash, PointKeyEqual>;
    using RoadMapIter = decltype(RoadMap{}.equal_range(Point{}));
    using DogsIdHasher = util::TaggedHasher<Dog::Id>;
public:
    using Dogs = std::unordered_map<Dog::Id, Dog, DogsIdHasher>;

    GameSession(const Map* map, bool randomize_spawn_points,
        const LootGeneratorConfig loot_generator_config)
        : map_(map)
        , randomize_spawn_points_(randomize_spawn_points)
        , loot_generator_config_(loot_generator_config)
        , loot_generator_(loot_generator_config.period, 
            loot_generator_config.probability,
            [&]() {return GetRandomDouble(0.0, 1.0);}) {
        LoadRoadMap();
    }
    const Map::Id& MapId() const {
        return map_->GetId();
    } 
   // Dog* FindDog(std::string_view nick_name);
    Dog* FindDog(const Dog::Id& dog_id);
    Dog* AddDog(std::string_view nick_name);
    void DeleteDog(const Dog::Id& dog_id);
    void SetDogs(const Dogs& dogs);
    const Dogs& GetDogs() const;
    void SetLoots(const Loots& loots) ;
    const Loots& GetLoots() const;
    void MoveDog(const Dog::Id& dog_id, Move move);
    void Tick(milliseconds time_delta_ms);
    const Loot::Id& GetLastLootId() const;
    void SetLastLootId(const Loot::Id& loot_id);
    const Dog::Id& GetLastDogId() const;
    void SetLastDogId(const Dog::Id& dog_id);
    bool RandomizeSpawnPoints() const {
        return randomize_spawn_points_;
    }
    GameSession(const GameSession& other) {
        loot_id_ = other.loot_id_;
        dog_id_ = other.dog_id_;
        dogs_ = other.dogs_;
        loots_ = other.loots_;
        map_ = other.map_;
        randomize_spawn_points_ = other.randomize_spawn_points_;
        road_map = other.road_map;
        loot_generator_config_ = other.loot_generator_config_;
        loot_generator_ = loot_gen::LootGenerator{ loot_generator_config_.period,
            loot_generator_config_.probability,
            [&]() {return GetRandomDouble(0.0, 1.0); } };
    }

private:
    Loot::Id loot_id_{ 0 };
    Dog::Id dog_id_{ 0 };
    Dogs dogs_;
    Loots loots_;
    const Map* map_;
    bool randomize_spawn_points_ = true;
    RoadMap road_map;
    LootGeneratorConfig loot_generator_config_;
    loot_gen::LootGenerator loot_generator_{loot_gen::LootGenerator::TimeInterval{0}, 0.0};

    Point2D GetRandomRoadCoord();
    void LoadRoadMap();
    bool PosInRoads(RoadMapIter roads, Point2D pos);
    Point2D GetExtremePos(RoadMapIter roads, Point2D pos);
    Point2D MoveDog(Point2D start_pos, Point2D end_pos);
    static double GetRandomDouble(double min, double max);
    static LostObjectType GetRandomInt(int min, int max);
    void MoveDogsInMap(milliseconds time_delta_ms);
    void CollectAndReturnLoots();
    void PushLootsToMap(milliseconds time_delta_ms);
    Dog::Id GetNextDogId();
    Loot::Id GetNextLootId();
    void MoveDogToContainer(Dog &&dog);
};

struct GameParam {
    milliseconds dog_retirement_time;
    LootGeneratorConfig loot_generator_config_;
};

class Game {
public:
    using Maps = std::vector<Map>;
    using GameSessions = std::deque<GameSession>;
    using TickSignal = sig::signal<void(milliseconds delta)>;
    Game() = default;
    Game(const GameParam& game_param) 
        : game_param_(std::move(game_param)){
    }
    void AddMap(const Map& map);
    void AddExtraData(const ExtraData& extra_data) {
        extra_data_ = std::move(extra_data);
    }
    void SetRandomizeSpawnPoints(bool randomize_spawn_points) {
        randomize_spawn_points_ = randomize_spawn_points;
    }
    bool GetRandomizeSpawnPoints() const {
        return randomize_spawn_points_;
    }
    const Maps& GetMaps() const noexcept {
        return maps_;
    }
    std::string_view GetLootTypes(const Map::Id& id) const {
        return extra_data_.GetLootTypes(*id);
    }
    const Map* FindMap(const Map::Id& id) const noexcept;
    GameSession* FindGameSession(const Map::Id& id) noexcept;
    GameSession* AddGameSession(const Map::Id& id);
    GameSessions& GetGameSessions();
    void SetGameSessions(const GameSessions& game_session);
    void Tick(milliseconds time_delta_ms);
    // Добавляем обработчик сигнала tick и возвращаем объект connection для управления,
    // при помощи которого можно отписаться от сигнала
    [[nodiscard]] sig::connection DoOnTick(const TickSignal::slot_type& handler) {
        return tick_signal_.connect(handler);
    }
    const LootGeneratorConfig& GetLootGeneratorConfig() const {
        return game_param_.loot_generator_config_;
    }
    const milliseconds& GetDogRetirementTime() {
        return game_param_.dog_retirement_time;
    }
    Game(const Game& other) {
        *this = other;
    }
    Game& operator=(const Game& other) {
        if (this == &other)
            return *this;
        randomize_spawn_points_ = other.randomize_spawn_points_;
        maps_ = other.maps_;
        extra_data_ = other.extra_data_;
        game_param_ = other.game_param_;
        game_sessions_ = other.game_sessions_;
        map_id_to_index_ = other.map_id_to_index_;
        map_id_to_game_sessions_index_ = other.map_id_to_game_sessions_index_;
        tick_signal_.connect(other.tick_signal_);
        return *this;
    }
private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    
    bool randomize_spawn_points_ = true;
    Maps maps_;
    ExtraData extra_data_;
    GameParam game_param_;
    GameSessions game_sessions_;
    MapIdToIndex map_id_to_index_;
    MapIdToIndex map_id_to_game_sessions_index_;
    TickSignal tick_signal_;
};

}  // namespace model
