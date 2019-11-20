#include <iostream>
#include <exception>
#include "GlfwManager.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

std::function<void(float, float, float)> GlfwManager::cameraUpdateCallback;

GlfwManager::GlfwManager(int window_width, int window_height, std::string title, bool is_fullscreen)
{
    glfwSetErrorCallback(errorCallback);
    if(!glfwInit())
        throw std::runtime_error("GLFW failed to initialize.");

    window = NULL;
    mouse_button_pressed = false;
    createWindow(window_width, window_height, title, is_fullscreen);
    initImGui();
    //ctor
}

GlfwManager::~GlfwManager()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if(window)
            glfwDestroyWindow(window);
        glfwTerminate();
    //dtor
}

void GlfwManager::createWindow(int window_width, int window_height, std::string title, bool is_fullscreen)
{
    // get Primary Monitor and default Video Mode
    prim_monitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(prim_monitor);

    // Context Hints.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //FrameBuffer Hints.
    glfwWindowHint(GLFW_RED_BITS, 8);
    glfwWindowHint(GLFW_GREEN_BITS, 8);
    glfwWindowHint(GLFW_BLUE_BITS, 8);
    glfwWindowHint(GLFW_ALPHA_BITS, 8);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    //Window Hints
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    if(is_fullscreen)
    {
        this->window_width = mode->width;
        this->window_height = mode->height;
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        glfwWindowHint(GLFW_AUTO_ICONIFY, true);
        window = glfwCreateWindow(mode->width, mode->height, title.data(), prim_monitor, NULL);
    }
    else
    {
        this->window_width = window_width;
        this->window_height = window_height;
        window = glfwCreateWindow(window_width, window_height, title.data(), NULL, NULL);
    }

    if(!window)
    {
        glfwTerminate();
        throw std::runtime_error("Window creation failed..");
    }

    glfwMakeContextCurrent(window);
    if( !gladLoadGLLoader( (GLADloadproc) glfwGetProcAddress) )
        throw std::runtime_error("Couldn't Initialize GLAD..");
    if(!GLAD_GL_VERSION_4_3)
        throw std::runtime_error("OpenGL version 4.3 not supported");

    glGetError();
    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);

    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0,0, framebuffer_width, framebuffer_height);
    glfwIconifyWindow(window);
    glfwSwapInterval(0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void GlfwManager::initImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

     // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
}

void GlfwManager::focusWindow()
{
    glfwRestoreWindow(window);
    glfwFocusWindow(window);
}

void GlfwManager::setCameraUpdateCallback(std::function<void(float, float, float)> cb)
{
    GlfwManager::cameraUpdateCallback = cb;
}

void GlfwManager::errorCallback(int error, const char* msg)
{
    std::cout << msg << std::endl;
}

void GlfwManager::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
}

void GlfwManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard)
        return;
    GlfwManager* ptr = (GlfwManager*) glfwGetWindowUserPointer(window);
}

void GlfwManager::cursorPosCallback(GLFWwindow* window, double new_cursor_x, double new_cursor_y)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;
    GlfwManager* ptr = (GlfwManager*) glfwGetWindowUserPointer(window);
    if(ptr->mouse_button_pressed)
    {
        float delta_x, delta_y;
        float zenith = 0.06, azimuth = 0.06;

        delta_x = new_cursor_x - ptr->old_cursor_x;
        delta_y = new_cursor_y - ptr->old_cursor_y;

        if(delta_x < 1 && delta_x > -1)
            azimuth = 0;
        else if (delta_x > 0)
            azimuth *= -1.0;

        if(delta_y < 1 && delta_y > -1)
            zenith = 0;
        else if(delta_y > 0)
            zenith *= -1.0;

        GlfwManager::cameraUpdateCallback(0, zenith, azimuth);

        ptr->old_cursor_x = new_cursor_x;
        ptr->old_cursor_y = new_cursor_y;
    }
    else
    {
        ptr->old_cursor_x = new_cursor_x;
        ptr->old_cursor_y = new_cursor_y;
    }

}

void GlfwManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;
    GlfwManager* ptr = (GlfwManager*) glfwGetWindowUserPointer(window);
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        ptr->mouse_button_pressed = true;
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        ptr->mouse_button_pressed = false;
    }
}

void GlfwManager::mouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
        return;
    cameraUpdateCallback(y_offset, 0, 0);
}
