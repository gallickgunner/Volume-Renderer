#ifndef RENDERERGUI_H
#define RENDERERGUI_H

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

#include "GlfwManager.h"
#include "RendererCore.h"
#include "UI/TransferFunction.h"
#include "glm/vec3.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "ImGuiFileBrowser.h"

class RendererGUI
{
    public:
        RendererGUI(int window_width, int window_height, std::string title, bool is_fullscreen = false);
        ~RendererGUI();
        void run();

    private:
        void startFrame();
        void renderFrame();
        void showMenu();
        void showProfiler();
        void showHistogram();
        void showTools();
        void showHounsfieldScale();
        void enableToolsGUI();
        void showMessageBox(std::string title, std::string msg);
        void showHelpMarker(std::string desc);
        bool showRawInfPanel();


        GlfwManager glfw_manager;
        RendererCore volren;
        imgui_addons::ImGuiFileBrowser file_dialog;
        TransferFunction transfer_func;
        std::string error_msg, error_title;
        float mspf, mspk;
        int workgroups_x, workgroups_y, profiler_wheight, tools_wheight;
        bool profiler_shown, histogram_shown, tools_shown, HU_scale_shown, renderer_start;
};

#endif // RENDERERGUI_H
