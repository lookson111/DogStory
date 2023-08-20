#pragma once 
#include "../sdk.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <atomic>
#include <map>
#include <memory>
#include <chrono>
#include "../util/tagged_uuid.h"
#include "geom.h"

namespace model {
using namespace geom;
using milliseconds = std::chrono::milliseconds;

using LootParam = int;
using LootsParam = std::vector<LootParam>;
using Score = unsigned;
using LostObjectType = unsigned;

enum class Move {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    STAND
};

enum class Direction {
    NORTH, 
    SOUTH,
    WEST, 
    EAST
};

struct Loot {
    using Id = util::Tagged<uint32_t, Loot>;

    Id id{ 0u };
    LostObjectType type{ 0u };
    Point2D pos;

    [[nodiscard]] auto operator<=>(const Loot&) const = default;
};
using Loots = std::list<Loot>;

class Dog {
public:
    using Id = util::Tagged<uint64_t, Dog>;

    Dog(Id id, std::string nickname, Point2D coord) :
        nickname_(std::move(nickname)), id_(std::move(id)),
        coord_(std::move(coord)){
    }
    const Id& GetId() const {
        return id_;
    }
    const std::string& GetName() const noexcept {
        return nickname_;
    }
    const Point2D& GetPoint() const {
        return coord_;
    }
    const Point2D& GetPrevPoint() const {
        return prev_coord_;
    }
    const Speed2D& GetSpeed() const {
        return speed_;
    }
    std::string GetDirection() const;
    Direction GetDir() const {
        return dir_;
    }
    void Diraction(Move move, Dimension2D speed);
    Point2D GetEndPoint(milliseconds move_time_ms);
    void SetPoint(Point2D coord) {
        std::swap(prev_coord_, coord_);
        coord_ = coord;
    }
    bool IsStanding() {
        return speed_ == zero_speed_;
    }
    void Stop() {
        speed_ = zero_speed_;
    }
    void PutTheLoot(Loots &session_loots, auto it_loot) {
        loots_.splice(loots_.end(), session_loots, it_loot);
    }
    const Loots& GetLoots() const {
        return loots_;
    }
    void LootsReturn(const LootsParam& loots_param) {
        for (const auto& loot : loots_) {
            score_ += loots_param[loot.type];
        }
        return loots_.clear();
    }
    Score GetScore() const {
        return score_;
    }
    void SetSpeed(const Speed2D& speed) {
        speed_ = speed;
    }
    void SetDirection(Direction dir) {
        dir_ = dir;
    }
    void AddScore(Score score) {
        score_ = score;
    }
    milliseconds GetStayTime() const {
        return lifetime_-last_move_time_;
    }
    milliseconds GetLifetime() const {
        return lifetime_;
    }
    void IncLifetime(milliseconds time_delta_ms) {
        lifetime_ += time_delta_ms;
        if (speed_ != zero_speed_) {
            last_move_time_ = lifetime_;
        }
    }
    void SetLastMoveTime(milliseconds time_ms) {
        last_move_time_ = time_ms;
    }
    void SetLifeTime(milliseconds time_ms) {
        lifetime_ = time_ms;
    }
private:
    static Speed2D zero_speed_;
    Id id_ = Id{0};
    std::string nickname_ = "";
    Point2D coord_;
    Point2D prev_coord_;
    Speed2D speed_ = zero_speed_;
    Direction dir_ = Direction::NORTH;
    Loots loots_; 
    Score score_ = 0;
    milliseconds last_move_time_{0};
    milliseconds lifetime_{0};
};

}  // namespace model
