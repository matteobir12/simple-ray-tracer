#pragma once

#include <glad/glad.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace Compute {
namespace Detail {
GLuint LoadComputeShader(const char* const path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "err opening" << path << std::endl;
    return 0;
  }

  const std::string source(
    (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();

  GLuint shader_ID = glCreateShader(GL_COMPUTE_SHADER);

  const char* const src = source.c_str();
  glShaderSource(shader_ID, 1, &src, nullptr);

  glCompileShader(shader_ID);

  GLint success = 0;
  glGetShaderiv(shader_ID, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint log_size = 0;
    glGetShaderiv(shader_ID, GL_INFO_LOG_LENGTH, &log_size);
    std::vector<char> error_log((size_t)log_size);
    glGetShaderInfoLog(shader_ID, log_size, &log_size, &error_log[0]);
    std::cerr << "compile error:\n" << &error_log[0] << std::endl;
    glDeleteShader(shader_ID);
    return 0;
  }

  return shader_ID;
}
}

GLuint CreateComputeProgram(const char* const path) {
  GLuint compute_shader = Detail::LoadComputeShader(path);
  if (compute_shader == 0)
    return 0;

  GLuint program = glCreateProgram();
  glAttachShader(program, compute_shader);
  glLinkProgram(program);

  GLint success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    GLint log_size = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_size);
    std::vector<char> error_log((size_t)log_size);
    glGetProgramInfoLog(program, log_size, &log_size, &error_log[0]);
    std::cerr << "Compute shader link error:\n" << &error_log[0] << std::endl;
    glDeleteProgram(program);
    glDeleteShader(compute_shader);
    return 0;
  }

  glDetachShader(program, compute_shader);
  glDeleteShader(compute_shader);

  return program;
}

}