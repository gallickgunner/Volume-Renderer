#include "RendererGUI.h"
#include "glm/vec2.hpp"
#include <functional>

RendererGUI::RendererGUI(int window_width, int window_height, std::string title, bool is_fullscreen) :
    glfw_manager(window_width, window_height, title, is_fullscreen)
{
    profiler_shown = true;
    tools_shown = false;
    histogram_shown = false;
    HU_scale_shown = false;
    renderer_start = false;
    mspf = mspk = 0.0f;
    profiler_wheight = tools_wheight = 0;
}

RendererGUI::~RendererGUI()
{
    //dtor
}

void RendererGUI::startFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void RendererGUI::renderFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void RendererGUI::run()
{
    volren.window_size = glm::vec2(glfw_manager.window_width, glfw_manager.window_height);
    volren.framebuffer_size = glm::vec2(glfw_manager.framebuffer_width, glfw_manager.framebuffer_height);
    volren.setup();

    glfw_manager.setCameraUpdateCallback(std::bind(&(volren.main_cam.setOrientation), &volren.main_cam,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3)
                                        );
    glfw_manager.focusWindow();

    int frame_count = 0;
    double prev_time = glfwGetTime(), prev_frame_time = 0, skip_ticks = 16.66666;

    volren.loadShader("VolumeRenderer.cs", false);
    while(!glfwWindowShouldClose(glfw_manager.window))
    {
        //Show ms per frame and per kernel averaged over 1 sec intervals...
        if(glfwGetTime() - prev_time >= 1.0)
        {
            mspf = (glfwGetTime() - prev_time) * 1000/frame_count;
            mspk = (float) volren.kerneltime_sum / frame_count;
            frame_count = 0;
            volren.kerneltime_sum = 0;
            prev_time = glfwGetTime();
        }

        //Lock frame rate to 60 fps
        if( (glfwGetTime() - prev_frame_time) * 1000 <= skip_ticks)
            continue;

        frame_count++;
        prev_frame_time = glfwGetTime();

        //Start Drawing a new frame
        glClear(GL_COLOR_BUFFER_BIT);

        startFrame();
        showMenu();

        if(histogram_shown)
            showHistogram();

        if(profiler_shown)
            showProfiler();

        if(tools_shown)
            showTools();

        if(HU_scale_shown)
            showHounsfieldScale();

        if(!volren.title.empty() && !volren.msg.empty())
        {
            error_title = volren.title;
            error_msg = volren.msg;
            volren.title.clear();
            volren.msg.clear();
            ImGui::OpenPopup(error_title.c_str());
        }
        showMessageBox(error_title, error_msg);

        if(renderer_start)
            volren.render();

        renderFrame();
        glfwSwapBuffers(glfw_manager.window);
        glfwPollEvents();
    }
}

void RendererGUI::enableToolsGUI()
{
    renderer_start = true;
    HU_scale_shown = true;
    histogram_shown = true;
    tools_shown = true;
}

void RendererGUI::showMenu()
{
    bool open_filedialog = false, save_fildialog = false, open_shaderdialog = false, open_inf_panel = false, open_about = false, open_usage = false, show_error = false;
    if(ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Load PVM/RAW"))
            {
                if(ImGui::MenuItem("UINT8", NULL))
                {
                    volren.datasize_bytes = 1;
                    open_filedialog = true;
                }

                if(ImGui::MenuItem("UINT16", NULL))
                {
                    volren.datasize_bytes = 2;
                    open_filedialog = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Load Shader", NULL))
                open_shaderdialog = true;

            if(ImGui::MenuItem("Reload Shader", NULL, false, !volren.loaded_shader.empty()))
                volren.loadShader("", true);

            if(ImGui::MenuItem("Save PNG/JPG", NULL, false, renderer_start))
                save_fildialog = true;

            ImGui::Separator();
            ImGui::MenuItem("Start", NULL, &renderer_start, (!volren.loaded_shader.empty() && !volren.loaded_dataset.empty()));
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            if(ImGui::MenuItem("Profiler", NULL, &profiler_shown))
            {
                if(!profiler_shown)
                    profiler_wheight = 25;
            }
            ImGui::MenuItem("IsoValue Histogram", NULL, &histogram_shown);
            if(ImGui::MenuItem("Tools", NULL, &tools_shown, (!volren.loaded_shader.empty() && !volren.loaded_dataset.empty())))
            {
                if(!tools_shown)
                    tools_wheight = 0;
            }
            ImGui::MenuItem("Windowing", NULL, &HU_scale_shown, (!volren.loaded_shader.empty() && !volren.loaded_dataset.empty()));
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if(ImGui::MenuItem("About"))
                open_about = true;
            if(ImGui::MenuItem("Usage"))
                open_usage = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if(open_filedialog)
        ImGui::OpenPopup("Open RAW/PVM File");

    if(open_shaderdialog)
        ImGui::OpenPopup("Open Compute Shader File");

    if(save_fildialog)
        ImGui::OpenPopup("Save Image");

    if(file_dialog.showOpenFileDialog("Open RAW/PVM File", ImVec2(700, 310), ".raw,.pvm"))
    {
        std::string ext = file_dialog.selected_fn.substr(file_dialog.selected_fn.length()-3, 3);

        //If pvm file was loaded or if raw file was loaded and raw.inf was present call readVolumeData immediately. Else ask user for information regarding data.
        if(ext == "pvm" || volren.checkRawInfFile(file_dialog.selected_fn))
            volren.readVolumeData(file_dialog.selected_fn);
        else
            open_inf_panel = true;

        if(!volren.loaded_shader.empty() && !open_inf_panel)
            enableToolsGUI();
    }

    if(open_inf_panel)
        ImGui::OpenPopup("Enter Information for Raw File");
    if(showRawInfPanel())
    {
        volren.readVolumeData(file_dialog.selected_fn);
        if(!volren.loaded_shader.empty())
            enableToolsGUI();
    }

    if(file_dialog.showOpenFileDialog("Open Compute Shader File", ImVec2(700, 310), ".cs"))
    {
        volren.loadShader(file_dialog.selected_fn, false);
        if(!volren.loaded_dataset.empty())
            enableToolsGUI();
    }

    if(file_dialog.showSaveFileDialog("Save Image", ImVec2(700, 310), ".png,.jpg,.bmp"))
        show_error = !(volren.saveImage(file_dialog.selected_fn, file_dialog.ext));

    if(show_error)
        ImGui::OpenPopup("Error saving Image!");
    showMessageBox("Error saving Image!", "There was an error saving the image. Make sure the filename provided doesn't contain any invalid characters.");

    if(open_about)
        ImGui::OpenPopup("About");
    showMessageBox("About", "Volume Renderer is a personal project for viewing 3D volume datasets mainly CT Scans and MRI Images. It uses GPU based raymarching. Currently It reads only RAW/PVM files.");

    if(open_usage)
        ImGui::OpenPopup("Usage");
    showMessageBox("Usage", "Load a RAW/PVM file from Settings menu. If RAW file is loaded a \".raw.inf\" file with the same name must be present in the same directory. "
                   "If not present, the user will be asked once for the information regarding the dataset dimensions and aspect ratio.\n\nThe Histogram menu item "
                   "displays an IsoValue Histogram which is currently non-editable. You can manually set the control points in the compute shader using it as a reference. "
                   "The Tools item presents certain handy properties like alpha scaling and resetting camera. The windowing presents a slider used for viewing certain range of values."
                   "Value below min value are rendered black and values above max are rendered white.");

}

void RendererGUI::showMessageBox(std::string title, std::string msg)
{
    ImVec2 text_size = ImGui::CalcTextSize(msg.c_str(), NULL, true, 360);
    ImGui::SetNextWindowSize(ImVec2(380, 0));
    if (ImGui::BeginPopupModal(title.c_str(), NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 340);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetWindowSize().x - text_size.x) / 2));
        ImGui::TextWrapped(msg.c_str());
        ImGui::PopTextWrapPos();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);

        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::SetCursorPosX(380/2.0 - 25);
        if (ImGui::Button("Ok", ImVec2(50, 0)))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void RendererGUI::showProfiler()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_pos(io.DisplaySize.x - 10, 35);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1,0));
    ImGui::SetNextWindowSize(ImVec2(270, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.25,0.25,0.25,0.35));
    if (ImGui::Begin("Profiler##window", &profiler_shown, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
        ImGui::Text("Workgroups in X");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::Text(": %d", volren.workgroups_x);

        ImGui::Text("Workgroups in Y");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::Text(": %d", volren.workgroups_y);

        ImGui::Text("Dataset");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::Text(": ");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x);
        ImGui::TextWrapped("%s", volren.loaded_dataset.c_str());

        ImGui::Text("Shader");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::Text(": ");
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - ImGui::GetStyle().ItemSpacing.x);
        ImGui::TextWrapped("%s", volren.loaded_shader.c_str());

        ImGui::Text("ms/frame (capped)");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::Text(": %.2f ms", mspf);

        ImGui::Text("ms/kernel");
        ImGui::SameLine();
        ImGui::SetCursorPosX(140);
        ImGui::Text(": %.2f ms", mspk);
        profiler_wheight = 35 + ImGui::GetWindowHeight();
        ImGui::End();
    }
    ImGui::PopStyleColor();
}

void RendererGUI::showHistogram()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(10,35), ImGuiCond_Once, ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 300, 0), ImGuiCond_Once);
    ImGui::Begin("IsoValue Histogram##window");
    ImGui::PlotHistogram("IsoValue Histogram", volren.histogram.data(), volren.histogram.size(), 0, NULL, 0.0f, 100.0f, ImVec2(ImGui::GetWindowSize().x -200,180));
    ImGui::End();
}

void RendererGUI::showTools()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_pos(io.DisplaySize.x - 10, profiler_wheight + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1,0));
    ImGui::SetNextWindowSize(ImVec2(270, 0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.25,0.25,0.25,0.35));
    if (ImGui::Begin("Tools##window", &tools_shown, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
        ImGui::PushItemWidth(130);
        if(ImGui::SliderFloat("Alpha Scale", &volren.alpha_scale, 0.0f, 1.0f, "%.3f"))
            volren.setAlpha();
        ImGui::SameLine();
        showHelpMarker("Use Ctrl+Click to input value.");
        ImGui::PopItemWidth();

        if(ImGui::Checkbox("MIP", &volren.use_mip))
            volren.setMIP();
        ImGui::SameLine();
        showHelpMarker("Check to use Maximum Intensity Projection.");

        if(ImGui::Checkbox("View Top", &volren.rotate_to_top))
        {
            volren.rotate_to_bottom = false;
            volren.setInitialCameraRotation();
        }
        ImGui::SameLine();

        if(ImGui::Checkbox("View Bottom", &volren.rotate_to_bottom))
        {
            volren.rotate_to_top = false;
            volren.setInitialCameraRotation();
        }
        ImGui::SameLine();
        showHelpMarker("Since the Camera doesn't allow viewing sideways after rotating 90 degrees in Y. Use this to set initial rotation.");

        if(ImGui::Button("Reset Camera"))
            volren.main_cam.resetCamera();

        tools_wheight = 10 + ImGui::GetWindowHeight();
        ImGui::End();
    }
    ImGui::PopStyleColor();
}

void RendererGUI::showHounsfieldScale()
{
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 window_pos(io.DisplaySize.x - 10, profiler_wheight + tools_wheight + 10);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(1,0));
    ImGui::SetNextWindowSize(ImVec2(270, 0));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.25,0.25,0.25,0.35));
    if (ImGui::Begin("Windowing Parameters", &HU_scale_shown, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
    {
        ImGui::SetCursorPosX(25);
        if(ImGui::DragIntRange2("##Range", &volren.min_val, &volren.max_val, 0.5, volren.min_dataset_val, (volren.datasize_bytes == 2) ? volren.max_dataset_val - 1000 : volren.max_dataset_val, "Min: %d", "Max: %d"))
        {
            volren.setMinVal();
            volren.setMaxVal();
        }
        ImGui::SameLine();
        showHelpMarker("Use this to view a certain range of values. For 16 bit data the values recorded are probably in Hounsfield Units. Use the below table as reference for setting the range.");

        ImGui::Separator();

        ImVec2 text_size = ImGui::CalcTextSize("Hounsfield Scale", NULL, true, 270);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ((ImGui::GetWindowSize().x - text_size.x) / 2));
        ImGui::Text("Hounsfield Scale");

        ImGui::Text("Air"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("-1000");
        ImGui::Text("Lungs"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("-950 to -600");
        ImGui::Text("Fat"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("-100 to -80");
        ImGui::Text("Water"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("0");
        ImGui::Text("White matter"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+20 to +30");
        ImGui::Text("Grey matter"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+30 to +40");
        ImGui::Text("Muscle"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+10 to +40");
        ImGui::Text("Kidneys"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+20 to +40");
        ImGui::Text("Blood"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+50 to +60");
        ImGui::Text("Liver"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+50 to +70");
        ImGui::Text("Compact bone"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("+300 to +2500");
        ImGui::Text("Metals"); ImGui::SameLine(); ImGui::SetCursorPosX(140); ImGui::Text("> +2000");
        ImGui::End();
    }
    ImGui::PopStyleColor();
}

bool RendererGUI::showRawInfPanel()
{
    bool ret_val = false;
    //ImGui::SetNextWindowPos(ImVec2(.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f,0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 0));
    if(ImGui::BeginPopupModal("Enter Information for Raw File", NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("The application couldn't find or had problems in reading a \".raw.inf\" file with the same name as the file selected. Please manually provide the following parameters.");
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::InputInt3("Dimensions", &volren.tex3D_dim[0], ImGuiInputTextFlags_CharsDecimal);
        ImGui::InputFloat3("Voxel Spacing", &volren.voxel_size[0], "%.5g", ImGuiInputTextFlags_CharsDecimal);
        ImGui::Separator();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth()/2.0 - 25);
        if (ImGui::Button("Ok", ImVec2(50, 0)))
        {
            if(volren.tex3D_dim != glm::ivec3(0,0,0) && volren.voxel_size != glm::vec3(0,0,0))
            {
                ret_val = true;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }
    return ret_val;
}

void RendererGUI::showHelpMarker(std::string desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}


