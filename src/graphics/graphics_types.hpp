#ifndef GRAPHICS_TYPES_H
#define GRAPHICS_TYPES_H

#include <string>
#include <array>
#include <ostream>

#include "easygl_constants.hpp"

namespace graphics {

/**************** USEFUL TYPES YOU CAN PASS TO GRAPHICS FUNCTIONS **********/

/**
 * An (x,y) point data type. 
 */
class t_point {
public:
    float x;
    float y;

    /**
     * Behaves like a 2 argument plusequals (modifies the calling object).
     */
    void offset(float x, float y);

    /**
     * These add the given point to this point in a 
     * component-wise fashion, ie x = x + rhs.x
     *
     * Naturally, {+,-} don't modify the left-hand side and {+,-}= do.
     */
    t_point operator+(const t_point& rhs) const;
    t_point operator-(const t_point& rhs) const;
    t_point operator*(float rhs) const;
    t_point& operator+=(const t_point& rhs);
    t_point& operator-=(const t_point& rhs);
    t_point& operator*=(float rhs);

    /**
     * Assign that point to this one - copy the components
     */
    t_point& operator=(const t_point& src);

    t_point();
    t_point(const t_point& src);
    t_point(float x, float y);

};

t_point operator*(float lhs, const t_point& rhs);

t_point interpolate(const t_point& p1, float ratio, const t_point& p2);

inline std::ostream& operator<<(std::ostream& os, const t_point& p) {
    os << '{' << p.x << ',' << p.y << '}';
    return os;
}


/**
 * Represents a rectangle from x=left to x=right and from y=bottom to y=top.
 * Can be used in some graphics calls, and as a bounding box.
 */
class t_bound_box {
public:
    /**
     * These return their respective edge/point's location
     */
    const float& left() const;
    const float& right() const;
    const float& bottom() const;
    const float& top() const;
    float& left();
    float& right();
    float& bottom();
    float& top();

    const t_point& bottom_left() const;
    const t_point& top_right() const;
    t_point& bottom_left();
    t_point& top_right();

    t_point top_left() const;
    t_point bottom_right() const;

    std::pair<t_point, t_point> top_side() const { return {top_left(), top_right()}; }
    std::pair<t_point, t_point> right_side() const { return {top_right(), bottom_right()}; }
    std::pair<t_point, t_point> bottom_side() const { return {bottom_right(), bottom_left()}; }
    std::pair<t_point, t_point> left_side() const { return {bottom_left(), top_left()}; }

    template<typename T>
    auto get_side(const T& side) const {
        switch (side) {
            case T::LEFT:
                return left_side();
            case T::RIGHT:
                return right_side();
            case T::TOP:
                return top_side();
            case T::BOTTOM:
            default:
                return bottom_side();
        }
    }

    /**
     * Calculate and return the center
     */
    float get_xcenter() const;
    float get_ycenter() const;
    t_point get_center() const;

    /**
     * Calculate and return the width/height
     * ie. right/top - left/bottom respectively.
     */
    float get_width() const;
    float get_height() const;

    /**
     * These behave like the plusequal operator
     * They add their x and y values to all corners of the calling object.
     */
    void offset(const t_point& make_relative_to);
    void offset(float by_x, float by_y);

    /**
     * Is the given point inside this bbox?
     * Points on the edges or corners are included.
     */
    bool intersects(const t_point& test_pt) const;
    bool intersects(float x, float y) const;

    /**
     * Calculate and return the area of this rectangle.
     */
    float area() const;

    /**
     * These add the given point to this bbox - they
     * offset each corner by this point. Usful for calculating
     * the location of a box in a higher scope, or for moving
     * it around as part of a calculation
     *
     * Naturally, the {+,-} don't modify and the {+,-}= do.
     */
    t_bound_box operator+(const t_point& rhs) const;
    t_bound_box operator-(const t_point& rhs) const;
    t_bound_box& operator+=(const t_point& rhs);
    t_bound_box& operator-=(const t_point& rhs);

    /**
     * Assign that box to this one - copy it's left, right, bottom, and top.
     */
    t_bound_box& operator=(const t_bound_box& src);

    t_bound_box();
    t_bound_box(const t_bound_box& src);
    t_bound_box(float left, float bottom, float right, float top);
    t_bound_box(const t_point& p1, const t_point& p2);
    t_bound_box(const t_point& p1, float width, float height);
private:
    t_point bottomleft;
    t_point topright;
};

inline std::ostream& operator<<(std::ostream& os, const t_bound_box& bb) {
    os << '{' << bb.bottom_left() << '-' << bb.top_right() << '}';
    return os;
}

/**
 * A datatype that holds an RGB (red,green,blue) triplet; it is used in this
 * graphics library for specifying and holding colours.
 */
class t_color {
public:
    uint_fast8_t red;   // 8-bits per colour component
    uint_fast8_t green;
    uint_fast8_t blue;
    uint_fast8_t alpha;

    // Constructors and equality operations have the obvious meaning.
    t_color(uint_fast8_t r, uint_fast8_t g, uint_fast8_t b, uint_fast8_t a = 255);
    t_color(const t_color& src);
    t_color();
    bool operator==(const t_color& rhs) const;
    bool operator!=(const t_color& rhs) const;

    /*
     * Some useful functions for working with indexed colour,
     * t_color is a colour index (id); you can 
     * use the enumerated constants in easygl_constants.h to use named
     * colours (color_types) if you don't want to make your own rgb colours.
     */
    t_color(color_types src);
    t_color& operator=(const t_color&) = default;
    t_color& operator=(color_types color_enum);
    bool operator==(color_types rhs) const;
    bool operator!=(color_types rhs) const;
    
    static const std::array<t_color,29> predef_colors;
};


inline std::ostream& operator<<(std::ostream& os, const t_color& c) {
    os << "{r" << c.red << 'g' << c.green << 'b' << c.blue << 'a' << c.alpha << '}';
    return os;
}

}

#endif /* GRAPHICS_TYPES_H */

