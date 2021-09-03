#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

#include "face.h"
#include "aabb.h"
#include "earcut.h"


Face::Face()
: mBounds()
, mHoles()
{
};

void Face::test(std::vector<glm::dvec3>& iVertex, glm::dvec3 iN)
{
	glm::dvec3 lU = glm::normalize(iVertex[1] - iVertex[0]);
	glm::dvec3 lV = glm::cross(iN, lU);
	glm::dmat3 lMatrix(lU, lV, iN);
	glm::dmat3 lInverse = glm::inverse(lMatrix);
	double lD = glm::dot(lMatrix[2], iVertex[0]);

	std::vector<glm::dvec2> lVertex;
	for (size_t i=0; i<iVertex.size(); i++)
	{
		glm::dvec3 lPlanePoint = lInverse*iVertex[i];
		lVertex.push_back(glm::vec2(lPlanePoint.x, lPlanePoint.y));
	}

	for (size_t i=0; i<lVertex.size(); i++)
	{
		iVertex[i] = lMatrix*glm::dvec3(lVertex[i].x, lVertex[i].y, lD);
	}

};


void Face::triangulate(uint8_t iSide, Mesh& iMesh)
{
	glm::dvec3 lN = normal();

	// use longest edge as u 
	double lMax = 0;
	glm::dvec3 lU;
	for (size_t i=0; i<mBounds.size(); i++)
	{
		glm::dvec3 e = mBounds[(i+1)%mBounds.size()] - mBounds[i];
		if (glm::length2(e) > lMax)
		{
			lU = e;
			lMax = glm::length2(e);
		}
	}
	lU = glm::normalize(lU);

	// create local coordinate system and conversion matrices
	glm::dvec3 lV = glm::cross(lN, lU);
	glm::dmat3 lMatrix(lU, lV, lN);
	glm::dmat3 lInverse = glm::inverse(lMatrix);
	double lD = glm::dot(lMatrix[2], mBounds[0]);

	std::vector<glm::dvec3> lVertex;

	std::vector<std::vector<glm::dvec2>> lBounds;
	lBounds.resize(1+mHoles.size());
	lVertex.insert(lVertex.end(), mBounds.begin(), mBounds.end());
	project2D(mBounds, lInverse, lBounds[0]);
	std::vector<std::vector<glm::dvec2>> lHoles;
	for (size_t i=0; i<mHoles.size(); i++)
	{
		lVertex.insert(lVertex.end(), mHoles[i].begin(), mHoles[i].end());
		project2D(mHoles[i], lInverse, lBounds[i+1]);
	}

	std::vector<uint32_t> lIndices = mapbox::earcut<uint32_t>(lBounds);
	for (size_t i = 0; i < lIndices.size()/3; i++) 
	{
		glm::dvec3 lVa = lVertex[lIndices[i*3+0]];
		glm::dvec3 lVb = lVertex[lIndices[i*3+1]];
		glm::dvec3 lVc = lVertex[lIndices[i*3+2]];

		glm::dvec3 e0 = lVb - lVa;
		glm::dvec3 e1 = lVc - lVb;

		if (iSide & Mesh::FRONT)
		{
			if (glm::dot(lN, glm::normalize(glm::cross(e0, e1))) < 0.0)
			{
				iMesh.pushTriangle(lVc, lVb, lVa, lN); 
			}
			else
			{
				iMesh.pushTriangle(lVa, lVb, lVc, lN); 
			}
		}

		if (iSide & Mesh::BACK)
		{
			if (glm::dot(lN, glm::normalize(glm::cross(e0, e1))) < 0.0)
			{
				glm::dvec3 lNn = -lN;
				iMesh.pushTriangle(lVa, lVb, lVc, lNn); 
			}
			else
			{
				glm::dvec3 lNn = -lN;
				iMesh.pushTriangle(lVc, lVb, lVa, lNn); 
			}
		}
	}
}

void Face::project2D(std::vector<glm::dvec3>& iLoop3D, glm::dmat3& iProjection, std::vector<glm::dvec2>& iLoop2D)
{
	for (size_t i = 0; i < iLoop3D.size(); ++i)
	{
		glm::dvec3 lPlanePoint = iProjection*glm::dvec3(iLoop3D[i].x, iLoop3D[i].y, iLoop3D[i].z);
		iLoop2D.push_back(glm::dvec2(lPlanePoint));
	}
}

// Newells alg
glm::dvec3 Face::normal()
{
	glm::dvec3 lN(0);
  
	for (size_t i=0; i < mBounds.size(); i++) 
	{
		glm::dvec3 pj = mBounds[i];
		glm::dvec3 pi = mBounds[(i+1)%mBounds.size()];
        
		lN.x += (((pi.z) + (pj.z)) * ((pj.y) - (pi.y)));
		lN.y += (((pi.x) + (pj.x)) * ((pj.z) - (pi.z)));
		lN.z += (((pi.y) + (pj.y)) * ((pj.x) - (pi.x)));
	}
  
	return glm::normalize(lN);
}




std::vector<glm::dvec3>& Face::newHole()
{
	mHoles.resize(mHoles.size()+1);
	return mHoles[mHoles.size()-1];
};








/*
float Face::computeArea(std::vector<glm::dvec2>& iPolygon )
{
	float lArea = 0;
	for (int i=0; i<iPolygon.size(); i++)
	{
		p2t::Point& lPrev = *iPolygon[i];
		p2t::Point& lNext = *iPolygon[(i+1)%iPolygon.size()];
		lArea += (lNext.x - lPrev.x)*(lNext.y + lPrev.y);
	}
	return lArea/2.0;
}
*/

