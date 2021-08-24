#include "CubicSpline.h"
#include <iostream>
CubicSpline::CubicSpline()
{
    //ctor
}

CubicSpline::~CubicSpline()
{
    //dtor
}

glm::vec4 CubicSpline::getPointOnSpline(float t, float segment_idx)
{
    // cubic polynomial Y(t) = a + bt + ct^2 + dt^3,  t is in range 0-1
    CubicCoefficiants& coeffs = spline[segment_idx];
    return coeffs.a + t * (coeffs.b + t * (coeffs.c + t * coeffs.d));
}

glm::vec4 CubicSpline::getPointOnSpline(int iso_val)
{
    float t = 0;
    int segment_idx = 0;

    for(int i = 0; i < control_points.size(); i++)
    {
        if(control_points[i].iso_value == iso_val)
            return control_points[i].color;
        else  if(control_points[i].iso_value > iso_val)
        {
            segment_idx = i-1;
            t = (iso_val - control_points[i-1].iso_value) / (float)(control_points[i].iso_value - control_points[i-1].iso_value);
            break;
        }
    }

    // cubic polynomial Y(t) = a + bt + ct^2 + dt^3,  t is in range 0-1
    CubicCoefficiants& coeffs = spline[segment_idx];
    return coeffs.a + t * (coeffs.b + t * (coeffs.c + t * coeffs.d));
}

void CubicSpline::recomputeCoefficients(int inserted_idx, const std::vector<TransferFuncControlPoint>& control_points)
{
    glm::vec4 vec(1.0);
    for(int i = inserted_idx; i < control_points.size() - 1; i++)
        coeffs[i] = vec / (( 4.0f * vec) - coeffs[i - 1]);
    coeffs.push_back(vec  / ( (2.0f * vec) - coeffs.back() ) );
}

void CubicSpline::calcCubicSpline(const std::vector<TransferFuncControlPoint>& cp_list)
{
    control_points = cp_list;
    std::vector<glm::vec4> deltas(control_points.size()), derivatives(control_points.size());
    int n = control_points.size() - 1; // N+1 points, N segments

    /* Need to solve the equation, n = num of segments/curves between each pair of control points
     *
       [2 1       ] [ D0 ]   [3(y1 - y0)  ]
       : 1 4 1    : : D1 :   :3(y2 - y0)  :
       :  1 4 1   : : .. : = :     .      :
       :  .....   : : .. :   :     .      :
       :     1 4 1: : .. :   :3(yn - yn-2):
       [       1 2] [ Dn ]   [3(yn - yn-1)]

     * taken from: http://mathworld.wolfram.com/CubicSpline.html,
     * Algorithm taken from "An Introduction to Splines for Use in Computer Graphics and Geometric Modeling by Bartels" page 12-14
     *
     * After forward elimination above system transforms to
     *
       [1 C0       ] [ D0 ]   [  delta0  ]
       :  1 C1     : : D1 :   :  delta1  :
       :    1 C2   : : .. : = :    .     :
       :    ....   : : .. :   :    .     :
       :     1 Cn-1: : .. :   :    .     :
       [          1] [ Dn ]   [  deltaN  ]
     *
     * In our code, the left most matrix is stored in the vector "coeffs". The middle one is stored in "derivatives" and the right most matrix
     * is stored in vector "deltas"
     */

    spline.clear();
    coeffs.clear();
    //Coefficient vector can be stored. Will need to be recomputed from a certain point onwards whenever we have a new control point.
    if(coeffs.empty())
    {
        coeffs.push_back(glm::vec4(0.5f));
        glm::vec4 identity(1.0);
        for (int i = 1; i < n; i++)
            coeffs.push_back(identity / ((4.0f * identity) - coeffs[i - 1]) );
        coeffs.push_back(identity  / ( (2.0f * identity) - coeffs[n-1] ) );
    }

    deltas[0] = 3.0f * (control_points[1].color - control_points[0].color) * coeffs[0];
    for (int i = 1; i < n; i++)
        deltas[i] = ( 3.0f * (control_points[i + 1].color - control_points[i - 1].color) - deltas[i - 1]) * coeffs[i];

    deltas[n] = ( 3.0f * (control_points[n].color - control_points[n - 1].color) - deltas[n-1]) * coeffs[n];

    derivatives[n] = deltas[n];
    for (int i = n-1; i >= 0; i--)
        derivatives[i] = deltas[i] - coeffs[i] * derivatives[i+1];

    /* now compute the coefficients of all the cubic polynomials corresponding to every segment "a + bx + cx^2 + dx^3"
     * Note that for N control points we have N-1 segments thus N-1 number of polynomials.
     */
    for (int i = 0; i < n; i++)
    {
        glm::vec4 a,b,c,d;
        a = control_points[i].color;
        b = derivatives[i];
        c = 3.0f * (control_points[i+1].color - control_points[i].color) - 2.0f * derivatives[i] - derivatives[i + 1];
        d = 2.0f * (control_points[i].color - control_points[i+1].color) + derivatives[i] + derivatives[i + 1];
        spline.push_back({a,b,c,d});
    }
}
