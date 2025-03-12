#pragma once

#include "glm/glm.hpp"
#include "common/utils.h"

#include <vector>

namespace RayTracer {

	constexpr int RAND_COUNT = 100000;

	std::vector<glm::vec3> getNoiseBuffer() {
		std::vector<glm::vec3> ret(RAND_COUNT);
		for (int i = 0; i < RAND_COUNT; ++i) {
			ret.push_back(Common::randomUnitVector());
		}

		return ret;
	}
}