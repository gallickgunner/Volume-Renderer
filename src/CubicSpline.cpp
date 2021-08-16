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

glm::vec3 CubicSpline::getPointOnSpline(float t, int segment_idx)
{
    // cubic polynomial Y(t) = a + bt + ct^2 + dt^3,  t is in range 0-1
    CubicCoefficiants& coeffs = spline[segment_idx];
    return coeffs.a + t * (coeffs.b + t * (coeffs.c + t * coeffs.d));
}


void CubicSpline::recomputeCoefficients(int inserted_idx, const std::vector<glm::vec3>& control_points)
{
    glm::vec3 vec(1.0);
    for(int i = inserted_idx; i < control_points.size() - 1; i++)
        coeffs[i] = vec / (( 4.0f * vec) - coeffs[i - 1]);
    coeffs.push_back(vec  / ( (2.0f * vec) - coeffs.back() ) );
}

void CubicSpline::calcCubicSpline(const std::vector<glm::vec3>& control_points)
{
    std::vector<glm::vec3> deltas(control_points.size()), derivatives(control_points.size());
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
        coeffs.push_back(glm::vec3(0.5f));
        glm::vec3 identity(1.0);
        for (int i = 1; i < n; i++)
            coeffs.push_back(identity / ((4.0f * identity) - coeffs[i - 1]) );
        coeffs.push_back(identity  / ( (2.0f * identity) - coeffs[n-1] ) );
    }

    deltas[0] = 3.0f * (control_points[1] - control_points[0]) * coeffs[0];
    for (int i = 1; i < n; i++)
        deltas[i] = ( 3.0f * (control_points[i + 1] - control_points[i - 1]) - deltas[i - 1]) * coeffs[i];
    deltas[n] = ( 3.0f * (control_points[n] - control_points[n - 1]) - deltas[n-1]) * coeffs[n];


    derivatives[n] = deltas[n];
    glm::vec3 nn = deltas[n-1] * coeffs[n];

    for (int i = n-1; i >= 0; i--)
        derivatives[i] = deltas[i] - coeffs[i] * derivatives[i+1];
    glm::vec3 cn = derivatives[0];
    /* now compute the coefficients of all the cubic polynomials corresponding to every segment "a + bx + cx^2 + dx^3"
     * Note that for N control points we have N-1 segments thus N-1 number of polynomials.
     */
    for (int i = 0; i < n; i++)
    {
        glm::vec3 a,b,c,d;
        a = control_points[i];
        b = derivatives[i];  // reverse indexing
        c = 3.0f * (control_points[i+1] - control_points[i]) - 2.0f * derivatives[i] - derivatives[i + 1];
        d = 2.0f * (control_points[i] - control_points[i+1]) + derivatives[i] + derivatives[i + 1];
        spline.push_back({a,b,c,d});
    }
}
