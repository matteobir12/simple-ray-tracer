#include "raytracer/world.h"

namespace RayTracer {

bool World::CheckHit(const Common::Ray& ray, const Common::Interval& interval, HitRecord& hit) {
	HitRecord tempRec;
	bool hitSomething = false;
	auto closest = interval.max;

	for (const auto& object : objects)
	{
		if (object->Hit(ray, Common::Interval(interval.min, closest), tempRec))
		{
			hitSomething = true;
			closest = tempRec.t;
			hit = tempRec;
		}
	}

	return hitSomething;
}

}