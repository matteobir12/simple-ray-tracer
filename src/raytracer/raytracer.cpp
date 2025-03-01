#include <iostream>
#include <fstream>

#include "raytracer/raytracer.h"
#include "raytracer/world.h"

namespace RayTracer {

void writeColor(std::ofstream& out, const Color& pixelColor) {
	auto r = pixelColor.x;
	auto g = pixelColor.y;
	auto b = pixelColor.z;

	r = Common::linearToGamma(r);
	g = Common::linearToGamma(g);
	b = Common::linearToGamma(b);

	static const Common::Interval intensity(0.000f, 0.999f);
	int rByte = int(256 * intensity.clamp(r));
	int gByte = int(256 * intensity.clamp(g));
	int bByte = int(256 * intensity.clamp(b));

	out << rByte << ' ' << gByte << ' ' << bByte << '\n';
}

void RayTracer::Init(World& w) {
	camera.Initialize();
	world = w;
}

void RayTracer::Render() {
	uint width = camera.getWidth();
	uint height = camera.getHeight();
	uint samplesPerPixel = camera.getSettings().samplesPerPixel;
	uint maxDepth = camera.getSettings().maxDepth;

	// TODO this is what will need to move to a compute shader so the work can be done in wavefronts instead of a loop
	std::ofstream ostream(outFileName);
	ostream << "P3\n" << width << ' ' << height << "\n255\n";
	for (uint j = 0; j < height; ++j)
	{
		std::clog << "\rLines Remining: " << (height - j) << ' ' << std::flush;
		for (uint i = 0; i < width; ++i)
		{
			Color pixelColor(0, 0, 0);
			for (uint sample = 0; sample < samplesPerPixel; sample++)
			{
				Common::Ray r = camera.GetRay(i, j);
				pixelColor += camera.RayColor(r, maxDepth, world);
			}
			
			writeColor(ostream, camera.getPixelSamplesScale() * pixelColor);
		}
	}

	std::clog << "\rDone.		\n";
}

}