#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

std::ostream& operator<<(std::ostream& out, glm::vec3 const& vec);
std::ostream& operator<<(std::ostream& out, const glm::mat4& mat);
std::ostream& operator<<(std::ostream& out, const glm::dvec3& vec);
std::ostream& operator<<(std::ostream& out, const glm::dmat4& mat);
std::ostream& operator<<(std::ostream& out, const glm::dmat3& mat);
std::ostream& operator<<(std::ostream& out, const glm::dvec2& vec);
std::ostream& operator<<(std::ostream& out, const glm::dvec4& vec);


std::ostream& operator<<(std::ostream& out, const std::vector<glm::vec3>& list);
std::ostream& operator<<(std::ostream& out, const std::vector<glm::dvec3>& list);
std::ostream& operator<<(std::ostream& out, const std::vector<glm::dvec2>& list);

class AABB
{
	public:

		glm::dvec3 min;
		glm::dvec3 max;
		glm::dvec3 center;
		glm::dvec3 extent;

		AABB()
		{
			min[0] = std::numeric_limits<double>::max();
			min[1] = std::numeric_limits<double>::max();
			min[2] = std::numeric_limits<double>::max();

			max[0] = -std::numeric_limits<double>::max();
			max[1] = -std::numeric_limits<double>::max();
			max[2] = -std::numeric_limits<double>::max();
		};

		AABB(std::vector<glm::dvec3>& iPoints);
		AABB(std::vector<glm::dvec2>& iPoints);

		AABB(const AABB& iAABB)
		{
			min = iAABB.min;
			max = iAABB.max;
			center = iAABB.center;
			extent = iAABB.extent;
		};

		void set(double minx, double miny, double minz, double maxx, double maxy, double maxz)
		{
			min[0] = minx;
			min[1] = miny;
			min[2] = minz;

			max[0] = maxx;
			max[1] = maxy;
			max[2] = maxz;
		};

		void grow(AABB& iAABB);

		void grow(glm::dvec2 iPoint)
		{
			min[0] = std::min(min[0], iPoint.x);
			min[1] = std::min(min[1], iPoint.y);

			max[0] = std::max(max[0], iPoint.x);
			max[1] = std::max(max[1], iPoint.y);
		}

		void grow(glm::dvec3& iPoint);

		void translate(glm::dvec3& iVector)
		{
			min += iVector;
			max += iVector;
		}

		json_spirit::mObject minJson();
		json_spirit::mObject maxJson();

		double range()
		{
			return glm::length(glm::max(glm::abs(max), glm::abs(min)));
		}

};

std::ostream& operator<<(std::ostream& out, const AABB& aabb);
