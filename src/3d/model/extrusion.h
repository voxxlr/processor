#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include <vector>

#include "mesh.h"
#include "profile.h"

class Extrusion
{
	public:
		
		Extrusion(Profile* iProfile);

		void extrude(glm::dvec3 iVector, Mesh& iMesh);
		void revolve(glm::dvec3 iCenter, glm::dvec3 iAxis, double iAngle, Mesh& iMesh);
		void sweep(uint8_t iSide, std::vector<glm::dvec3>& iPath, Mesh& iMesh);

	private:
		
		Profile* mProfile;

		void segment(uint8_t iSide, std::vector<glm::dmat4>& iPath, Mesh& iMesh);

		void triangulateLoop(uint8_t iSide, std::vector<glm::dvec3>& iVb, std::vector<glm::dvec3>& iVt,Mesh& iMesh);
		void transformLoop(std::vector<glm::dvec2>& iSource, glm::dmat4& iTransform, std::vector<glm::dvec3>& iDest);
		void initLoop(std::vector<glm::dvec2>& iSource, std::vector<glm::dvec3>& iDest, glm::dmat4 iM); 

		glm::dmat4 mMatrix;

};










