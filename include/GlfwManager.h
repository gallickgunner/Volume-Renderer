/******************************************************************************
 *  Copyright (C) 2019 by Umair Ahmed.
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

#ifndef GLFWMANAGER_H
#define GLFWMANAGER_H

#include <string>
#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/vec4.hpp"
#include <functional>

class GlfwManager
{
    public:
        GlfwManager(int width, int height, std::string title, bool is_fullscreen = false);
        ~GlfwManager();
        void createWindow(int width, int height, std::string title, bool is_fullscreen = false);
        void focusWindow();
        void setCameraUpdateCallback(std::function<void(float, float, float)> cb);

        GLFWwindow* window;
        int window_width, window_height, framebuffer_width, framebuffer_height;

    private:
        void initImGui();
        static std::function<void(float, float, float)> cameraUpdateCallback;

        static void errorCallback(int error, const char* msg);
        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPosCallback(GLFWwindow* window, double new_cursor_x, double new_cursor_y);
        static void mouseScrollCallback(GLFWwindow* window, double x_offset, double y_offset);

        GLFWmonitor* prim_monitor;
        const GLFWvidmode* mode;
        bool mouse_button_pressed;
        double old_cursor_x, old_cursor_y;
};

#endif // GLFWMANAGER_H
