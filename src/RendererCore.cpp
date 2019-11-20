#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdint>

#include "glad/glad.h"
#include "RendererCore.h"
#include "pvm2raw.h"
#include "stb_image_write.h"

RendererCore::RendererCore() : main_cam(30), histogram(256,0.0f)
{
    voxel_size = glm::vec3(1.0f, 1.0f, 1.0f);
    tex3D_dim = glm::vec3(0, 0, 0);
    cs_ID = cs_programID = 0;
    alpha_scale = 1;
    min_val = 0;
    max_val = 0;
    datasize_bytes = -1;
    kerneltime_sum = 0.0;
    camera_ubo_ID = 0;
    workgroups_x = workgroups_y = 0;
    use_mip = rotate_to_bottom = rotate_to_top = false;
}

RendererCore::~RendererCore()
{
    if(cs_programID)
        glDeleteProgram(cs_programID);
}

void RendererCore::setup()
{
    setupFBO();
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_ID);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);

    //Setup a texture and load data later..
    glGenTextures(1, &vol_tex3D);
}

bool RendererCore::checkRawInfFile(std::string fn)
{
    std::ifstream inf_file;
    inf_file.open(fn + ".inf");
    if(inf_file)
        return true;
    else
        return false;
}

void RendererCore::setAlpha()
{
    if(cs_programID)
        glUniform1f(0, alpha_scale);
}

void RendererCore::setMinVal()
{
    if(cs_programID)
    {
        if(datasize_bytes == 2)
            glUniform1i(2, min_val+1000);
        else
            glUniform1i(2, min_val);
    }
}

void RendererCore::setMaxVal()
{
    if(cs_programID)
    {
        if(datasize_bytes == 2)
            glUniform1i(3, max_val+1000);
        else
            glUniform1i(3, max_val);
    }
}

void RendererCore::setMIP()
{
    if(cs_programID)
        glUniform1i(4, (use_mip) ? 1 : 0);
}

void RendererCore::setInitialCameraRotation()
{
    if(cs_programID)
    {
        main_cam.resetCamera();
        glUniform1i(5, (rotate_to_top) ? 1 : 0);
        glUniform1i(6, (rotate_to_bottom) ? 1 : 0);
    }
}

void RendererCore::setUniforms()
{
    if(cs_programID)
        glUniform3f(1, voxel_size.x, voxel_size.y, voxel_size.z);
    setAlpha();
    setMinVal();
    setMaxVal();
    setMIP();
    setInitialCameraRotation();

}

bool RendererCore::loadShader(std::string fn, bool reload)
{
    if(createShader(fn, reload))
    {
        if(createShaderProgram())
        {
            // get Total Workgroup count
            int workgroup_size[3];
            glGetProgramiv(cs_programID, GL_COMPUTE_WORK_GROUP_SIZE, workgroup_size);
            workgroups_x = window_size.x / workgroup_size[0];
            workgroups_y = window_size.y / workgroup_size[1];

            glUseProgram(cs_programID);
            glUniform1f(0, alpha_scale);
            if(!loaded_dataset.empty())
            {
                setUniforms();
                main_cam.resetCamera();
            }
            return true;
        }
    }
    loaded_shader.clear();
    return false;
}

void RendererCore::render()
{
    GLuint64 elapsed_time = 0;
    GLuint query;
    glGenQueries(1, &query);

    if(main_cam.is_changed)
        setupUBO(true);

    glBindImageTexture(0, fbo_texID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glBeginQuery(GL_TIME_ELAPSED, query);
    glDispatchCompute(workgroups_x, workgroups_y, 1);
    glEndQuery(GL_TIME_ELAPSED);
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
    kerneltime_sum += (double) elapsed_time/1000000.0;

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glBindImageTexture(0, 0, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glBlitFramebuffer(0, 0, framebuffer_size.x, framebuffer_size.y,
                      0, 0, framebuffer_size.x, framebuffer_size.y,
                      GL_COLOR_BUFFER_BIT,
                      GL_LINEAR
                      );
}

bool RendererCore::saveImage(std::string fn, std::string ext)
{
    int stride = (framebuffer_size.x % 4) + (framebuffer_size.x * 3);
    unsigned char* img_data = new unsigned char[stride  * framebuffer_size.y * 3];
    bool status = false;

    glReadPixels(0, 0, framebuffer_size.x, framebuffer_size.y, GL_RGB, GL_UNSIGNED_BYTE, img_data);
    stbi_flip_vertically_on_write(1);
    if(ext == ".png")
        status = stbi_write_png(fn.c_str(), framebuffer_size.x, framebuffer_size.y, 3, img_data, stride);
    else if(ext == ".jpg")
        status = stbi_write_jpg(fn.c_str(), framebuffer_size.x, framebuffer_size.y, 3, img_data, 100);
    else if(ext == ".bmp")
        status = stbi_write_bmp(fn.c_str(), framebuffer_size.x, framebuffer_size.y, 3, img_data);

    delete[] img_data;
    return status;
}

void RendererCore::setupFBO()
{
    glGenFramebuffers(1,&fbo_ID);
    glBindFramebuffer(GL_FRAMEBUFFER,fbo_ID);

    glGenTextures(1, &fbo_texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fbo_texID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, framebuffer_size.x, framebuffer_size.y, 0, GL_RGBA, GL_FLOAT,0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texID, 0);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        if(status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT)
            throw std::runtime_error("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
        else if(status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT)
            throw std::runtime_error("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
        else if(status == GL_FRAMEBUFFER_UNDEFINED)
            throw std::runtime_error("Framebuffer not complete. Error code: GL_FRAMEBUFFER_UNDEFINED");
        else if(status == GL_FRAMEBUFFER_UNSUPPORTED)
            throw std::runtime_error("Framebuffer not complete. Error code: GL_FRAMEBUFFER_UNSUPPORTED");
        else if(status == GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER)
            throw std::runtime_error("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        else if(status == GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER)
            throw std::runtime_error("Framebuffer not complete. Error code: GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER");
        else
            throw std::runtime_error("Framebuffer not complete.");
    }
}

void RendererCore::setupUBO(bool is_update)
{
    std::vector<float> cam_data;
    cam_data.clear();
    main_cam.setUBO(cam_data);
    bool is_init = false;
    if(!is_update || camera_ubo_ID == 0)
        is_init = true;

    if(is_init)
        glGenBuffers(1, &camera_ubo_ID);

    glBindBuffer(GL_UNIFORM_BUFFER, camera_ubo_ID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float)*cam_data.size(), cam_data.data(), GL_DYNAMIC_DRAW);

    if(is_init)
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, camera_ubo_ID);

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void RendererCore::readVolumeData(std::string fn)
{
    std::string ext = fn.substr(fn.length()-3, 3);
    void* volume_data = NULL;

    if(ext == "raw")
    {
        std::ifstream inf_file;
        inf_file.open(fn + ".inf");
        if(inf_file)
        {
            std::string line = "";
            voxel_size = glm::vec3(0,0,0);
            tex3D_dim = glm::ivec3(0,0,0);
            while(getline(inf_file, line))
            {
                // skip empty lines
                if(line.empty())
                    continue;
                else if(line == "#dimensions")
                {
                    getline(inf_file, line);
                    if(line.empty())
                    {
                        msg = "Dimensions for Volume Data not provided in \"raw.inf\" file.";
                        title = "Invalid .raw.inf file!";
                        return;
                    }
                    std::stringstream ss(line);
                    ss >> tex3D_dim.x;
                    ss >> tex3D_dim.y;
                    ss >> tex3D_dim.z;
                }
                else if(line == "#voxel-spacing")
                {
                    getline(inf_file, line);
                    if(line.empty())
                    {
                        msg = "Aspect Ratio for Volume Data not provided in \"raw.inf\" file.";
                        title = "Invalid .raw.inf file!";
                        return;
                    }
                    std::stringstream ss(line);
                    ss >> voxel_size.x;
                    ss >> voxel_size.y;
                    ss >> voxel_size.z;
                }
            }
            if(tex3D_dim == glm::ivec3(0,0,0))
            {
                msg = "Dimensions for Volume Data not provided in \"raw.inf\" file. Make sure the header is \"#dimesnsions\"";
                title = "Invalid .raw.inf file!";
                return;
            }

            if(voxel_size == glm::vec3(0,0,0))
            {
                msg = "Aspect Ratio for Volume Data not provided in \"raw.inf\" file. Make sure the header is \"#voxel-spacing\"";
                title = "Invalid .raw.inf file!";
                return;
            }
        }
        else
        {
            //If no .raw.inf file found write one, using user provided parameters.
            std::ofstream oinf_file;
            oinf_file.open(fn + ".inf");
            if(oinf_file)
            {
                oinf_file << "#dimensions\n" << tex3D_dim.x << " "
                     << tex3D_dim.y << " " << tex3D_dim.z << "\n\n"

                     << "#voxel-spacing\n" << voxel_size.x << " "
                     << voxel_size.y << " " << voxel_size.z << std::endl;
            }
        }
        std::ifstream raw_file;
        raw_file.open(fn, std::ios::binary);

        if(!raw_file)
        {
            msg = "Failed to Open RAW file...";
            title = "Error!";
            return;
        }
        int len = tex3D_dim.x * tex3D_dim.y * tex3D_dim.z;

        if(len == 0)
        {
            msg = "Texture Dimensions shouldn't contain any zeroes. Please provide a valid .raw.inf file.";
            title = "Invalid Data Size!";
            return;
        }

        //Read RAW file into byte array;
        if(datasize_bytes == 1)
            volume_data = (uint8_t*) new uint8_t[len]();
        else
            volume_data = (uint16_t*) new uint16_t[len]();
        raw_file.read((char*)volume_data, len * datasize_bytes);
    }
    else
    {
        unsigned int components = -1;
        glm::uvec3 dims(0, 0, 0);
        volume_data = readPVMvolume(fn.c_str(), &dims.x, &dims.y, &dims.z, &components, &voxel_size.x, &voxel_size.y, &voxel_size.z);
        tex3D_dim = glm::ivec3(dims.x, dims.y, dims.z);
        if(!volume_data)
        {
            msg = "Error reading PVM file";
            title = "Error!";
            return;
        }
    }

    std::cout << "Dataset dimensions: " << tex3D_dim.x << ", " << tex3D_dim.y << ", " << tex3D_dim.z << std::endl;
    std::cout << "Dataset Aspect ratio: " << voxel_size.x << ", " << voxel_size.y << ", " << voxel_size.z << std::endl;

    //Calculate Histogram
    int max_value = -1, min_value = 9000000, len = tex3D_dim.x * tex3D_dim.y * tex3D_dim.z;
    if(datasize_bytes == 2)
    {
        for(int i = 0; i < len; i++)
        {
            if(i == 8390640)
                continue;
           uint16_t val = (((uint16_t*)(volume_data))[i]);

            if(val > max_value)
                max_value = val;
            if(val < min_value)
                min_value = val;
        }
        max_val = max_value;
        max_dataset_val = max_value;
        min_val = min_value;
        min_dataset_val = min_value;
    }
    else
    {
        min_val = min_dataset_val = 0;
        max_val = max_dataset_val = 255;
    }

    for(int i = 0; i < len; i++)
    {
        uint16_t val = 0;
        if(datasize_bytes == 1)
            val = (((uint8_t*)(volume_data))[i]);
        else
        {
            val = (((uint16_t*)(volume_data))[i]);
            val = std::round(val * 255.0f/max_dataset_val);
        }
        if(val == 0)
            continue;
        histogram[val]++;

        if(histogram[val] > max_value)
            max_value = histogram[val];
    }

    for(int i = 0; i < histogram.size(); i++)
        histogram[i] = histogram[i] * 100.0f / max_value;

    //Upload data from array to 3D texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, vol_tex3D);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    if(tex3D_dim.x % 4 != 0)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, (datasize_bytes == 1) ? GL_R8UI : GL_R16UI, tex3D_dim.x, tex3D_dim.y, tex3D_dim.z, 0, GL_RED_INTEGER, (datasize_bytes == 1) ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT, volume_data);
    if(ext == "pvm")
    {
        if(datasize_bytes == 1)
            free((uint8_t*)volume_data);
        else
            free((uint16_t*)volume_data);
    }
    else
    {
        if(datasize_bytes == 1)
            delete[] (uint8_t*)volume_data;
        else
            delete[] (uint16_t*)volume_data;
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    title = "File Loaded!";
    msg = "File Loaded Successfully!";

    if(!loaded_shader.empty())
    {
        setUniforms();
        main_cam.resetCamera();
    }

    int idx = fn.find_last_of("/");
    loaded_dataset =  fn.substr(idx+1, fn.length() - idx);
}

bool RendererCore::createShader(std::string fn, bool reload)
{
    std::string shader_data = "";
    std::streamoff len;
    std::ifstream file;

    if(reload)
        fn = loaded_shader;

    file.open(fn, std::ios::binary);
    if(!file.is_open())
    {
        msg = "Failed to Open Shader file.";
        title = "Error!";
        return false;
    }

    file.seekg(0, std::ios::end);
    len = file.tellg();
    file.seekg(0, std::ios::beg);

    shader_data.resize(len);
    file.read(&shader_data[0], len);

    const GLchar* source = (const GLchar *) shader_data.c_str();

    cs_ID = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(cs_ID, 1, &source,0);
    glCompileShader(cs_ID);

    GLint isCompiled = 0;
    glGetShaderiv(cs_ID, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(cs_ID, GL_INFO_LOG_LENGTH, &max_length);
        std::vector<GLchar> infoLog(max_length);
        glGetShaderInfoLog(cs_ID, max_length, &max_length, &infoLog[0]);

        glDeleteShader(cs_ID);
        std::string err_log(infoLog.begin(), infoLog.end());
        std::cout << "\n" << err_log << std::endl;

        msg = "Failed to Compile Shader, detailed log is printed in console.";
        title = "Shader Error!";
        return false;
    }
    int idx = fn.find_last_of("/");
    loaded_shader = fn.substr(idx+1, fn.length() - idx);
    return true;
}

bool RendererCore::createShaderProgram()
{
    if(!cs_programID)
        cs_programID = glCreateProgram();
    glAttachShader(cs_programID, cs_ID);

    glLinkProgram(cs_programID);
    glDetachShader(cs_programID, cs_ID);
    glDeleteShader(cs_ID);

    GLint isLinked = 0;
    glGetProgramiv(cs_programID, GL_LINK_STATUS, &isLinked);
    if(isLinked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetProgramiv(cs_programID, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> infoLog(max_length);
        glGetProgramInfoLog(cs_programID, max_length, &max_length, &infoLog[0]);

        glDeleteProgram(cs_programID);
        cs_programID = 0;

        std::string err_log(infoLog.begin(), infoLog.end());
        std::cout << "\n" << err_log << std::endl;

        msg = "Failed to Link Program Object, detailed log is printed in console.";
        title = "Shader Error!";
        return false;
    }

    title = "Shader Loaded!";
    msg = "Shader Loaded Successfully!";
    return true;
}

