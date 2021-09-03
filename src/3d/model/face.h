#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include <vector>

#include "mesh.h"

class Face
{
	public:

		Face();

		glm::dvec3 normal();

		void triangulate(uint8_t iSide, Mesh& iMesh);

		std::vector<glm::dvec3>& newHole();

		std::vector<glm::dvec3> mBounds;
		std::vector<std::vector<glm::dvec3>> mHoles; 

		void test(std::vector<glm::dvec3>& iVertex, glm::dvec3 iN);

	private:

		void project2D(std::vector<glm::dvec3>& iLoop3D, glm::dmat3& iProjection, std::vector<glm::dvec2>& iLoop2D);

};








