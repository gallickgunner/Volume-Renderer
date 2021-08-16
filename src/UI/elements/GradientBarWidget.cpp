#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <algorithm>
#include <cmath>
#include <iostream>
#include "elements/GradientBarWidget.h"

GradientBarWidget::GradientBarWidget()
{
    knots_frameID = 0;
    color_knots.reserve(15);
    prev_width = -1.0f;
    //ctor
}

GradientBarWidget::~GradientBarWidget()
{
    //dtor
}


bool GradientBarWidget::render(float knot_frame_height, float gradient_height, float gradient_width)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 init_cursor_pos = ImGui::GetCursorPos();
    ImVec2 knot_frame_cursor_pos = init_cursor_pos + ImVec2(0, gradient_height + style.ItemSpacing.y);
    ImRect knot_frame_usable_bb;

    //min max values of the scale
    float v_min, v_max;
    knot_size = ImVec2(15, knot_frame_height - 3.0);
    knot_half_sz = ImVec2(knot_size.x/2.0f, knot_size.y/2.0f);

    if(gradient_width == 0.0f)
    {
        gradient_width = ImGui::GetContentRegionMax().x - ImGui::GetStyle().WindowPadding.x;
        knot_frame_usable_bb = ImRect(knot_frame_cursor_pos + knot_half_sz, knot_frame_cursor_pos + ImVec2(gradient_width, knot_frame_height) - knot_half_sz);

        if(prev_width != gradient_width)
        {
            //Always set position using getKnotRectFromValue() so that position of the knot is snapped to an isoValue and the value doesn't change when widget is scaled.
            for(int i = 0; i < color_knots.size(); i++)
            {
                color_knots[i].pos_x = getKnotRectFromValue(knot_frame_usable_bb, color_knots[i].value, color_knots[0].value, color_knots.back().value).GetCenter().x;
            }
            prev_width = gradient_width;
        }
    }

    if(knots_frameID == 0)
        knots_frameID = ImGui::GetID("##knotsframe");

    //If empty initialize with 2 color knots at the start and end
    if(color_knots.empty())
    {
        fixed_knot1_ID = ImGui::GetID("K1");
        fixed_knot2_ID = ImGui::GetID("K2");
        color_knots.push_back({"K1", 0,  init_cursor_pos.x + knot_half_sz.x, IM_COL32_BLACK});
        color_knots.push_back({"K2", 255, init_cursor_pos.x + gradient_width - knot_half_sz.x, IM_COL32_WHITE});
    }

    v_min = color_knots[0].value;
    v_max = color_knots.back().value;

    list_splitter.Split(win->DrawList, 2);
    list_splitter.SetCurrentChannel(win->DrawList, 1);
    ImVec2 old_pos = ImGui::GetCursorPos();
    for(int i = 0; i < color_knots.size(); i++)
    {
        ImGuiID knot_id = ImGui::GetID(color_knots[i].id.c_str());
        float curr_knot_pos = color_knots[i].pos_x;
        ImRect knot_bb(curr_knot_pos - knot_half_sz.x, knot_frame_usable_bb.Min.y, curr_knot_pos + knot_half_sz.x, knot_frame_usable_bb.Max.y);

        ImGui::SetItemAllowOverlap();
        ImGui::SameLine();

        //If the knot was clicked, activate it to show color picker panel below in the transfer func window
        if(renderColorKnot(color_knots[i].id.c_str(), knot_bb.GetCenter(), color_knots[i].color))
            active_knot = &color_knots[i];

        if (ImGui::IsItemActive() && knot_id != fixed_knot1_ID && knot_id != fixed_knot2_ID)
        {
            int min_idx = ImClamp(i-1, 0, (int)color_knots.size() - 1 );
            int max_idx = ImClamp(i+1, 0, (int)color_knots.size() - 1 );

            // relative min is previous knot's value + 1. +1 because prev knot's value is already occupied by that knot. Similar for rel_max
            int rel_min = color_knots[min_idx].value + 1;
            int rel_max = color_knots[max_idx].value - 1;
            color_knots[i].value = getScaleValueFromPosition(ImGui::GetMousePos().x, knot_frame_usable_bb, v_min, v_max, rel_min, rel_max);
            color_knots[i].pos_x = getKnotRectFromValue(knot_frame_usable_bb, color_knots[i].value, v_min, v_max).GetCenter().x;
        }
    }

    //Render gradient bar
    ImGui::GetCurrentContext()->HoveredIdAllowOverlap = false;
    ImGui::GetCurrentContext()->ActiveIdAllowOverlap = false;
    list_splitter.SetCurrentChannel(win->DrawList, 0);

    ImGui::SetCursorPos(old_pos);
    for(int i = 1; i < color_knots.size(); i++)
    {
        ColorKnot* prev_knot = &color_knots[i-1];
        ColorKnot* curr_knot = &color_knots[i];

        ImVec2 size_param(curr_knot->pos_x + knot_size.x - prev_knot->pos_x, gradient_height);
        renderRectMulti(ImVec2(prev_knot->pos_x - knot_half_sz.x, ImGui::GetCursorPosY()), size_param, prev_knot->color, curr_knot->color, curr_knot->color, prev_knot->color);

        if(i < color_knots.size() - 1)
            ImGui::SameLine();
    }

    ImVec4 bg_col(0.207, 0.31, 0.425, 0.433);
    if(renderColorKnotFrame(ImGui::GetCursorPos(), ImVec2(gradient_width, knot_frame_height), ImGui::GetColorU32(bg_col)))
    {
        if(ImGui::IsMouseClicked(1))
        {
            std::string id = "K" + std::to_string(color_knots.size() + 1);

            //When knots are added for the first time relative min/max is equal to absolute min/max i.e. the ends.
            int val = getScaleValueFromPosition(ImGui::GetMousePos().x, knot_frame_usable_bb, v_min, v_max, v_min, v_max);
            float pos_x = getKnotRectFromValue(knot_frame_usable_bb, val, v_min, v_max).GetCenter().x;

            float grey_val = (float)(val - v_min) / (v_max - v_min);
            color_knots.push_back({id, val, pos_x, ImGui::GetColorU32(ImVec4(grey_val, grey_val, grey_val, 1.0))});
            std::sort(color_knots.begin(), color_knots.end(), [](const ColorKnot& a, const ColorKnot& b)
            {
                return a.pos_x < b.pos_x;
            });
        }
    }
    list_splitter.Merge(win->DrawList);
}

ImRect GradientBarWidget::getKnotRectFromValue(ImRect knot_frame_bb, int v, int v_min, int v_max)
{
    float knot_t = getScaleRatioFromValue(v, v_min, v_max);
    const float knot_pos = ImLerp(knot_frame_bb.Min.x, knot_frame_bb.Max.x, knot_t);
    return ImRect(knot_pos - knot_half_sz.x, knot_frame_bb.Min.y, knot_pos + knot_half_sz.x, knot_frame_bb.Max.y);
}

float GradientBarWidget::getScaleRatioFromValue(int v, int v_min, int v_max)
{
    return (float)(v - v_min) / (float)(v_max - v_min);
}

int GradientBarWidget::getScaleValueFromPosition(float mpos_x, ImRect frame_bb, int abs_min, int abs_max, int rel_min, int rel_max)
{
    frame_bb.Translate(ImGui::GetWindowPos());
    float t = (mpos_x - frame_bb.Min.x) / frame_bb.GetWidth();
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

bool GradientBarWidget::renderColorKnot(const char* label, ImVec2 center, ImU32 col, bool border, float border_size, int num_segments)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiID knob_id = win->GetID(label);

    //convert to absolute coords
    center += (ImGui::GetWindowPos() - win->Scroll);
    ImRect total_bb(center - knot_half_sz, center + knot_half_sz);
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, knob_id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, knob_id, &hovered, &held, ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_AllowItemOverlap);

    ImDrawList* draw_list = win->DrawList;
    ImVec4 color = ImGui::ColorConvertU32ToFloat4(col);
    color.w = 0.6;
    draw_list->AddRectFilled(total_bb.Min, total_bb.Max, ImGui::GetColorU32(col), 2.0, ImDrawCornerFlags_All);

    //Knot border
    if (border && border_size > 0.0f)
    {
        draw_list->AddRect(total_bb.Min, total_bb.Max, IM_COL32(255, 0, 0, 255), 1.0, ImDrawCornerFlags_All, 1.0);
        draw_list->AddCircleFilled(center, 2, IM_COL32(170, 0, 0, 255), num_segments);
    }
    return pressed;
}

bool GradientBarWidget::renderColorKnotFrame(const ImVec2& cursorPos, const ImVec2& size_arg, ImU32 fill_col, bool border, float border_size, float rounding)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImVec2 window_pos = win->Pos;
    const ImGuiStyle& style = ImGui::GetStyle();

    ImRect bb(window_pos + cursorPos - win->Scroll, window_pos + cursorPos + size_arg - win->Scroll);

    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, knots_frameID))
        return false;

    win->DrawList->AddRectFilled(bb.Min, bb.Max, fill_col, rounding);

    //Draw border
    if(border && border_size > 0.0f)
    {
        win->DrawList->AddRect(bb.Min + ImVec2(1, 1), bb.Max + ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_BorderShadow), rounding, ImDrawCornerFlags_All, border_size);
        win->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImGuiCol_Border), rounding, ImDrawCornerFlags_All, border_size);
    }
    return ImGui::ItemHoverable(bb, knots_frameID);
}

void GradientBarWidget::renderRectMulti(const ImVec2& cursorPos, const ImVec2& size_arg, ImU32 col_upr_left, ImU32 col_upr_right, ImU32 col_bot_right, ImU32 col_bot_left)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImVec2 window_pos = win->Pos;

    ImRect bb(window_pos + cursorPos - win->Scroll, window_pos + cursorPos + size_arg - win->Scroll);
    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(std::to_string(cursorPos.x).c_str())))
        return;
   // ImGui::RenderNavHighlight(bb, ImGui::GetID(std::to_string(cursorPos.x).c_str()));
    win->DrawList->AddRectFilledMultiColor(bb.Min, bb.Max, col_upr_left, col_upr_right, col_bot_right, col_bot_left);
}

