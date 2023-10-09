#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace RenderEngine
{
	struct Geometry
	{
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> colors;
		std::vector<glm::vec3> normals;
		std::vector<int16_t> indexes;
		std::vector<glm::vec2> uv;
	};
}
