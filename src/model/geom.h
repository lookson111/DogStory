#pragma once

#include <compare>

namespace geom {

using Dimension2D = double;

struct Vec2D {
    Vec2D() = default;
    Vec2D(Dimension2D x, Dimension2D y)
        : x(x)
        , y(y) {
    }

    Vec2D& operator*=(Dimension2D scale) {
        x *= scale;
        y *= scale;
        return *this;
    }

    auto operator<=>(const Vec2D&) const = default;

    Dimension2D x = 0;
    Dimension2D y = 0;
};

inline Vec2D operator*(Vec2D lhs, Dimension2D rhs) {
    return lhs *= rhs;
}

inline Vec2D operator*(Dimension2D lhs, Vec2D rhs) {
    return rhs *= lhs;
}

struct Point2D {
    Point2D() = default;
    Point2D(Dimension2D x, Dimension2D y)
        : x(x)
        , y(y) {
    }

    Point2D& operator+=(const Vec2D& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    auto operator<=>(const Point2D&) const = default;

    Dimension2D x = 0;
    Dimension2D y = 0;
};

inline Point2D operator+(Point2D lhs, const Vec2D& rhs) {
    return lhs += rhs;
}

inline Point2D operator+(const Vec2D& lhs, Point2D rhs) {
    return rhs += lhs;
}

struct Speed2D {
    Dimension2D x = 0, y = 0;

    auto operator<=>(const Speed2D&) const = default;
};

}  // namespace geom
