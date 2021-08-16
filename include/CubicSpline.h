#ifndef CUBICSPLINE_H
#define CUBICSPLINE_H

#include <vector>
#include "glm/vec3.hpp"
#include "ControlPoints.h"

class CubicSpline
{
    public:
        CubicSpline();
        ~CubicSpline();

        void calcCubicSpline(const std::vector<glm::vec3>& control_points);
        void recomputeCoefficients(int inserted_idx, const std::vector<glm::vec3>& control_points);
        glm::vec3 getPointOnSpline(float t, int segment_idx);

    private:
        struct CubicCoefficiants{
          glm::vec3 a, b, c, d;
        };
        std::vector<glm::vec3> coeffs;
        std::vector<CubicCoefficiants> spline;
};

#endif // CUBICSPLINE_H
