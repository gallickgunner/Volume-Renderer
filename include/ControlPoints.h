#ifndef CONTROLPOINTS_H
#define CONTROLPOINTS_H

#include <string>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"


struct AlphaControlPoint
{
    std::string label;
    glm::vec3 point;    // (x,y,z) => (iso_value, opacity, 0)
};

struct ColorControlPoint
{
    std::string label;
    int iso_value;
    glm::vec3 point;
    glm::vec4 cp_color; // Color of the control point being drawn
};

#endif // CONTROLPOINTS_H
