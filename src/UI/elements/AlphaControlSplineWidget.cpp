#include "AlphaControlSplineWidget.h"
#include "glm/common.hpp"
#include "Utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>

AlphaControlSplineWidget::AlphaControlSplineWidget()
{
    knot_radius = glm::vec2(6.0);
    v_min = glm::vec2(0.0f, 0.0f);
    v_max = glm::vec2(255.0f, 1.0);
    knot_col_active = glm::vec4(0.984, 0.3588, 0.011, 0.933);
    knot_col_hover = glm::vec4(0.56, 0.133, 0.101, 0.933);
    knot_col_base = glm::vec4(0.611, 0.196, 0.153, 0.933);
    bg_col = glm::vec4(0.207, 0.31, 0.425, 0.733);
    //ctor
}

AlphaControlSplineWidget::AlphaControlSplineWidget(int min_dataset_value, int max_dataset_value)
{
    knot_radius = glm::vec2(6.0);
    v_min = glm::vec2(0.0f, 0.0f);
    v_max = glm::vec2(255.0f, 1.0);
    knot_col_active = glm::vec4(0.984, 0.37, 0.01, 1.0);
    knot_col_hover = glm::vec4(0.56, 0.133, 0.101, 0.733);
    knot_col_base = glm::vec4(0.811, 0.196, 0.153, 0.733);
    bg_col = glm::vec4(0.207, 0.31, 0.425, 0.733);
    active_knot = nullptr;
}

AlphaControlSplineWidget::~AlphaControlSplineWidget()
{
    //dtor
}

void AlphaControlSplineWidget::render(float height, float width)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImGuiStyle& style = ImGui::GetStyle();
    glm::vec2 init_cursor_pos = ImGui::GetCursorScreenPos();

    bool is_knot_hovered = false;
    int hovered_knot_idx = -1;

    if(width == 0.0f)
        width = ImGui::GetContentRegionMax().x - style.WindowPadding.x;

    frame_bb = ImRect(init_cursor_pos, init_cursor_pos + glm::vec2(width, height));

    //If empty initialize with 2 alpha knots at the start and end
    if(alpha_knots.empty())
    {
        alpha_knots.push_back({"A1", glm::vec3(0.0, 0.0, 0.0)});
        alpha_knots.push_back({"A2", glm::vec3(255, 1.0, 0.0)});
        points_arr.push_back(alpha_knots[0].point);
        points_arr.push_back(alpha_knots[1].point);
        alpha_spline.calcCubicSpline(points_arr);
    }

    //Draw Graph Frame
    renderGraphBackground(height, width);
    //win->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, ImDrawCornerFlags_All);
    ImGui::SetCursorScreenPos(init_cursor_pos + glm::vec2(0, height));
    renderCubicSpline(20);

    int del_idx = -1;
    for(int i = 0; i< alpha_knots.size(); i++)
    {
        glm::vec4 knot_col = knot_col_base;

        bool is_active = false;
        if(active_knot && active_knot->point.x == alpha_knots[i].point.x)
        {
            knot_col = knot_col_active;
            is_active = true;
        }

        ImGuiID knot_id = ImGui::GetID(alpha_knots[i].label.c_str());
        if( renderAlphaKnot(alpha_knots[i].label.c_str(), getAbsMousePositionFromPoint(glm::vec2(alpha_knots[i].point)), ImGui::GetColorU32(knot_col), is_active) )
            active_knot = &alpha_knots[i];

        if(ImGui::IsItemHovered())
        {
            is_knot_hovered = true;
            hovered_knot_idx = i;
            if(ImGui::IsMouseClicked(1))
            {
                del_idx = i;
                is_knot_hovered = false;
                hovered_knot_idx = -1;
            }
        }
        if (ImGui::IsItemActive())
        {
            is_knot_hovered = true;
            hovered_knot_idx = i;

            active_knot = &alpha_knots[i];
            int min_idx = ImClamp(i-1, 0, (int)alpha_knots.size() - 1 );
            int max_idx = ImClamp(i+1, 0, (int)alpha_knots.size() - 1 );

            // relative min is previous knot's value + 1. +1 because prev knot's value is already occupied by that knot. Similar for rel_max
            glm::vec2 rel_min = glm::vec2(alpha_knots[min_idx].point);
            rel_min.x += 1;

            glm::vec2 rel_max = glm::vec2(alpha_knots[max_idx].point);
            rel_max -= 1;

            if(i == 0)
            {
                rel_min = v_min;
                rel_max = v_min;
                rel_max.y = 1.0;
            }
            else if(i == alpha_knots.size() - 1)
            {
                rel_max = v_max;
                rel_min = v_max;
                rel_min.y = 0.0;
            }

            glm::vec2 new_point = getPointFromAbsMousePosition((glm::vec2)ImGui::GetMousePos(), rel_min, rel_max);
            alpha_knots[i].point = glm::vec3(new_point, 0.0);
            points_arr[i] = alpha_knots[i].point;
            alpha_spline.calcCubicSpline(points_arr);
        }
    }

    /*if(del_idx > 0 && del_idx < alpha_knots.size() - 1)
    {
        alpha_knots.erase(alpha_knots.begin() + del_idx);
        points_arr.erase(points_arr.begin() + del_idx);
        alpha_spline.calcCubicSpline(points_arr);
    }*/

    //If Right Clicked inside graph, add a control point
    if(is_knot_hovered || drawing_area_bb.Contains(ImGui::GetMousePos()))
    {
        glm::vec2 point;
        if(is_knot_hovered)
            point = alpha_knots[hovered_knot_idx].point;
        else
        {
            point = getPointFromAbsMousePosition((glm::vec2)ImGui::GetMousePos(), v_min, v_max);
            int segment_idx = -1;
            float segment_t = -1.0;
            for(int i = 0; i < alpha_knots.size(); i++)
            {
                if(alpha_knots[i].point.x >= point.x)
                {
                    segment_idx = i-1;
                    segment_t = (point.x - alpha_knots[i-1].point.x) / (alpha_knots[i].point.x - alpha_knots[i-1].point.x);
                    break;
                }
            }
            point = glm::clamp((glm::vec2)alpha_spline.getPointOnSpline(segment_t, segment_idx), v_min, v_max);
        }

        std::string tooltip_text = "(" + std::to_string(point.x) + ", " + std::to_string(point.y) + ")";
        ImGui::BeginTooltip();
        ImGui::Text("(%d, %.2f)", (int)point.x, (float)point.y);
        ImGui::EndTooltip();

        if(ImGui::IsMouseClicked(1) && del_idx == -1)
        {
            std::string knot_label = "A" + std::to_string(alpha_knots.size() + 1);
            alpha_knots.push_back({knot_label, glm::vec3(point, 0.0)});
            std::sort(alpha_knots.begin(), alpha_knots.end(), [](const AlphaControlPoint& a, const AlphaControlPoint& b)
            {
                return a.point.x < b.point.x;
            });

            points_arr.clear();
            for(int i = 0; i < alpha_knots.size(); i++)
                points_arr.push_back(alpha_knots[i].point);
            alpha_spline.calcCubicSpline(points_arr);
        }
    }

}



glm::vec2 AlphaControlSplineWidget::getScaleRatioFromPoint(glm::vec2 v)
{
    return (v - v_min) / (v_max - v_min);
}


glm::vec2 AlphaControlSplineWidget::getAbsMousePositionFromPoint(glm::vec2 point)
{
    glm::vec2 t = getScaleRatioFromPoint(point);
    return glm::mix(graph_bot_left, graph_top_right, t);
}

glm::vec2 AlphaControlSplineWidget::getPointFromAbsMousePosition(glm::vec2 pos, glm::vec2 rel_min, glm::vec2 rel_max)
{
    glm::vec2 t = (pos - (glm::vec2)drawing_area_bb.Min) / (glm::vec2)drawing_area_bb.GetSize();
    t = glm::clamp(t, glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f));
    t.y = 1.0 - t.y;

    glm::vec2 result;
    result = (v_max - v_min) * t;
    result.x = glm::round(result.x);
    result.x = glm::clamp(result.x, rel_min.x, rel_max.x);

    return result;
}


void AlphaControlSplineWidget::renderCubicSpline(int num_segments)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();

    if(alpha_knots.size() == 2)
    {
        win->DrawList->PathLineTo((ImVec2) getAbsMousePositionFromPoint(alpha_knots[0].point));
        win->DrawList->PathLineTo((ImVec2) getAbsMousePositionFromPoint(alpha_knots[1].point));
    }
    else
    {
        float t_step = 1.0f / (float)num_segments;

        //Render n-1 segments/curves.
        win->DrawList->PathLineTo((ImVec2) getAbsMousePositionFromPoint(alpha_knots[0].point));
        for(int i = 0; i < alpha_knots.size() - 1; i++)
        {
            for (int i_step = 1; i_step <= num_segments; i_step++)
            {
                glm::vec2 point = alpha_spline.getPointOnSpline(t_step * i_step, i);
                point = glm::clamp(point, v_min, v_max);
                win->DrawList->_Path.push_back((ImVec2) getAbsMousePositionFromPoint(point) );
            }
        }
    }

    glm::vec4 spline_col(1.0f, 1.0f, 0.0f, 1.0f);
    win->DrawList->PathStroke(ImGui::GetColorU32(spline_col), false, 2.0f);

}

bool AlphaControlSplineWidget::renderAlphaKnot(const char* label, glm::vec2 center, ImU32 col, bool is_active, bool border, float border_size, int num_segments)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();
    const ImGuiID knot_id = win->GetID(label);

    ImRect total_bb(center - knot_radius, center + knot_radius);
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, knot_id))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(total_bb, knot_id, &hovered, &held, ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_AllowItemOverlap);

    ImDrawList* draw_list = win->DrawList;
    ImVec4 color = ImGui::ColorConvertU32ToFloat4(col);
    color.w = 0.6;

    if(hovered && !is_active)
        col = ImGui::GetColorU32(knot_col_hover);
    draw_list->AddCircleFilled(center, knot_radius.x, col, num_segments);

    //Knot border
    if (border && border_size > 0.0f)
        draw_list->AddCircle(center, knot_radius.x, col, num_segments, border_size);
    return pressed;
}

void AlphaControlSplineWidget::renderGraphBackground(float height, float width)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();
    win->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, ImDrawCornerFlags_All);

    //bot left and top right corners of the entire frame
    glm::vec2 frame_bot_left = frame_bb.GetBL();
    glm::vec2 frame_top_right = frame_bb.GetTR();
    float padding_draw_area = 40;
    float label_y_factor = style.FramePadding.y + ImGui::GetTextLineHeight();

    //bot left and top right corners of the drawing region
    float labely_width = ImGui::CalcTextSize("Opacity").x;
    float axes_padding = labely_width + labely_width/3.0;

    graph_bot_left.x = frame_bot_left.x + axes_padding;
    graph_bot_left.y = frame_bot_left.y - axes_padding;
    graph_top_right = frame_top_right + glm::vec2(-padding_draw_area, padding_draw_area);
    drawing_area_bb = ImRect(ImVec2(graph_bot_left.x, graph_top_right.y), ImVec2(graph_top_right.x, graph_bot_left.y));

    //Draw X-Axis
    ImGui::SetCursorScreenPos(frame_bot_left + glm::vec2(width/2.0, -label_y_factor));
    ImGui::Text("Isovalue");
    win->DrawList->PathLineTo(graph_bot_left);
    win->DrawList->PathLineTo(frame_bot_left + glm::vec2(width, -axes_padding));
    win->DrawList->PathStroke(IM_COL32_WHITE, false, 2.0f);

    //Draw Scale on X-axis
    for(int i = 0; i <= 255; i+= 50)
    {
        if(i == 250)
            i = 255;

        glm::vec2 abs_mpos = getAbsMousePositionFromPoint(glm::vec2(i, 0.0));
        if(i != 0)
        {
            win->DrawList->PathLineTo(abs_mpos);
            win->DrawList->PathLineTo(abs_mpos + glm::vec2(0.0, 6.0));
            win->DrawList->PathStroke(IM_COL32_WHITE, false, 1.0f);
        }
        std::string label = std::to_string(i);
        ImGui::SetCursorScreenPos(ImVec2(abs_mpos + glm::vec2(-ImGui::CalcTextSize(label.c_str()).x/2.0, 6.0)));
        ImGui::Text(label.c_str());
    }

    //Draw Y-Axis
    float text_width = ImGui::CalcTextSize("Opacity").x + style.FramePadding.x;
    ImGui::SetCursorScreenPos(getAbsMousePositionFromPoint(glm::vec2(0.0, 0.5)) - glm::vec2(text_width, ImGui::GetTextLineHeight()/2.0));
    ImGui::Text("Opacity");
    win->DrawList->PathLineTo(drawing_area_bb.Min);
    win->DrawList->PathLineTo(graph_bot_left);
    win->DrawList->PathStroke(IM_COL32_WHITE, false, 2.0f);

    //Draw Scale on Y-axis
    for(int i = 1; i <= 5; i++)
    {
        float val = i * 0.2;

        glm::vec2 abs_mpos = getAbsMousePositionFromPoint(glm::vec2(0.0, val));
        float text_width = 36.0;
        if(i != 5)
        {
            win->DrawList->PathLineTo(abs_mpos);
            win->DrawList->PathLineTo(abs_mpos - glm::vec2(6.0, 0.0));
            win->DrawList->PathStroke(IM_COL32_WHITE, false, 1.0f);
        }
        ImGui::SetCursorScreenPos(ImVec2(abs_mpos - glm::vec2(text_width, ImGui::GetTextLineHeight()/2.0)));
        ImGui::Text("%.1f", val);
    }


    //Draw the top and the right most axes that define the drawing region together with the X and Y axes.
    /*
     *  --------T-------
     *  |              |
     *  L              R
     *  |-------B------|
     */
    win->DrawList->PathLineTo(drawing_area_bb.GetTL());
    win->DrawList->PathLineTo(drawing_area_bb.GetTR());
    win->DrawList->PathLineTo(drawing_area_bb.GetBR());
    win->DrawList->PathStroke(IM_COL32_WHITE, false, 0.4f);
}

