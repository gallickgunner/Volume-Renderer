#ifndef GRADIENTBARWIDGET_H
#define GRADIENTBARWIDGET_H

#include <vector>
#include <string>

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

class GradientBarWidget
{
    public:
        GradientBarWidget();
        ~GradientBarWidget();

        struct ColorKnot{
            std::string id;
            int value;
            float pos_x;
            ImU32 color;
        };

        ColorKnot* active_knot;
        std::vector<ColorKnot> color_knots;
        std::string frame_label;
        ImGuiID knots_frameID;

        bool render(float knot_frame_height, float gradient_height, float width = 0.0f);

    private:
        float prev_width;
        ImGuiID fixed_knot1_ID, fixed_knot2_ID;
        ImVec2 knot_size, knot_half_sz;
        ImDrawListSplitter list_splitter;

        ImRect getKnotRectFromValue(ImRect knot_frame_bb, int v, int v_min, int v_max);
        float getScaleRatioFromValue(int v, int v_min, int v_max);
        int getScaleValueFromPosition(float mpos_x, ImRect frame_bb, int abs_min, int abs_max, int rel_min, int rel_max);

        bool renderColorKnot(const char* label, ImVec2 center, ImU32 col, bool border = true, float border_size = 3.0f, int num_segments = 20);
        bool renderColorKnotFrame(const ImVec2& cursorPos, const ImVec2& size_arg, ImU32 fill_col, bool border = true, float border_size = 1.0f, float rounding = 0.0f);
        void renderRectMulti(const ImVec2& cursorPos, const ImVec2& size_arg, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left);
};

#endif // GRADIENTBARWIDGET_H
