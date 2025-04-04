#include "graphics/shader.h"
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <exception>
#include <regex>

namespace Graphics
{

    Shader::Shader(const char *path)
    {
        std::string source;
        std::ifstream shaderStream;

        shaderStream.open(path);
        std::stringstream sStream;
        sStream << shaderStream.rdbuf();
        shaderStream.close();

        std::string relPath = GetRelPath(path);
        m_shaderSource = Parse(sStream, relPath);
    }

    Shader::~Shader()
    {
        glDeleteShader(m_shader);
    }

    std::string Shader::GetRelPath(std::string path)
    {
        auto const pos = path.find_last_of('/');
        return path.substr(0, pos + 1);
    }

    std::string Shader::Parse(std::stringstream &stream, std::string relPath, uint level)
    {
        if (level > 32)
            throw std::range_error("header inclusion depth limit reached, might be caused by cyclic header inclusion");

        static const std::regex rex("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
        std::stringstream output;

        std::smatch matches;

        std::string line;
        while (std::getline(stream, line))
        {
            if (std::regex_search(line, matches, rex))
            {
                std::string include_file = relPath;
                include_file += matches[1];
                std::string include_string;

                std::ifstream shaderStream;
                shaderStream.open(include_file);
                std::stringstream sStream;
                sStream << shaderStream.rdbuf();
                shaderStream.close();

                output << Parse(sStream, relPath, level + 1) << std::endl;
            }
            else
            {
                output << line << std::endl;
            }
        }
        return output.str();
    }

    void Shader::Use()
    {
        glUseProgram(m_program);
    }

    void Shader::SetBool(const std::string &name, bool val)
    {
        glUniform1i(glGetUniformLocation(m_program, name.c_str()), (int)val);
    }

    void Shader::SetInt(const std::string &name, int val)
    {
        glUniform1i(glGetUniformLocation(m_program, name.c_str()), val);
    }

    void Shader::SetUInt(const std::string &name, uint val)
    {
        glUniform1ui(glGetUniformLocation(m_program, name.c_str()), val);
    }

    void Shader::SetFloat(const std::string &name, float val)
    {
        glUniform1f(glGetUniformLocation(m_program, name.c_str()), val);
    }

    void Shader::SetVec3(const std::string &name, glm::vec3 val)
    {
        glUniform3f(glGetUniformLocation(m_program, name.c_str()), val.x, val.y, val.z);
    }

    void Shader::SetBoolArray(const std::string &name, uint count, bool *val)
    {
        glUniform1iv(glGetUniformLocation(m_program, name.c_str()), count, (int *)val);
    }

    void Shader::SetIntArray(const std::string &name, uint count, const int *val)
    {
        glUniform1iv(glGetUniformLocation(m_program, name.c_str()), count, val);
    }

    void Shader::SetFloatArray(const std::string &name, uint count, const float *val)
    {
        glUniform1fv(glGetUniformLocation(m_program, name.c_str()), count, val);
    }

    void Shader::SetVec3Array(const std::string &name, uint count, glm::vec3 val)
    {
        glUniform3fv(glGetUniformLocation(m_program, name.c_str()), count, glm::value_ptr(val));
    }

    //---------------Compute-----------------------//
    void Compute::Init()
    {
        const char *src = m_shaderSource.c_str();
        m_shader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(m_shader, 1, &src, nullptr);
        glCompileShader(m_shader);

        GLint success;
        glGetShaderiv(m_shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            GLchar infoLog[512];
            glGetShaderInfoLog(m_shader, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                      << infoLog << std::endl;
            std::terminate();
        }

        m_program = glCreateProgram();
        glAttachShader(m_program, m_shader);
        glLinkProgram(m_program);

        // Check for linking errors
        GLchar infoLog[512];
        glGetProgramiv(m_program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(m_program, 512, nullptr, infoLog);
            std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n"
                      << infoLog << std::endl;
            std::terminate();
        }
    }

    void Graphics::Compute::SetInt(const std::string &name, int value)
    {
        GLint currentProg = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProg);
        if (currentProg != m_program)
        {
            glUseProgram(m_program);
        }

        GLint location = glGetUniformLocation(m_program, name.c_str());
        if (location != -1)
        {
            glUniform1i(location, value);
        }
        else
        {
            // Don't treat as error - some uniforms might be optimized out in the shader
            // if not used, especially with layout(binding = X) syntax
        }

        // Restore previous program if needed
        if (currentProg != m_program && currentProg != 0)
        {
            glUseProgram(currentProg);
        }
    }

}
