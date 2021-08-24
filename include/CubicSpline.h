#ifndef CUBICSPLINE_H
#define CUBICSPLINE_H
#include <string>
#include <vector>
#include "glm/vec4.hpp"

class CubicSpline
{
    public:
        CubicSpline();
        ~CubicSpline();

        struct TransferFuncControlPoint
        {
            std::string label;
            int iso_value;
            glm::vec4 color;
        };
        void calcCubicSpline(const std::vector<TransferFuncControlPoint>& control_points);
        void recomputeCoefficients(int inserted_idx, const std::vector<TransferFuncControlPoint>& control_points);
        glm::vec4 getPointOnSpline(int iso_value);
        glm::vec4 getPointOnSpline(float t, float segment_idx);

    private:
        struct CubicCoefficiants{
          glm::vec4 a, b, c, d;
        };
        std::vector<glm::vec4> coeffs;
        std::vector<CubicCoefficiants> spline;
        std::vector<TransferFuncControlPoint> control_points;
};

#endif // CUBICSPLINE_H
