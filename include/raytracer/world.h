#pragma once

#include "raytracer_types.h"
#include "common/types.h"

#include <vector>
#include <memory>

namespace RayTracer {

class World {
public:
	World() = default;

	void add(std::shared_ptr<Hittable> hittable) { objects.push_back(hittable); }
	void clear() { objects.clear(); }

	bool CheckHit(const Common::Ray& r, const Common::Interval& interval, HitRecord& hit);

private:
	std::vector<std::shared_ptr<Hittable>> objects;
};

}
