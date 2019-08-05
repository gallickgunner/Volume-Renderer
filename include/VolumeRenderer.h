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

#ifndef VOLUMERENDERER_H
#define VOLUMERENDERER_H

#include "GlfwManager.h"
#include "glm/vec3.hpp"
#include "Camera.h"

class VolumeRenderer
{
    public:
        VolumeRenderer();
        ~VolumeRenderer();
        void setup();
        void start();

    private:
        void setupFBO();
        void setupUBO(bool is_update = false);
        void readVolumeData();

        GlfwManager glfw_manager;
        Camera main_cam;
        GLuint vol_tex3D, camera_ubo_ID, fbo_ID, fbo_texID, cs_ID, cs_programID;
        glm::vec3 voxel_size;

};

#endif // VOLUMERENDERER_H
