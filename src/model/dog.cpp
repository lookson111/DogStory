#include "model.h"

#include <stdexcept>

geom::Speed2D model::Dog::zero_speed_ = geom::Speed2D{0.0, 0.0};
namespace model {
using namespace std::literals;

std::string Dog::GetDirection() const
{
    switch (dir_) {
    case Direction::NORTH:
        return "U";
    case Direction::EAST:
        return "R";
    case Direction::SOUTH:
        return "D";
    case Direction::WEST:
        return "L";
    }
    return "U";
}

void Dog::Diraction(Move move, Dimension2D speed) {
    static constexpr double dzero = 0.0; 
    static constexpr double invert = -1.0; 
    switch (move) {
    case Move::LEFT:
        speed_ = { invert*speed, dzero };
        dir_ = Direction::WEST;
        break;
    case Move::RIGHT:
        speed_ = { speed, dzero };
        dir_ = Direction::EAST;
        break;
    case Move::UP:
        speed_ = { dzero, invert*speed };
        dir_ = Direction::NORTH;
        break;
    case Move::DOWN:
        speed_ = { dzero, speed };
        dir_ = Direction::SOUTH;
        break;
    case Move::STAND:
        speed_ = zero_speed_;
        break;
    }
}

Point2D Dog::GetEndPoint(std::chrono::milliseconds move_time_ms)
{
    auto msChronoToDoubleSec = [] (auto ms) {
        static constexpr double to_sec = 1000.0;
        return double(ms.count()) / to_sec;
    };
    if (IsStanding())
        return coord_; 
    double dt_second = msChronoToDoubleSec(move_time_ms);
    Point2D end_point;
    end_point.x = coord_.x + speed_.x * dt_second;
    end_point.y = coord_.y + speed_.y * dt_second;
    return end_point;
}

}  // namespace model
