#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include <vector>

#include "mesh.h"

class Profile
{
	public:
		
		Profile(std::vector<glm::dvec3>& iProfile);
		Profile(std::vector<glm::dvec2>& iProfile);
		Profile();	

		void append(Profile* iProfile);
		Profile* mNext;

		std::vector<glm::dvec2> mBounds;
		std::vector<std::vector<glm::dvec2>> mHoles; 
		std::vector<glm::dvec2>& newHole();


		void transform(glm::dmat3& iMatrix);

		void triangulate();
		void cap(uint8_t iSide, glm::dmat4& iMatrix, Mesh& iMesh);

		static void circle(Profile& iProfile, double iRadius);
		static void circle(Profile& iProfile, double iOuterRadius, double iInnerRadius);

	private:

		void ccw(std::vector<glm::dvec2>& iLine);
		void transform(std::vector<glm::dvec2>& iVertex, glm::dmat3& iMatrix);

		std::vector<glm::dvec2> mTriangulation;
};










