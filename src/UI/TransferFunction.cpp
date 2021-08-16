#include "TransferFunction.h"
#include "glm/vec2.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

TransferFunction::TransferFunction() : data_scale(DataScale::ADAPTIVE_SCALE), alpha_spline(0, 255)
{
    max_dataset_val = 255;
    min_medical_val = -1000;
    //ctor
}

TransferFunction::~TransferFunction()
{
    //dtor
}

void TransferFunction::render()
{
    ImGui::ShowDemoWindow();
    ImGui::Begin("Transfer Function");
    alpha_spline.render(300);
    //grad_bar.render(30, 220);
    ImGui::End();
}

int TransferFunction::convertValue(DataScale to_scale, int value)
{
    if(to_scale == data_scale)
        return value;

    if(data_scale == DataScale::NORMALIZED_SCALE)
    {
        if(to_scale == DataScale::MEDICAL_SCALE)
            return (((value / 255.0f) * max_dataset_val) - min_medical_val);
        else // adaptive sacale
            return (value / 255.0f) * max_dataset_val;
    }
    else if(data_scale == DataScale::MEDICAL_SCALE)
    {
        if(to_scale == DataScale::NORMALIZED_SCALE)
            return ((value + min_medical_val) / (float)max_dataset_val) * 255;
        else //adaptive scale
            return (value + min_medical_val);
    }
    else
    {
        if(data_scale == DataScale::NORMALIZED_SCALE)
            return (value / (float)max_dataset_val) * 255;
        else // medical scale
            return (value - min_medical_val);
    }
}
