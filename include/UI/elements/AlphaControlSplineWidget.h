#ifndef ALPHACONTROLSPLINEWIDGET_H
#define ALPHACONTROLSPLINEWIDGET_H

#include <vector>
#include <string>
#include "CubicSpline.h"
#include "glm/vec2.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

class AlphaControlSplineWidget
{
    public:
        AlphaControlSplineWidget();
        AlphaControlSplineWidget(int min_dataset_value, int max_dataset_value);
        ~AlphaControlSplineWidget();

        struct AlphaControlPoint
        {
            std::string label;
            glm::vec3 point;    // (x,y,z) => (iso_value, opacity, 0)
        };

        std::vector<AlphaControlPoint> alpha_knots;
        AlphaControlPoint* active_knot;
        void render(float height, float width = 0.0);

    private:
        CubicSpline alpha_spline;
        glm::vec2 knot_radius, v_min, v_max, graph_bot_left, graph_top_right;
        glm::vec4 bg_col, knot_col_active, knot_col_hover, knot_col_base;
        std::vector<glm::vec3> points_arr;
        ImRect frame_bb, drawing_area_bb;

        ImRect getKnotRectFromValue(glm::vec2 v);
        glm::vec2 getScaleRatioFromPoint(glm::vec2 v);
        glm::vec2 getPointFromAbsMousePosition(glm::vec2 pos, glm::vec2 rel_min, glm::vec2 rel_max);
        glm::vec2 getAbsMousePositionFromPoint(glm::vec2 point);

        void renderCubicSpline(int num_segments = 20);
        void renderGraphBackground(float height, float width);
        bool renderAlphaKnot(const char* label, glm::vec2 center, ImU32 col, bool is_active, bool border = true, float border_size = 3.0f, int num_segments = 20);
        //bool renderColorKnotFrame(const ImVec2& cursorPos, const ImVec2& size_arg, ImU32 fill_col, bool border = true, float border_size = 1.0f, float rounding = 0.0f);
        //void renderRectMulti(const ImVec2& cursorPos, const ImVec2& size_arg, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left);
};

#endif // ALPHACONTROLSPLINEWIDGET_H
