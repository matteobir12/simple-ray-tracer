#pragma once

#include <string>
#include <vector>

#include "common/types.h"
#include "common/utils.h"
#include "graphics/texture.h"
#include "raytracer_types.h"
#include "raytracer/camera.h"

namespace RayTracer {

// TODO need to get this to pass a texture into render to write to instead of the output file

class RayTracer {
public:
	RayTracer(CameraSettings settings, std::string outFile, bool writeTexture)
		: camera(settings)
		, outFileName(outFile)
		, world()
		, writeTexture(writeTexture)
		, texture()
	{}

	void Init(World& w);
	void Render();

	Graphics::Texture& getTexture() { return texture; }

private:
	std::string outFileName;
	Camera camera;
	World world;
	Graphics::Texture texture;
	bool writeTexture;
};
}