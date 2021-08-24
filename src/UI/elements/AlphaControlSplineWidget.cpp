#include "elements/AlphaControlSplineWidget.h"
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
    bool snapping = false;
    int hovered_knot_idx = -1;

    if(width == 0.0f)
        width = ImGui::GetContentRegionMax().x - style.WindowPadding.x;

    frame_bb = ImRect(init_cursor_pos, init_cursor_pos + glm::vec2(width, height));

    //If empty initialize with 2 alpha knots at the start and end
    if(alpha_knots.empty())
    {
        alpha_knots.push_back({"A1", 0, glm::vec4(0, 0, 0, 0)});
        alpha_knots.push_back({"A3", 141, glm::vec4(0, 0, 0, 0.759)});
        alpha_knots.push_back({"A4", 149, glm::vec4(0, 0, 0, 0.45)});
        alpha_knots.push_back({"A2", 255, glm::vec4(0, 0, 0, 1)});
        alpha_spline.calcCubicSpline(alpha_knots);
    }

    //Draw Graph Frame
    renderGraphBackground(height, width);
    ImGui::SetCursorScreenPos(init_cursor_pos + glm::vec2(0, height));
    renderCubicSpline(255);

    int del_idx = -1;
    for(int i = 0; i< alpha_knots.size(); i++)
    {
        glm::vec4 knot_col = knot_col_base;

        bool is_active = false;
        if(active_knot && active_knot->iso_value == alpha_knots[i].iso_value)
        {
            knot_col = knot_col_active;
            is_active = true;
        }

        ImGuiID knot_id = ImGui::GetID(alpha_knots[i].label.c_str());
        glm::vec2 coordinate_point(alpha_knots[i].iso_value, alpha_knots[i].color.w);
        ImGui::SetItemAllowOverlap();
        if( renderAlphaKnot(alpha_knots[i].label.c_str(), getAbsMousePositionFromPoint(coordinate_point), ImGui::GetColorU32(knot_col), is_active) )
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
            glm::vec2 rel_min = glm::vec2(alpha_knots[min_idx].iso_value, alpha_knots[min_idx].color.w);
            rel_min.x += 1;

            glm::vec2 rel_max = glm::vec2(alpha_knots[max_idx].iso_value, alpha_knots[max_idx].color.w);
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

            glm::vec2 new_coordinate_point = getPointFromAbsMousePosition((glm::vec2)ImGui::GetMousePos(), rel_min, rel_max);

            if(snapping)
            {
                int yval = new_coordinate_point.y * 100;
                int upper_boundary = (yval + 10.0f)/10.0f;
                upper_boundary *= 10;
                int lower_boundary = upper_boundary - 10;

                float upper_boundf = upper_boundary/100.0f;
                float lower_boundf = upper_boundf - 0.1f;

                if(upper_boundary - yval < 1)
                    new_coordinate_point.y = upper_boundf;
                else if(yval - lower_boundary < 1)
                    new_coordinate_point.y = lower_boundf;
            }

            alpha_knots[i].color.w = new_coordinate_point.y;
            alpha_knots[i].iso_value = new_coordinate_point.x;
            alpha_spline.calcCubicSpline(alpha_knots);
        }
    }

    if(del_idx > 0 && del_idx < alpha_knots.size() - 1)
    {
        alpha_knots.erase(alpha_knots.begin() + del_idx);
        alpha_spline.calcCubicSpline(alpha_knots);
    }

    //If Right Clicked inside graph, add a control point
    if(is_knot_hovered || drawing_area_bb.Contains(ImGui::GetMousePos()))
    {
        glm::vec2 coord;
        if(is_knot_hovered)
            coord = glm::vec2(alpha_knots[hovered_knot_idx].iso_value, alpha_knots[hovered_knot_idx].color.w);
        else
        {
            coord = getPointFromAbsMousePosition((glm::vec2)ImGui::GetMousePos(), v_min, v_max);
            coord = glm::vec2(coord.x, alpha_spline.getPointOnSpline(coord.x).w);
            coord = glm::clamp(coord, v_min, v_max);
        }


        ImGui::BeginTooltip();
        std::string tooltip_text = "";
        if(is_knot_hovered)
            tooltip_text = alpha_knots[hovered_knot_idx].label + ": (%d, %.3f)";
        else
            tooltip_text = "(%d, %.3f)";
        ImGui::Text(tooltip_text.c_str(), (int)coord.x, (float)coord.y);
        ImGui::EndTooltip();

        if(ImGui::IsMouseClicked(1) && del_idx == -1)
        {
            std::string knot_label = "A" + std::to_string(alpha_knots.size() + 1);
            alpha_knots.push_back({knot_label, coord.x, glm::vec4(0, 0, 0, coord.y)});
            std::sort(alpha_knots.begin(), alpha_knots.end(), [](const CubicSpline::TransferFuncControlPoint& a, const CubicSpline::TransferFuncControlPoint& b)
            {
                return a.iso_value < b.iso_value;
            });

            alpha_spline.calcCubicSpline(alpha_knots);
        }
    }

    /*/Render Knot Labels
    for(int i = 0; i < alpha_knots.size(); i++)
    {
        ImGui::SetCursorScreenPos(getAbsMousePositionFromPoint(alpha_knots[i].point) + glm::vec2(-knot_radius.x, ImGui::GetTextLineHeight() - knot_radius.y));
        ImGui::Text(alpha_knots[i].label.c_str());
    }*/

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
    result.x = (int) glm::clamp(result.x, rel_min.x, rel_max.x);

    return result;
}


void AlphaControlSplineWidget::renderCubicSpline(int num_segments)
{
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();

    win->DrawList->PathLineTo(getAbsMousePositionFromPoint(glm::vec2(alpha_knots[0].iso_value, alpha_knots[0].color.w)));

    if(alpha_knots.size() == 2)
        win->DrawList->PathLineTo(getAbsMousePositionFromPoint(glm::vec2(alpha_knots[1].iso_value, alpha_knots[1].color.w)));
    else
    {
        float t_step = 1.0f / (float)num_segments;

        //Render n-1 segments/curves.
        for(int i = 0; i < alpha_knots.size() - 1; i++)
        {
            for (int i_step = 1; i_step <= num_segments; i_step++)
            {
                glm::vec2 coord;
                coord.x = alpha_knots[i].iso_value + (t_step * i_step * (alpha_knots[i+1].iso_value - alpha_knots[i].iso_value));
                coord.y = glm::clamp(alpha_spline.getPointOnSpline(t_step * i_step, i).w, v_min.y, v_max.y);
                win->DrawList->_Path.push_back(getAbsMousePositionFromPoint(coord) );
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
   // ImGui::ItemSize(total_bb, style.FramePadding.y);
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
    glm::vec4 abc(0.2, 0.2, 0.2, 0.7);
    win->DrawList->AddRectFilled(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(abc), 0.0f, ImDrawCornerFlags_All);

    //bot left and top right corners of the entire frame
    glm::vec2 frame_bot_left = frame_bb.GetBL();
    glm::vec2 frame_top_right = frame_bb.GetTR();

    //Constants for UI
    float padding_draw_area = 60;
    float labely_width = ImGui::CalcTextSize("Opacity").x;
    float padding_yaxis = labely_width * 2.0;
    float padding_xaxis = labely_width;

    //bot left and top right corners of the drawing region
    graph_bot_left.x = frame_bot_left.x + padding_yaxis;
    graph_bot_left.y = frame_bot_left.y - padding_xaxis;
    graph_top_right = frame_top_right + glm::vec2(-padding_draw_area, padding_draw_area);
    drawing_area_bb = ImRect(ImVec2(graph_bot_left.x, graph_top_right.y), ImVec2(graph_top_right.x, graph_bot_left.y));

    win->DrawList->AddRectFilled(drawing_area_bb.Min, drawing_area_bb.Max, ImGui::GetColorU32(bg_col), 0.0f, ImDrawCornerFlags_All);

    //Draw X-Axis
    float labelx_offsety = style.FramePadding.y  * 2.0 + ImGui::GetTextLineHeight();
    ImGui::SetCursorScreenPos(frame_bot_left + glm::vec2(width/2.0, -labelx_offsety));
    ImGui::Text("Isovalue");
    win->DrawList->PathLineTo(graph_bot_left);
    win->DrawList->PathLineTo(graph_bot_left + glm::vec2(drawing_area_bb.GetWidth() + padding_draw_area - 10, 0));
    win->DrawList->PathStroke(IM_COL32_WHITE, false, 2.0f);

    //Draw arrowhead at the end of the X-axis
    glm::vec2 p1(graph_bot_left + glm::vec2(drawing_area_bb.GetWidth() + padding_draw_area - 10, 0));
    glm::vec2 p2(p1.x - 12, p1.y - 6);
    glm::vec2 p3(p1.x - 12, p1.y + 6);
    win->DrawList->AddTriangleFilled(p1, p2, p3, IM_COL32_WHITE);

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
    glm::vec2 offset(style.FramePadding.x - padding_yaxis, drawing_area_bb.GetHeight()/2.0 - ImGui::GetTextLineHeight()/2.0);
    ImGui::SetCursorScreenPos((glm::vec2)drawing_area_bb.GetTL() + offset);
    ImGui::Text("Opacity");
    win->DrawList->PathLineTo((glm::vec2)drawing_area_bb.Min - glm::vec2(0, padding_draw_area - 10));
    win->DrawList->PathLineTo(graph_bot_left);
    win->DrawList->PathStroke(IM_COL32_WHITE, false, 2.0f);

    //Draw arrowhead at the end of Y-axis
    p1 = glm::vec2((glm::vec2)drawing_area_bb.Min - glm::vec2(0, padding_draw_area - 10));
    p2 = glm::vec2(p1.x - 6, p1.y + 12);
    p3 = glm::vec2(p1.x + 6, p1.y + 12);
    win->DrawList->AddTriangleFilled(p1, p2, p3, IM_COL32_WHITE);

    //Draw Scale on Y-axis and also draw horizontal grid lines
    for(int i = 1; i <= 5; i++)
    {
        float val = i * 0.2;

        glm::vec2 abs_mpos = getAbsMousePositionFromPoint(glm::vec2(0.0, val));
        float text_width = 36.0;
        if(i != 5)
        {
            win->DrawList->PathLineTo(abs_mpos);
            win->DrawList->PathLineTo(abs_mpos - glm::vec2(6.0, 0.0));
            win->DrawList->PathStroke(IM_COL32_WHITE, false, 0.3f);

            win->DrawList->PathLineTo(abs_mpos);
            win->DrawList->PathLineTo(abs_mpos + glm::vec2(drawing_area_bb.GetWidth(), 0));
            win->DrawList->PathStroke(ImGui::GetColorU32(glm::vec4(0.7, 0.7, 0.7, 0.5)), false, 0.3f);
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
    win->DrawList->PathStroke(ImGui::GetColorU32(glm::vec4(0.7, 0.7, 0.7, 0.5)), false, 0.4f);


}

