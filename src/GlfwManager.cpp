/******************************************************************************
 *  Copyright (C) 2018 by Umair Ahmed.
 *
 *  This is a free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "GlfwManager.h"
#include "RuntimeError.h"

#include <iostream>

GlfwManager::GlfwManager()
{
    glfwSetErrorCallback(errorCallback);
    if(!glfwInit())
        throw RuntimeError("GLFW failed to initialize.");
    window = NULL;
    //ctor
}

GlfwManager::~GlfwManager()
{
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
        throw RuntimeError("Window creation failed..");
    }

    glfwMakeContextCurrent(window);
    if( !gladLoadGLLoader( (GLADloadproc) glfwGetProcAddress) )
        throw RuntimeError("Couldn't Initialize GLAD..");
    if(!GLAD_GL_VERSION_4_3)
        throw RuntimeError("OpenGL version 4.3 not supported");

    glGetError();
    glfwSetWindowUserPointer(window, this);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    glViewport(0,0, framebuffer_width, framebuffer_height);
    glfwIconifyWindow(window);
    glfwSwapInterval(0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glfwSwapBuffers(window);
    glfwPollEvents();
    glGetError();
}

void GlfwManager::focusWindow()
{
    glfwRestoreWindow(window);
    glfwFocusWindow(window);
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
    GlfwManager* ptr = (GlfwManager*) glfwGetWindowUserPointer(window);
}

void GlfwManager::cursorPosCallback(GLFWwindow* window, double new_cursor_x, double new_cursor_y)
{
    GlfwManager* ptr = (GlfwManager*) glfwGetWindowUserPointer(window);
    /*if(ptr->space_flag)
    {
        float delta_x, delta_y;
        float x_rad = 0.06, y_rad = 0.06;

        delta_x = new_cursor_x - ptr->old_cursor_x;
        delta_y = new_cursor_y - ptr->old_cursor_y;

        if(delta_x < 1 && delta_x > -1)
            y_rad = 0;
        else if (delta_x > 0)
            y_rad *= -1.0;

        if(delta_y < 1 && delta_y > -1)
            x_rad = 0;
        else if(delta_y > 0)
            x_rad *= -1.0;
        cameraUpdateCallback(Vec4f(0,0,0,0), x_rad, y_rad);
        ptr->old_cursor_x = new_cursor_x;
        ptr->old_cursor_y = new_cursor_y;
    }
    else
    {
        ptr->old_cursor_x = new_cursor_x;
        ptr->old_cursor_y = new_cursor_y;
    }
    gui->screen->cursorPosCallbackEvent(new_cursor_x, new_cursor_y);*/

}

void GlfwManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int modifiers)
{
}
