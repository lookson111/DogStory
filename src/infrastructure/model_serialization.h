#pragma once
#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>

#include "../model/model.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, Map::Id& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj);
}

template <typename Archive>
void serialize(Archive& ar, Loot& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id);
    ar&(obj.type);
    ar&(obj.pos);
}

template <typename Archive>
void serialize(Archive& ar, Dog::Id& obj, [[maybe_unused]] const unsigned version) {
    ar& (*obj);
}
template <typename Archive>
void serialize(Archive& ar, GameSession::Dogs& obj, [[maybe_unused]] const unsigned version) {
    for (auto &dog : obj)
        ar& (dog);
}

template <typename Archive>
void serialize(Archive& ar, model::Game::GameSessions& objs, [[maybe_unused]] const unsigned version) {
    for (auto &odj : objs)
        ar& (odj);
}
}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPoint())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDir())
        , score_(dog.GetScore())
        , bag_content_(dog.GetLoots()) 
        , lifetime_(dog.GetLifetime().count())
        , stay_time_(dog.GetStayTime().count())
    {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, name_, pos_/*, bag_capacity_*/};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        dog.SetLastMoveTime(std::chrono::milliseconds(lifetime_-stay_time_));
        dog.SetLifeTime(std::chrono::milliseconds(lifetime_));
        model::Loots bag_content = bag_content_;
        for (auto it = bag_content.begin(), itn = bag_content.begin(); it != bag_content.end(); it = itn) {
            itn = std::next(it);
            dog.PutTheLoot(bag_content, it);
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    geom::Speed2D speed_;
    model::Direction direction_ = model::Direction::NORTH;
    model::Score score_ = 0;
    model::Loots bag_content_;
    long long stay_time_;
    long long lifetime_;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& game_session)
        : map_id_(game_session.MapId())
        , last_loot_id_(game_session.GetLastLootId())
        , last_dog_id_(game_session.GetLastDogId())
        , loots_(game_session.GetLoots())
    {
        for (auto &[id, dog] : game_session.GetDogs())
            dogs_repr_.push_back(DogRepr(dog));
    }

    [[nodiscard]] model::GameSession Restore(
            const model::Map* map, 
            bool randomize_spawn_points,
            const model::LootGeneratorConfig loot_generator_config) const {
        model::GameSession game_session(map, randomize_spawn_points, loot_generator_config);
        game_session.SetLastLootId(last_loot_id_);
        model::GameSession::Dogs dogs;
        for (auto &dog_repr : dogs_repr_) {
            auto dog = dog_repr.Restore();
            auto id = dog.GetId();
            dogs.emplace(id, dog);
        }
        game_session.SetDogs(dogs);
        game_session.SetLoots(loots_);
        game_session.SetLastLootId(last_loot_id_);
        game_session.SetLastDogId(last_dog_id_);
        return game_session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& map_id_;
        ar&* last_loot_id_;
        ar&* last_dog_id_;
        ar& loots_;
        ar& dogs_repr_;
    }
    [[nodiscard]] model::Map::Id GetMapId() const {
        return map_id_;
    }

private:
    model::Map::Id map_id_{""};
    model::Loot::Id last_loot_id_{0};
    model::Dog::Id last_dog_id_{0};
    model::Loots loots_;
    std::list<DogRepr> dogs_repr_;
};


class GameSessionsRepr {
public:
    GameSessionsRepr() = default;

    explicit GameSessionsRepr(const model::Game::GameSessions& game_sessions)
    {
        for (auto &gs : game_sessions)
            game_sessions_repr.push_back(GameSessionRepr(gs));
    }

    [[nodiscard]] model::Game::GameSessions Restore(model::Game &game) const {
        model::Game::GameSessions gs;
        for (auto &game_session_repr : game_sessions_repr) {
            auto map = game.FindMap(game_session_repr.GetMapId());
            gs.push_back(game_session_repr.Restore(map, game.GetRandomizeSpawnPoints(), 
                game.GetLootGeneratorConfig()));
        }
        return gs;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_sessions_repr;
    }

private:
    std::list<GameSessionRepr> game_sessions_repr;
};


class GameRepr {
public:
    GameRepr() = default;

    explicit GameRepr(model::Game& game)
        : game_sessions_repr(game.GetGameSessions())
    {
    }

    void Restore(model::Game &game) const {
        game.SetGameSessions(game_sessions_repr.Restore(game));
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_sessions_repr;
    }

private:
    GameSessionsRepr game_sessions_repr;
};

}  // namespace serialization
