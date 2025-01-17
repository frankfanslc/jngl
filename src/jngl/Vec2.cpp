// Copyright 2018-2021 Jan Niklas Hasse <jhasse@bixense.com>
// For conditions of distribution and use, see copyright notice in LICENSE.txt

#include "Vec2.hpp"

#include <cmath>
#include <iostream>

namespace jngl {

Vec2::Vec2() = default;

Vec2::Vec2(const double x, const double y) : x(x), y(y) {
}

bool Vec2::isNull() const {
	return std::fpclassify(x) == FP_ZERO && std::fpclassify(y) == FP_ZERO;
}

} // namespace jngl

std::ostream& operator<<(std::ostream& os, const jngl::Vec2& v) {
	return os << "[x=" << v.x << ", y=" << v.y << "]";
}
