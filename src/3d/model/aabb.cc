#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "aabb.h"

std::ostream& operator<<(std::ostream& out, glm::vec3 const& vec)
{
    out << "{" << vec.x << " " << vec.y << " "<< vec.z  << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const glm::mat4& mat)
{
    out << "{" << mat[0][0] << ",\t" << mat[0][1] << ",\t" << mat[0][2] << ",\t" << mat[0][3] << "\n"
		<< " " << mat[1][0] << ",\t" << mat[1][1] << ",\t" << mat[1][2] << ",\t" << mat[1][3] << "\n"
		<< " " << mat[2][0] << ",\t" << mat[2][1] << ",\t" << mat[2][2] << ",\t" << mat[2][3] << "\n"
		<< " " << mat[3][0] << ",\t" << mat[3][1] << ",\t" << mat[3][2] << ",\t" << mat[3][3] << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const glm::dvec3& vec)
{
    out << "{" << vec.x << " " << vec.y << " "<< vec.z  << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const glm::dvec4& vec)
{
    out << "{" << vec.x << " " << vec.y << " " << vec.z  << " " << vec.w  <<"}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const glm::dvec2& vec)
{
    out << "{" << vec.x << " " << vec.y << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const glm::dmat4& mat)
{
    out << "{" << mat[0][0] << ",\t" << mat[0][1] << ",\t" << mat[0][2] << ",\t" << mat[0][3] << "\n"
		<< " " << mat[1][0] << ",\t" << mat[1][1] << ",\t" << mat[1][2] << ",\t" << mat[1][3] << "\n"
		<< " " << mat[2][0] << ",\t" << mat[2][1] << ",\t" << mat[2][2] << ",\t" << mat[2][3] << "\n"
		<< " " << mat[3][0] << ",\t" << mat[3][1] << ",\t" << mat[3][2] << ",\t" << mat[3][3] << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const glm::dmat3& mat)
{
    out << "{" << mat[0][0] << ",\t" << mat[0][1] << ",\t" << mat[0][2] << "\n"
		<< " " << mat[1][0] << ",\t" << mat[1][1] << ",\t" << mat[1][2] << "\n"
		<< " " << mat[2][0] << ",\t" << mat[2][1] << ",\t" << mat[2][2] << "}";
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::vector<glm::vec3>& iList)
{
	for (size_t i=0; i<std::min((size_t)100, iList.size()); i++)
	{
		std::cout << iList[i] << "\n";
	}
    return out;
};

std::ostream& operator<<(std::ostream& out, const std::vector<glm::dvec3>& iList)
{
	for (size_t i=0; i<std::min((size_t)100, iList.size()); i++)
	{
		std::cout << iList[i] << "\n";
	}
    return out;
};

std::ostream& operator<<(std::ostream& out, const std::vector<glm::dvec2>& iList)
{
	for (size_t i=0; i<std::min((size_t)100, iList.size()); i++)
	{
		std::cout << iList[i] << "\n";
	}
    return out;
};

std::ostream& operator<<(std::ostream& out, const AABB& aabb)
{
	glm::dvec3 lE = aabb.max - aabb.min;
    out << "min" << aabb.min << "\n"
		<< "max" << aabb.max << "\n"
		<< "ext" << lE << "\n";

    return out;
}

AABB::AABB(std::vector<glm::dvec3>& iPoints)
{
	min[0] = std::numeric_limits<double>::max();
	min[1] = std::numeric_limits<double>::max();
	min[2] = std::numeric_limits<double>::max();

	max[0] = -std::numeric_limits<double>::max();
	max[1] = -std::numeric_limits<double>::max();
	max[2] = -std::numeric_limits<double>::max();

	for (size_t i=0; i<iPoints.size(); i++)
	{
		grow(iPoints[i]);
	}
};

AABB::AABB(std::vector<glm::dvec2>& iPoints)
{
	min[0] = std::numeric_limits<double>::max();
	min[1] = std::numeric_limits<double>::max();
	min[2] = std::numeric_limits<double>::max();

	max[0] = -std::numeric_limits<double>::max();
	max[1] = -std::numeric_limits<double>::max();
	max[2] = -std::numeric_limits<double>::max();

	for (size_t i=0; i<iPoints.size(); i++)
	{
		grow(iPoints[i]);
	}
};

void AABB::grow(AABB& iAABB)
{
	min[0] = std::min(min[0], iAABB.min[0]);
	min[1] = std::min(min[1], iAABB.min[1]);
	min[2] = std::min(min[2], iAABB.min[2]);

	max[0] = std::max(max[0], iAABB.max[0]);
	max[1] = std::max(max[1], iAABB.max[1]);
	max[2] = std::max(max[2], iAABB.max[2]);
}

void AABB::grow(glm::dvec3& iPoint)
{
	min[0] = std::min(min[0], iPoint.x);
	min[1] = std::min(min[1], iPoint.y);
	min[2] = std::min(min[2], iPoint.z);
	max[0] = std::max(max[0], iPoint.x);
	max[1] = std::max(max[1], iPoint.y);
	max[2] = std::max(max[2], iPoint.z);
}

json_spirit::mObject AABB::minJson()
{
	json_spirit::mObject lMin;
	lMin["x"] = min[0];
	lMin["y"] = min[1];
	lMin["z"] = min[2];
	return lMin;
};

json_spirit::mObject AABB::maxJson()
{
	json_spirit::mObject lMax;
	lMax["x"] = max[0];
	lMax["y"] = max[1];
	lMax["z"] = max[2];
	return lMax;
}






/*

bool BspNode::intersect(Triangle& iTriangle, glm::dvec3& iNormal) 
{
	// check if triangke intersects AABB  http://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox_tam.pdf
	glm::dvec3 v0 = mVertexCache[std::get<0>(iTriangle)] - mCenter;
	glm::dvec3 v1 = mVertexCache[std::get<1>(iTriangle)] - mCenter;
	glm::dvec3 v2 = mVertexCache[std::get<2>(iTriangle)] - mCenter;

	// Test 1
	for (int i=0; i<3; i++)
	{
		if (std::min({v0[i],v1[i],v2[i]}) > mExtent[i] || std::max({v0[i],v1[i],v2[i]}) < -mExtent[i])
		{
			return false;
		}
	}

	// Test 2 
	double d = -glm::dot(iNormal, v0);
	glm::dvec3 lMin, lMax;
	for (int i=0; i<3; i++) 
	{
		if (iNormal[i] > 0.0) 
		{
			lMin[i] = -mExtent[i];
			lMax[i] = mExtent[i];
		} 
		else 
		{
			lMin[i] = mExtent[i];
			lMax[i] = -mExtent[i];
		}
	}

	if (glm::dot(iNormal, lMin)+d > 0.0 || glm::dot(iNormal, lMax)+d < 0.0)
	{
		return false;
	}

	// Test 3
	static glm::dvec3 u[3] = { glm::dvec3(1.0, 0.0, 0.0), 
							   glm::dvec3(0.0, 1.0, 0.0),
							   glm::dvec3(0.0, 0.0, 1.0) };

	glm::dvec3 e[3] = { v1 - v0, v2 - v1, v0 - v2 };
	for (int ui=0; ui<3; ui++)
	{
		for (int ei=0; ei<3; ei++)
		{
			glm::dvec3 ue = glm::cross(u[ui], e[ei]);

			double p0 = glm::dot(v0, ue);
			double p1 = glm::dot(v1, ue);
			double p2 = glm::dot(v2, ue);

			double r = mExtent.x * fabs(glm::dot(u[0], ue)) + mExtent.y * fabs(glm::dot(u[1], ue)) + mExtent.z * fabs(glm::dot(u[2], ue));

			if (std::max(-std::max({p0, p1, p2}), std::min({p0, p1, p2})) > r) 
			{
				return false;
			}
		}
	}

	return true;
}

		void test(BspNode& iTree);
		void test(Triangle& iTriangle, glm::dvec3& iNormal);
		bool intersect(Triangle& iTriangle, glm::dvec3& iNormal);

*/