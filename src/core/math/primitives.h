/**
 * @file   primitives.h
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.12.15
 *
 * @brief  Declaration of some math primitives.
 */

#pragma once

#include <array>
#include <glm/glm.hpp>

#undef near
#undef far

namespace viscom { namespace math {

    template<typename real> using Seg2 = std::array<glm::tvec2<real, glm::highp>, 2>;
    template<typename real> using Seg3 = std::array<glm::tvec3<real, glm::highp>, 2>;

    template<typename real> using Tri2 = std::array<glm::tvec2<real, glm::highp>, 3>;
    template<typename real> using Tri3 = std::array<glm::tvec3<real, glm::highp>, 3>;

    template<typename real> struct Frustum {
        std::array<glm::tvec4<real, glm::highp>, 6> planes;

        glm::tvec4<real, glm::highp>& left() { return planes[0]; }
        const glm::tvec4<real, glm::highp>& left() const { return planes[0]; }
        glm::tvec4<real, glm::highp>& right() { return planes[1]; }
        const glm::tvec4<real, glm::highp>& right() const { return planes[1]; }
        glm::tvec4<real, glm::highp>& top() { return planes[2]; }
        const glm::tvec4<real, glm::highp>& top() const { return planes[2]; }
        glm::tvec4<real, glm::highp>& bttm() { return planes[3]; }
        const glm::tvec4<real, glm::highp>& bttm() const { return planes[3]; }
        glm::tvec4<real, glm::highp>& near() { return planes[4]; }
        const glm::tvec4<real, glm::highp>& near() const { return planes[4]; }
        glm::tvec4<real, glm::highp>& far() { return planes[5]; }
        const glm::tvec4<real, glm::highp>& far() const { return planes[5]; }
    };

    template<typename real> struct AABB2 {
        std::array<glm::tvec2<real, glm::highp>, 2> minmax;
    };

    template<typename real> struct AABB3 {
        std::array<glm::tvec3<real, glm::highp>, 2> minmax;
    };
}}
