
#ifndef GRAPHICS__GEOMETRY_H
#define GRAPHICS__GEOMETRY_H

#include <array>
#include <cmath>
#include <ostream>
#include <type_traits>

template <class T, class F>
static constexpr bool is_safe_numeric_conversion =
(
	(
		(
			( std::is_integral<T>::value && std::is_integral<F>::value ) || ( std::is_floating_point<T>::value && std::is_floating_point<F>::value )
		) && sizeof(T) >= sizeof(F)
	) || ( std::is_floating_point<T>::value && std::is_integral<F>::value )
) &&
(
	   ( std::is_signed<T>::value && std::is_signed<F>::value )
	|| ( std::is_unsigned<T>::value && std::is_unsigned<F>::value )
);

namespace geom {

/**
 * A point datatype.
 */
template<typename PRECISION>
struct Point {
private:
	PRECISION m_x;
	PRECISION m_y;
public:

	Point() : m_x(0), m_y(0) { }

	template<typename THEIR_PRECISION>
	explicit Point(const Point<THEIR_PRECISION>& src)
		: Point(static_cast<PRECISION>(src.x()), static_cast<PRECISION>(src.y()))
	{ }

	template<typename THEIR_PRECISION, typename = std::enable_if_t<is_safe_numeric_conversion<THEIR_PRECISION, PRECISION>>>
	operator Point<THEIR_PRECISION>() {
		return {static_cast<PRECISION>(x()), static_cast<PRECISION>(y())};
	}

	Point(const Point& src) : Point(src.x(),src.y()) { }

	Point(PRECISION x, PRECISION y) : m_x(x), m_y(y) { }

	PRECISION& x() { return m_x; }
	const PRECISION& x() const { return m_x; }
	PRECISION& y() { return m_y; }
	const PRECISION& y() const { return m_y; }

	void set(PRECISION x, PRECISION y) {
		m_x = x;
		m_y = y;
	}
	void set(const Point& src) {
		m_x = src.x();
		m_y = src.y();
	}

	/**
	 * Behaves like a 2 argument plusequals.
	 */
	void offset(PRECISION x, PRECISION y) {
		m_x += x;
		m_y += y;
	}

	template<typename PRECISION2>
	Point operator* (PRECISION2 rhs) const {
		Point result = *this;
		result *= rhs;
		return result;
	}
	template<typename PRECISION2>
	Point operator/ (PRECISION2 rhs) const {
		Point result = *this;
		result /= rhs;
		return result;
	}
	template<typename PRECISION2>
	Point& operator+= (const Point<PRECISION2>& rhs) {
		m_x += rhs.x();
		m_y += rhs.y();
		return *this;
	}
	template<typename PRECISION2>
	Point& operator-= (const Point<PRECISION2>& rhs) {
		m_x -= rhs.x();
		m_y -= rhs.y();
		return *this;
	}
	template<typename PRECISION2>
	Point& operator*= (PRECISION2 rhs) {
		m_x *= rhs;
		m_y *= rhs;
		return *this;
	}
	template<typename PRECISION2>
	Point& operator/= (PRECISION2 rhs) {
		m_x /= rhs;
		m_y /= rhs;
		return *this;
	}

	/**
	 * Assign that point to this one - copy the components
	 */
	Point& operator= (const Point& src) {
		m_x = src.x();
		m_y = src.y();
		return *this;
	}

};

/**
 * constructor helper - will chose a type parameter for Point
 * that will be at least as precise as x and y
 */
template<typename PRECISION1, typename PRECISION2>
auto make_point(PRECISION1 x, PRECISION2 y) -> Point<decltype(x+y)> {
	return {x,y};
}

const int POSITIVE_DOT_PRODUCT = 0;
const int NEGATIVE_DOT_PRODUCT = 1;

/**
 * These add the given point to this point in a
 * componentwise fashion, ie x = x + rhs.x
 *
 * Naturally, {+,-} don't modify and {+,-}= do.
 */
template<typename PRECISION, typename PRECISION2>
auto operator+ (const Point<PRECISION>& lhs, const Point<PRECISION2>& rhs) {
	return make_point(lhs.x() + rhs.x(), lhs.y() + rhs.y());
}
template<typename PRECISION, typename PRECISION2>
auto operator- (const Point<PRECISION>& lhs, const Point<PRECISION2>& rhs) {
	return make_point(lhs.x() - rhs.x(), lhs.y() - rhs.y());
}

template<typename PRECISION, typename PRECISION2>
auto deltaX(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return p2.x() - p1.x();
}
template<typename PRECISION, typename PRECISION2>
auto deltaY(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return p2.y() - p1.y();
}
template<typename PRECISION, typename PRECISION2>
Point<PRECISION> delta(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return {deltaX(p1, p2), deltaY(p1, p2)};
}
template<typename PRECISION, typename PRECISION2>
auto multiply(Point<PRECISION> p, PRECISION2 constant) {
	return make_point(p.x() * constant, p.y() * constant);
}
template<typename PRECISION, typename PRECISION2>
auto divide(Point<PRECISION> p, PRECISION2 constant) {
	return make_point(p.x() / constant, p.y() / constant);
}
template<typename PRECISION, typename PRECISION2>
auto add(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return make_point(p1.x() + p2.x(), p1.y() + p2.y());
}
template<typename PRECISION, typename PRECISION2>
auto distance(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return sqrt(pow(deltaX(p1, p2), 2) + pow(deltaY(p1, p2), 2));
}
template<typename PRECISION>
auto magnitudeSquared(Point<PRECISION> p) {
	return pow(p.x(), 2) + pow(p.y(), 2);
}
template<typename PRECISION>
auto magnitude(Point<PRECISION> p) {
	return sqrt(magnitudeSquared(p));
}
template<typename PRECISION>
auto unit(Point<PRECISION> p) {
	return divide(p, (PRECISION) magnitude(p));
}
template<typename PRECISION, typename PRECISION2>
auto dotProduct(Point<PRECISION> p1, Point<PRECISION2> p2) {
	return p1.x() * p2.x() + p1.y() * p2.y();
}
template<typename PRECISION>
Point<PRECISION> getPerpindular(Point<PRECISION> p) {
	return {p.y(),-p.x()};
}
template<typename PRECISION, typename PRECISION2>
auto project(Point<PRECISION> source, Point<PRECISION2> wall) {
	return multiply(wall, dotProduct(wall, source) / magnitudeSquared(wall));
}
template<typename PRECISION, typename POINT_LIST>
auto& farthestPoint(Point<PRECISION> p, const POINT_LIST& test_points) {
	auto farthestDistance = distancef(p, *std::begin(test_points));
	auto farthest_point = &(*std::begin(test_points));
	for (auto& test_point : test_points) {
		auto distance = distancef(p, test_point);
		if (farthestDistance < distance) {
			farthest_point = &test_point;
			farthestDistance = distance;
		}
	}
	return *farthest_point;
}
template<typename PRECISION, typename PRECISION2, typename POINT_LIST>
auto farthestFromLineWithSides(Point<PRECISION> p, Point<PRECISION2> q, POINT_LIST& test_points) {
	auto line = delta(p, q);
	std::array<decltype(*std::begin(test_points)),2> farthests;
	std::array<decltype(magnitude(perpindictularDeltaVectorToLine(line, delta(p, test_points[0])))),2> farthestDistance;
	for (auto& test_point : test_points) {
		auto relativeToP = delta(p, test_point);
		auto distance = magnitude(perpindictularDeltaVectorToLine(line, relativeToP));
		int dotProductSign = dotProduct(line, relativeToP) < 0 ? NEGATIVE_DOT_PRODUCT : POSITIVE_DOT_PRODUCT;
		if (farthestDistance[dotProductSign] < distance) {
			farthests[dotProductSign] = &test_point;
			farthestDistance[dotProductSign] = distance;
		}
	}
	return farthests;
}
template<typename PRECISION, typename PRECISION2, typename PRECISION3>
auto distanceToLine(Point<PRECISION> onLine_1, Point<PRECISION2> onLine_2, Point<PRECISION3> p) {
	return magnitude(perpindictularDeltaVectorToLine(onLine_1, onLine_2, p));
}
template<typename PRECISION, typename PRECISION2>
auto perpindictularDeltaVectorToLine(Point<PRECISION> onLine_1, Point<PRECISION2> onLine_2, Point<PRECISION> p) {
	return perpindictularDeltaVectorToLine(delta(onLine_1, onLine_2), delta(p, onLine_1));
}
template<typename PRECISION, typename PRECISION2>
auto perpindictularDeltaVectorToLine(Point<PRECISION> direction, Point<PRECISION2> p) {
	return delta(p, project(p, direction));
}

template<typename PRECISION, typename PRECISION2>
auto operator*(PRECISION lhs, const Point<PRECISION2>& rhs) {
	return rhs*lhs;
}

template<typename PRECISION>
std::ostream& operator<<(std::ostream& os, const Point<PRECISION>& p) {
	os << '{' << p.x() << ',' << p.y() << '}';
	return os;
}

template<typename PRECISION1, typename PRECISION2>
bool operator==(const Point<PRECISION1>& p1, const Point<PRECISION2>& p2) {
	return p1.x() == p2.x() && p1.y() == p2.y();
}

/**
 * Represents a rectangle, used as a bounding box.
 */
template<typename PRECISION>
class BoundBox {
public:
	using point_type = Point<PRECISION>;

private:
	point_type minpoint;
	point_type maxpoint;

public:
	BoundBox()
		: minpoint()
		, maxpoint()
	{ }

	template<typename THEIR_PRECISION>
	BoundBox(const BoundBox<THEIR_PRECISION>& src)
		: minpoint(src.min_point())
		, maxpoint(src.max_point())
	{ }

	BoundBox(PRECISION minx, PRECISION miny, PRECISION maxx, PRECISION maxy)
		: minpoint(minx,miny)
		, maxpoint(maxx,maxy)
	{ }

	template<typename THEIR_PRECISION, typename THEIR_PRECISION2>
	BoundBox(const Point<THEIR_PRECISION>& minpoint, const Point<THEIR_PRECISION2>& maxpoint)
		: minpoint(minpoint)
		, maxpoint(maxpoint)
	{ }

	template<typename THEIR_PRECISION, typename THEIR_PRECISION2, typename THEIR_PRECISION3>
	BoundBox(const Point<THEIR_PRECISION>& minpoint, THEIR_PRECISION2 width, THEIR_PRECISION3 height)
		: minpoint(minpoint)
		, maxpoint(minpoint)
	{
		maxpoint.offset(width, height);
	}

	/**
	 * These return their respective edge/point's location
	 */
	const PRECISION& minx() const { return min_point().x(); }
	const PRECISION& maxx() const { return max_point().x(); }
	const PRECISION& miny() const { return min_point().y(); }
	const PRECISION& maxy() const { return max_point().y(); }
	PRECISION& minx() { return min_point().x(); }
	PRECISION& maxx() { return max_point().x(); }
	PRECISION& miny() { return min_point().y(); }
	PRECISION& maxy() { return max_point().y(); }

	const point_type& min_point() const { return minpoint; }
	const point_type& max_point() const { return maxpoint; }
	point_type& min_point() { return minpoint; }
	point_type& max_point() { return maxpoint; }

	/**
	 * Calculate and return the center
	 */
	PRECISION get_xcenter() const { return (maxx() + minx()) / 2; }
	PRECISION get_ycenter() const { return (maxy() + miny()) / 2; }
	point_type get_center() const { return point_type(get_xcenter(), get_ycenter()); }

	/**
	 * Calculate and return the width/height
	 * ie. maxx/maxy - minx/miny respectively.
	 */
	PRECISION get_width()  const { return maxx() - minx(); }
	PRECISION get_height() const { return maxy() - miny(); }

	/**
	 * These behave like the plusequal operator
	 * They add their x and y values to all corners
	 */
	void offset(const point_type& make_relative_to) {
		this->minpoint += make_relative_to;
		this->maxpoint += make_relative_to;
	}

	void offset(PRECISION by_x, PRECISION by_y) {
		this->minpoint.offset(by_x, by_y);
		this->maxpoint.offset(by_x, by_y);
	}

	/**
	 * Does the given point coinside with this bbox?
	 * Points on the edges or corners are included.
	 */
	template<typename POINT>
	bool intersects(const POINT& test_pt) const {
		return intersects(test_pt.x(), test_pt.y());
	}

	bool intersects(PRECISION x, PRECISION y) const {
		if (x < minx() || maxx() < x || y < miny() || maxy() < y) {
			return false;
		} else {
			return true;
		}
	}

	/**
	 * Calculate and return the area of this rectangle.
	 */
	PRECISION area() const {
		return std::abs(get_width() * get_height());
	}

	/**
	 * These add the given point to this bbox - they
	 * offset each corner by this point. Usful for calculating
	 * the location of a box in a higher scope, or for moving
	 * it around as part of a calculation
	 *
	 * Naturally, the {+,-} don't modify and the {+,-}= do.
	 */
	BoundBox operator+ (const point_type& rhs) const {
		BoundBox result = *this;
		result.offset(rhs);
		return result;
	}

	BoundBox operator- (const point_type& rhs) const {
		BoundBox result = *this;
		result.offset(point_type(-rhs.x, -rhs.y));
		return result;
	}

	BoundBox& operator+= (const point_type& rhs) {
		this->offset(rhs);
		return *this;
	}

	BoundBox& operator-= (const point_type& rhs) {
		this->offset(point_type(-rhs.x, -rhs.y));
		return *this;
	}

	/**
	 * Assign that box to this one - copy it's minx, maxx, miny, and maxy.
	 */
	BoundBox& operator= (const BoundBox& src) {
		this->min_point() = src.min_point();
		this->max_point() = src.max_point();
		return *this;
	}
};

template<typename PRECISION1, typename PRECISION2>
bool operator==(const BoundBox<PRECISION1>& b1, const BoundBox<PRECISION2>& b2) {
	return b1.min_point() == b2.min_point() && b1.max_point() == b2.max_point();
}

} // end namespace geom

#endif /* GRAPHICS__GEOMETRY_H */
