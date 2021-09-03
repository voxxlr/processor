#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "extrusion.h"
#include "aabb.h"
#include "earcut.h"

Extrusion::Extrusion(Profile* iProfile)
: mMatrix(1.0)
, mProfile(iProfile)
{
	mProfile->triangulate();
}


void Extrusion::triangulateLoop(uint8_t iSide, std::vector<glm::dvec3>& iV0, std::vector<glm::dvec3>& iV1, Mesh& iMesh)
{
	for (size_t i=0; i<iV0.size(); i++)
	{
		glm::dvec3& lV0(iV0[i]);
		glm::dvec3& lV1(iV0[(i+1)%iV0.size()]);
		glm::dvec3& lV2(iV1[(i+1)%iV0.size()]);
		glm::dvec3& lV3(iV1[i]);

		switch (iSide)
		{
			case Mesh::FRONT: 
			{
				glm::dvec3 lN = Mesh::normal(lV0, lV1, lV2);
				iMesh.pushTriangle(lV0, lV1, lV2, lN);
				iMesh.pushTriangle(lV2, lV3, lV0, lN);
			}
			break;

			case Mesh::BACK: 
			{
				glm::dvec3 lN = Mesh::normal(lV2, lV1, lV0);
				iMesh.pushTriangle(lV2, lV1, lV0, lN);
				iMesh.pushTriangle(lV0, lV3, lV2, lN);
			}
			break;
		}
	}
}

void Extrusion::initLoop(std::vector<glm::dvec2>& iSource, std::vector<glm::dvec3>& iDest, glm::dmat4 iM)
{
	iDest.reserve(iSource.size());
	for (size_t i=0; i<iSource.size(); i++)
	{
		iDest.push_back(glm::dvec3(iSource[i], 0.0));
	}
}; 

void Extrusion::transformLoop(std::vector<glm::dvec2>& iSource, glm::dmat4& iTransform, std::vector<glm::dvec3>& iDest)
{
	iDest.resize(iSource.size());
	for (size_t i=0; i<iSource.size(); i++)
	{
		iDest[i] = glm::dvec3(iTransform*glm::dvec4(iSource[i], 0.0, 1.0));
	}
}

void Extrusion::segment(uint8_t iSide, std::vector<glm::dmat4>& iPath, Mesh& iMesh)
{
	std::vector<glm::dvec3> lBounds[2];
	transformLoop(mProfile->mBounds, iPath[0], lBounds[0]);
	std::vector<std::vector<glm::dvec3>> lHoles[2];
	lHoles[0].resize(mProfile->mHoles.size());
	lHoles[1].resize(mProfile->mHoles.size());
	for (size_t h=0; h<mProfile->mHoles.size(); h++)
	{
		transformLoop(mProfile->mHoles[h], iPath[0], lHoles[0][h]);
	}

	mMatrix = iPath.front();
	mProfile->cap(iSide == Mesh::FRONT ? Mesh::BACK : Mesh::FRONT, mMatrix, iMesh);

	for (size_t s=1; s<iPath.size(); s++)
	{
		transformLoop(mProfile->mBounds, iPath[s], lBounds[s%2]);
		triangulateLoop(iSide == Mesh::FRONT ? Mesh::FRONT : Mesh::BACK, lBounds[(s+1)%2], lBounds[s%2], iMesh);
		for (size_t h=0; h<mProfile->mHoles.size(); h++)
		{
			transformLoop(mProfile->mHoles[h], iPath[s], lHoles[s%2][h]);
			triangulateLoop(iSide == Mesh::FRONT ? Mesh::BACK : Mesh::FRONT, lHoles[(s+1)%2][h], lHoles[s%2][h], iMesh);
		}
	}

	mMatrix = iPath.back();
	mProfile->cap(iSide == Mesh::FRONT ? Mesh::FRONT : Mesh::BACK, mMatrix, iMesh);
}

void Extrusion::revolve(glm::dvec3 iCenter, glm::dvec3 iAxis, double iAngle, Mesh& iMesh)
{
	while (mProfile)
	{
		static float POINTS = 40;  // for full rotation
		int lSteps = ceil((POINTS-1)*iAngle/(2*M_PI));
		glm::dmat4 lT0 = glm::translate(glm::dmat4(1.0), -iCenter); 
		glm::dmat4 lT1 = glm::translate(glm::dmat4(1.0), iCenter); 

		double lDr = iAngle/lSteps;
		glm::dmat4 lMatrix(1.0);
		std::vector<glm::dmat4> lPath;
		lPath.push_back(lMatrix);
		for (int s=0; s<=lSteps; s++)
		{
			lMatrix = lT1*glm::rotate(glm::dmat4(1.0), s*lDr, iAxis)*lT0;
			lPath.push_back(lMatrix);
		}

		glm::dvec3 lD = glm::cross(iAxis, glm::dvec3(mProfile->mBounds[0], 0) - iCenter);
		if (lD.z < 0)
		{
			segment(Mesh::BACK, lPath, iMesh);
		}
		else
		{
			segment(Mesh::FRONT, lPath, iMesh);
		}

		Profile* lProfile = mProfile;
		mProfile = mProfile->mNext;
		delete mProfile;
	}
}

void Extrusion::extrude(glm::dvec3 iAxis, Mesh& iMesh)
{
	while (mProfile)
	{
		std::vector<glm::dmat4> lPath;

		lPath.push_back(glm::dmat4(1.0));

		glm::dmat4 lM(1.0);
		lM[3][0] = iAxis[0];
		lM[3][1] = iAxis[1];
		lM[3][2] = iAxis[2];
		lPath.push_back(lM);

		if (iAxis.z > 0)
		{
			segment(Mesh::FRONT, lPath, iMesh);
		}
		else
		{
			segment(Mesh::BACK, lPath, iMesh);
		}

		Profile* lProfile = mProfile;
		mProfile = mProfile->mNext;
		delete lProfile;
	}
}

void Extrusion::sweep(uint8_t iSide, std::vector<glm::dvec3>& iPath, Mesh& iMesh)
{
	while (mProfile)
	{
		std::vector<glm::dmat4> lPath;

		glm::dvec3 lZ(glm::normalize(iPath[1]-iPath[0]));
		glm::dvec3 lX(glm::normalize((fabs(lZ.z) < fabs(lZ.x)  ? glm::dvec3(lZ.y,-lZ.x,0) : glm::dvec3(0,-lZ.z,lZ.y))));
		glm::dvec3 lY(glm::cross(lZ, lX));

		#define PUSH_M(i)\
			lM[0][0] = lX.x;\
			lM[0][1] = lX.y;\
			lM[0][2] = lX.z;\
			lM[3][0] = iPath[i].x;\
			lM[1][0] = lY.x;\
			lM[1][1] = lY.y;\
			lM[1][2] = lY.z;\
			lM[3][1] = iPath[i].y;\
			lM[2][0] = lZ.x;\
			lM[2][1] = lZ.y;\
			lM[2][2] = lZ.z;\
			lM[3][2] = iPath[i].z;\
			lPath.push_back(lM);\


		glm::dmat4 lM(1.0);
		PUSH_M(0)

		for (size_t s=1; s<iPath.size()-1; s++)
		{
			glm::dvec3 lZn = glm::normalize((iPath[s]-iPath[s-1]) + (iPath[s+1]-iPath[s-1]));
			lZ = glm::normalize((iPath[s]-iPath[s-1]) + (iPath[s+1]-iPath[s-1]));
			lX = glm::cross(lY, lZ);
			lY = glm::cross(lZ, lX);
			PUSH_M(s)
		}

		lZ = glm::normalize(iPath[iPath.size()-1]-iPath[iPath.size()-2]);
		lX = glm::cross(lY, lZ);
		lY = glm::cross(lZ, lX);
		PUSH_M(iPath.size()-1)

		segment(iSide, lPath, iMesh);

		Profile* lProfile = mProfile;
		mProfile = mProfile->mNext;
		delete mProfile;
	}
}

