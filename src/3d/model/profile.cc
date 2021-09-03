#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "profile.h"
#include "aabb.h"
#include "earcut.h"

Profile::Profile()
: mNext(0)
{
}
  
Profile::Profile(std::vector<glm::dvec2>& iProfile)
: mNext(0)
{
	mBounds.reserve(iProfile.size());
	for (std::vector<glm::dvec2>::iterator lIter = iProfile.begin(); lIter != iProfile.end(); lIter++)
	{
		mBounds.push_back(*lIter);
	}

	triangulate();
};

Profile::Profile(std::vector<glm::dvec3>& iProfile)
: mNext(0)
{
	mBounds.reserve(iProfile.size());
	for (std::vector<glm::dvec3>::iterator lIter = iProfile.begin(); lIter != iProfile.end(); lIter++)
	{
		mBounds.push_back(*lIter);
	}

	triangulate();
};

void Profile::append(Profile* iProfile)
{
	if (iProfile)
	{
		iProfile->mNext = mNext;
		mNext = iProfile;
	}
};

std::vector<glm::dvec2>& Profile::newHole()
{
	mHoles.resize(mHoles.size()+1);
	return mHoles[mHoles.size()-1];
};

void Profile::ccw(std::vector<glm::dvec2>& iLine)
{
	// make sure winding is CCW
	double lArea = 0;
	for (size_t i=0; i<iLine.size(); i++)
	{
		glm::dvec2& lPrev = iLine[i];
		glm::dvec2& lNext = iLine[(i+1)%iLine.size()];
		lArea += (lNext.x - lPrev.x)*(lNext.y + lPrev.y);
	}
	if (lArea > 0)
	{
		std::reverse(iLine.begin(), iLine.end());
	}
} 

void Profile::triangulate()
{
	// make sure the shape is open and has ccw winding order
	//if (mBounds.back() == mBounds.front())
	if (glm::all(glm::epsilonEqual(mBounds.back(), mBounds.front(), 0.00001)))
	{
		mBounds.pop_back();
	}
	ccw(mBounds);
	for (std::vector<std::vector<glm::dvec2>>::iterator lIter = mHoles.begin(); lIter != mHoles.end(); ++lIter )
	{
		if (glm::all(glm::epsilonEqual(lIter->back(), lIter->front(), 0.00001)))
		//if (lIter->back() == lIter->front())
		{
			lIter->pop_back();
		}
		ccw(*lIter);
	}
	//std::cout << mBounds;
	// this is a cracy conversion JSTIER
	std::vector<glm::dvec2> lVertex;
	std::vector<std::vector<glm::dvec2>> lBounds;
	lBounds.push_back(mBounds);
	lVertex.insert(lVertex.end(), mBounds.begin(), mBounds.end());
	for (size_t i=0; i<mHoles.size(); i++)
	{
		lBounds.push_back(mHoles[i]);
		lVertex.insert(lVertex.end(), mHoles[i].begin(), mHoles[i].end());
	}

	std::vector<uint32_t> lIndices = mapbox::earcut<uint32_t>(lBounds);
	for (size_t i = 0; i < lIndices.size()/3; i++) 
	{
		mTriangulation.push_back(lVertex[lIndices[i*3+0]]);
		mTriangulation.push_back(lVertex[lIndices[i*3+1]]);
		mTriangulation.push_back(lVertex[lIndices[i*3+2]]);
	}

	if (mNext)
	{
		mNext->triangulate();
	}
}

void Profile::circle(Profile& iProfile, double iRadius)
{
	static double kN = 12;
	
	double dR = 2.0*M_PI/kN;
	for (int i=0; i<kN; i++)
	{
		iProfile.mBounds.push_back(iRadius*glm::dvec3(cos(i*dR), sin(i*dR), 0));
	}
};

void Profile::circle(Profile& iProfile, double iOuterRadius, double iInnerRadius)
{
	static double kN = 12;
	
	std::vector<glm::dvec2> lHole;

	double dR = 2.0*M_PI/kN;
	for (int i=0; i<kN; i++)
	{
		iProfile.mBounds.push_back(iOuterRadius*glm::dvec3(cos(i*dR), sin(i*dR), 0));
		lHole.push_back(iInnerRadius*glm::dvec3(cos(i*dR), sin(i*dR), 0));
	}

	iProfile.mHoles.push_back(lHole);
};

void Profile::cap(uint8_t iSide, glm::dmat4& iMatrix, Mesh& iMesh)
{
	for (size_t i = 0; i < mTriangulation.size()/3; i++) 
	{
		glm::dvec3 lVa = glm::dvec3(iMatrix*glm::dvec4(mTriangulation[i*3+0], 0, 1.0));
		glm::dvec3 lVb = glm::dvec3(iMatrix*glm::dvec4(mTriangulation[i*3+1], 0, 1.0));
		glm::dvec3 lVc = glm::dvec3(iMatrix*glm::dvec4(mTriangulation[i*3+2], 0, 1.0));

		switch (iSide)
		{
			case Mesh::FRONT: 
			{
				glm::dvec3 lN; 
				lN.x = iMatrix[0][2];
				lN.y = iMatrix[1][2];
				lN.z = iMatrix[2][2];

				iMesh.pushTriangle(lVa, lVb, lVc, lN); 
			}
			break;

			case Mesh::BACK:
			{
				glm::dvec3 lN; 
				lN.x = -iMatrix[0][2];
				lN.y = -iMatrix[1][2];
				lN.z = -iMatrix[2][2];

				iMesh.pushTriangle(lVc, lVb, lVa, lN); 
			}
			break;
		}
	}
}

void Profile::transform(std::vector<glm::dvec2>& iVertex, glm::dmat3& iMatrix)
{
	for (size_t i=0; i<iVertex.size(); i++)
	{
		iVertex[i] = glm::vec2(iMatrix*glm::dvec3(iVertex[i].x, iVertex[i].y, 1.0));
	};
};

void Profile::transform(glm::dmat3& iMatrix)
{
	transform(mBounds, iMatrix);
	for (std::vector<std::vector<glm::dvec2>>::iterator lIter = mHoles.begin(); lIter !=  mHoles.end(); ++lIter )
	{
		transform((*lIter), iMatrix);
	}
}