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

#include <sstream>

#include "VolumeRenderer.h"
#include "RuntimeError.h"
#include "GLroutines.h"
#include "pvm2raw.h"

#include <functional>

VolumeRenderer::VolumeRenderer() : main_cam(30)
{
    voxel_size = glm::vec3(1.0f, 1.0f, 1.0f);
    //ctor
}

VolumeRenderer::~VolumeRenderer()
{
    //dtor
}

void VolumeRenderer::setup()
{
    glfw_manager.createWindow(640, 480, "Volume Renderer");
    glfw_manager.setCameraUpdateCallback(std::bind(&(main_cam.setOrientation), &main_cam,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3)
                                        );
    readVolumeData();
    setupFBO();
    setupUBO();
    createShader(cs_ID, "VolumeRenderer.cs", GL_COMPUTE_SHADER);
    createShaderProgram(cs_programID, cs_ID);

}

void VolumeRenderer::start()
{
    // get Total Workgroup count
    int workgroup_size[3];
    glGetProgramiv(cs_programID, GL_COMPUTE_WORK_GROUP_SIZE, workgroup_size);
    int no_x = glfw_manager.window_width / workgroup_size[0] ;
    int no_y = glfw_manager.window_height / workgroup_size[1] ;

    std::cout << "\nNo. of workgroups in X: " << no_x << "\n"
              << "No. of workgroups in Y: " << no_y << std::endl;

    GLuint64 elapsed_time = 0;
    GLuint query;
    glGenQueries(1, &query);

    glUseProgram(cs_programID);
    GLuint uniform_location = glGetUniformLocation(cs_programID, "voxel_size");
    glUniform3f(uniform_location, voxel_size.x, voxel_size.y, voxel_size.z);

    glfw_manager.focusWindow();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_ID);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    while(!glfwWindowShouldClose(glfw_manager.window))
    {
        if(main_cam.is_changed)
        {
            setupUBO(true);
        }
        glBindImageTexture(0, fbo_texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glBeginQuery(GL_TIME_ELAPSED, query);
        glDispatchCompute(no_x, no_y, 1);
        glEndQuery(GL_TIME_ELAPSED);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

        glClear(GL_COLOR_BUFFER_BIT);
        glBlitFramebuffer(0, 0, glfw_manager.framebuffer_width, glfw_manager.framebuffer_height,
                          0, 0, glfw_manager.framebuffer_width, glfw_manager.framebuffer_height,
                          GL_COLOR_BUFFER_BIT,
                          GL_LINEAR
                          );

        glfwSwapBuffers(glfw_manager.window);
        glfwPollEvents();
    }
}

void VolumeRenderer::setupFBO()
{
    glGenFramebuffers(1,&fbo_ID);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo_ID);

    glGenTextures(1, &fbo_texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo_texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, glfw_manager.framebuffer_width, glfw_manager.framebuffer_height, 0, GL_RGBA, GL_FLOAT,0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texID, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        if(status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            throw RuntimeError("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
        else if(status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            throw RuntimeError("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
        else if(status == GL_FRAMEBUFFER_UNDEFINED)
            throw RuntimeError("Framebuffer not complete. Error code: GL_FRAMEBUFFER_UNDEFINED");
        else if(status == GL_FRAMEBUFFER_UNSUPPORTED)
            throw RuntimeError("Framebuffer not complete. Error code: GL_FRAMEBUFFER_UNSUPPORTED");
        else if(status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
            throw RuntimeError("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        else if(status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
            throw RuntimeError("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        else
            throw RuntimeError("Framebuffer not complete.");
    }
}

void VolumeRenderer::setupUBO(bool is_update)
{
    std::vector<float> cam_data;
    cam_data.clear();
    main_cam.setUBO(cam_data);

    if(!is_update)
        glGenBuffers(1, &camera_ubo_ID);

    glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo_ID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float)*cam_data.size(), cam_data.data(), GL_DYNAMIC_DRAW);

    if(!is_update)
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, camera_ubo_ID);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}
void VolumeRenderer::readVolumeData()
{
    std::string fn = "settings.txt", line = "", pvm_fn, raw_fn;
    glm::vec3 tex3D_dim(0,0,0);

    std::ifstream settings_file;
    settings_file.open(fn);

    if(settings_file)
    {
        while(getline(settings_file, line))
        {
            // skip comments and empty lines
            if(line[0] == '#' || line.empty())
                continue;

            if(line == "pvm")
            {
                getline(settings_file, pvm_fn);
                if(pvm_fn.empty())
                    throw RuntimeError("PVM file name not provided in \"settings.txt\"");

                //If PVM file provided no need to check for other options.
                break;
            }
            else if (line == "raw")
            {
                getline(settings_file, raw_fn);
                if(raw_fn.empty())
                    throw RuntimeError("RAW file name not provided in \"settings.txt\"");
            }
            else if(line == "dimensions")
            {
                getline(settings_file, line);
                if(line.empty())
                    throw RuntimeError("Dimensions for Volume Data not provided in \"settings.txt\"");
                std::stringstream ss(line);
                ss >> tex3D_dim.x;
                ss >> tex3D_dim.y;
                ss >> tex3D_dim.z;
            }
            else if(line == "aspect-ratio")
            {
                getline(settings_file, line);
                if(line.empty())
                    throw RuntimeError("Aspect Ratio for Volume Data not provided in \"settings.txt\"");
                std::stringstream ss(line);
                ss >> voxel_size.x;
                ss >> voxel_size.y;
                ss >> voxel_size.z;
            }
        }
    }

    if(!pvm_fn.empty())
    {
        raw_fn = std::string(pvm_fn.begin(), pvm_fn.end()-4);
        if( !pvm2raw(pvm_fn.c_str(), (raw_fn + ".raw").c_str(), voxel_size, tex3D_dim) )
            throw RuntimeError("Error reading Data or Opening PVM file...\n");

        //Generate a RAW information file which stores the Raw file's dimensions and aspect ratio.
        std::ofstream inf_file;
        inf_file.open(raw_fn + ".raw.inf");
        inf_file << "#dimensions\n" << tex3D_dim.x << " "
                 << tex3D_dim.y << " " << tex3D_dim.z << "\n\n"

                 << "#aspect-ratio\n" << voxel_size.x << " "
                 << voxel_size.y << " " << voxel_size.z << std::endl;

        raw_fn += ".raw";
    }

    std::ifstream raw_file;
    raw_file.open(raw_fn, std::ios::binary);

    if(!raw_file)
        throw RuntimeError("Failed to Open file...", __FILE__, std::to_string(__LINE__) );
    int len = tex3D_dim.x * tex3D_dim.y * tex3D_dim.z;

    if(len == 0)
    {
        raw_file.seekg(0, std::ios::end);
        len = raw_file.tellg();
        raw_file.seekg(0, std::ios::beg);
    }

    //Read RAW file into byte array;
    GLubyte* volume_data = new GLubyte[len];
    raw_file.read((char*)volume_data, len);

    //Clear first and last slices to 0 so we don't get garbage values
    int length = tex3D_dim.x * tex3D_dim.y;
    int start = (tex3D_dim.x * tex3D_dim.y) * (tex3D_dim.z - 1);

    for(int i = 0; i < length; i++)
        volume_data[i] = 0;

    length *= tex3D_dim.z;
    for(int i = start; i < length; i++)
        volume_data[i] = 0;


    /*int start = (tex3D_dim.x * tex3D_dim.y) * (tex3D_dim.z - 230);
    for(int i = start; i < len; i++)
    {
        volume_data[i] = 0;
    }*/

    //Upload data from array to 3D texture
    glGenTextures(1, &vol_tex3D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, vol_tex3D);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if( ((int)tex3D_dim.x % 4) != 0)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, tex3D_dim.x, tex3D_dim.y, tex3D_dim.z, 0, GL_RED, GL_UNSIGNED_BYTE, volume_data);

    delete [] volume_data;
}
