#include "graphics/shader.h"
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

namespace Graphics {

Shader::Shader(const char* path) {
	std::string source;
	std::ifstream shaderStream;

	shaderStream.open(path);
	std::stringstream sStream;
	sStream << shaderStream.rdbuf();
	shaderStream.close();
    m_shaderSource = sStream.str();
}

Shader::~Shader() {
    glDeleteShader(m_shader);
}

void Shader::Use(){
    glUseProgram(m_program);
}

void Shader::SetBool(const std::string& name, bool val) {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), (int)val);
}

void Shader::SetInt(const std::string& name, int val) {
    glUniform1i(glGetUniformLocation(m_program, name.c_str()), val);
}

void Shader::SetFloat(const std::string& name, float val) {
    glUniform1f(glGetUniformLocation(m_program, name.c_str()), val);
}

void Shader::SetVec3(const std::string& name, glm::vec3 val) {
    glUniform3f(glGetUniformLocation(m_program, name.c_str()), val.x, val.y, val.z);
}

void Shader::SetBoolArray(const std::string& name, uint count, bool* val) {
    glUniform1iv(glGetUniformLocation(m_program, name.c_str()), count, (int*)val);
}

void Shader::SetIntArray(const std::string& name, uint count, const int* val) {
    glUniform1iv(glGetUniformLocation(m_program, name.c_str()), count, val);
}

void Shader::SetFloatArray(const std::string& name, uint count, const float* val) {
    glUniform1fv(glGetUniformLocation(m_program, name.c_str()), count, val);
}

void Shader::SetVec3Array(const std::string& name, uint count, glm::vec3 val) {
    glUniform3fv(glGetUniformLocation(m_program, name.c_str()), count, glm::value_ptr(val));
}


//---------------Compute-----------------------//
void Compute::Init() {
    const char* src = m_shaderSource.c_str();
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
    }
}

}