#ifndef UTILS_H
#define UTILS_H

#include "imgui/imgui_internal.h"
#include "glm/vec2.hpp"


float getScaleRatioFromValue(int v, int v_min, int v_max)
{
    return (float)(v - v_min) / (float)(v_max - v_min);
}

ImRect getKnotRectFromValue(ImRect frame_bb, glm::vec2 knot_halflen, int v, int v_min, int v_max)
{
    float t = getScaleRatioFromValue(v, v_min, v_max);
    const float knot_pos = ImLerp(frame_bb.Min.x, frame_bb.Max.x, t);
    return ImRect(knot_pos - knot_halflen.x, frame_bb.Min.y, knot_pos + knot_halflen.x, frame_bb.Max.y);
}

int getScaleValueFromPosition(ImRect frame_bb, glm::vec2 pos, int abs_min, int abs_max, int rel_min, int rel_max)
{
    float t = (pos.x - frame_bb.Min.x) / frame_bb.GetWidth();
    int result;
    if (t < 1.0f)
    {
        float v_new = (abs_max - abs_min) * t;
        result = abs_min + (v_new + 0.5f);
        result = ImClamp(result, rel_min, rel_max);
    }
    else
        result = rel_max;
    return result;
}

#endif // UTILS_H
