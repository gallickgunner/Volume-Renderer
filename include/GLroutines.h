#ifndef GLROUTINES_H
#define GLROUTINES_H

#include "glad/glad.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <exception>


void createShader(GLuint& shaderID, const std::string& shader_fn, GLenum shaderType);
void createShaderProgram(GLuint& programID, std::vector<GLuint> shaderIDs);
void createShaderProgram(GLuint& programID, GLuint shaderID);

void createShader(GLuint& shaderID, const std::string& shader_fn, GLenum shaderType)
{
    std::string shader_data = "";
    std::streamoff len;
    std::ifstream file;

    file.open(shader_fn, std::ios::binary);
    if(!file.is_open())
        throw std::runtime_error("Failed to Open Shader file");
    file.seekg(0, std::ios::end);
    len = file.tellg();
    file.seekg(0, std::ios::beg);

    shader_data.resize(len);
    file.read(&shader_data[0], len);

    const GLchar* source = (const GLchar *) shader_data.c_str();

    shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID,1, &source,0);
    glCompileShader(shaderID);

    GLint isCompiled = 0;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &isCompiled);
    if(isCompiled == GL_FALSE)
    {
        GLint max_length = 0;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &max_length);
        std::vector<GLchar> infoLog(max_length);
        glGetShaderInfoLog(shaderID, max_length, &max_length, &infoLog[0]);

        glDeleteShader(shaderID);
        std::string err_log(infoLog.begin(), infoLog.end());
        std::cout << "\n" << err_log << std::endl;

        throw std::runtime_error("Failed to Compile Shader, detailed log is given above...");
    }
}

void createShaderProgram(GLuint& programID, std::vector<GLuint> shaderIDs)
{
    programID = glCreateProgram();
    for(int i = 0; i < shaderIDs.size(); i++)
        glAttachShader(programID, shaderIDs[i]);

    glLinkProgram(programID);

    GLint isLinked = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);
    if(isLinked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> infoLog(max_length);
        glGetProgramInfoLog(programID, max_length, &max_length, &infoLog[0]);

        for(int i = 0; i < shaderIDs.size(); i++)
            glDetachShader(programID, shaderIDs[i]);

        glDeleteProgram(programID);
        for(int i = 0; i < shaderIDs.size(); i++)
            glDeleteShader(shaderIDs[i]);

        std::string err_log(infoLog.begin(), infoLog.end());
        std::cout << "\n" << err_log << std::endl;
        throw std::runtime_error("Failed to Link Program Object, detailed log is given above...");
    }
}

void createShaderProgram(GLuint& programID, GLuint shaderID)
{
    programID = glCreateProgram();
    glAttachShader(programID, shaderID);

    glLinkProgram(programID);

    GLint isLinked = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &isLinked);
    if(isLinked == GL_FALSE)
    {
        GLint max_length = 0;
        glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> infoLog(max_length);
        glGetProgramInfoLog(programID, max_length, &max_length, &infoLog[0]);

        glDetachShader(programID, shaderID);
        glDeleteProgram(programID);
        glDeleteShader(shaderID);

        std::string err_log(infoLog.begin(), infoLog.end());
        std::cout << "\n" << err_log << std::endl;
        throw std::runtime_error("Failed to Link Program Object, detailed log is given above...");
    }
}
#endif // GLROUTINES_H
