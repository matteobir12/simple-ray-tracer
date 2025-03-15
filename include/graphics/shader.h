#pragma once

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <glm/vec3.hpp>
#include <glm/glm.hpp>

#include "common/types.h"

#include <string>

namespace Graphics {

using uint = Common::uint;

class Shader {
public:
	Shader(const char* path);
	virtual ~Shader();

	virtual void Init() = 0;
	void Use();

	GLuint GetProgram() const { return m_program; }

	void SetBool(const std::string& name, bool val);
	void SetInt(const std::string& name, int val);
	void SetFloat(const std::string& name, float val);
	void SetVec3(const std::string& name, glm::vec3 val);
	void SetBoolArray(const std::string& name, uint count, bool* val);
	void SetIntArray(const std::string& name, uint count, const int* val);
	void SetFloatArray(const std::string& name, uint count, const float* val);
	void SetVec3Array(const std::string& name, uint count, const glm::vec3 val);

protected:
	std::string m_shaderSource;
	GLuint m_shader;
	GLuint m_program;

private:
	std::string Parse(std::stringstream& stream, std::string relPath, uint level = 0);
	std::string GetRelPath(std::string path);
};

class Compute : public Shader {
public:
	Compute(const char* path)
		: Shader(path)
	{}

	void Init() override;
};

}