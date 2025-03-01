#pragma once

#include <string>

#include "common/types.h"
#include "common/utils.h"
#include "raytracer_types.h"
#include "raytracer/camera.h"

namespace RayTracer {

// TODO need to get this to pass a texture into render to write to instead of the output file

class RayTracer {
public:
	RayTracer(CameraSettings settings, std::string outFile)
		: camera(settings)
		, outFileName(outFile)
		, world()
	{}

	void Init(World& w);
	void Render();

private:
	std::string outFileName;
	Camera camera;
	World world;
};
}