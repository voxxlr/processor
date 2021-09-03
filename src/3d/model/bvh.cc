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

#include "bvh.h"
#include "mesh.h"

Bvh::Bvh(int iI0, int iIN, Mesh& iMesh, std::vector<uint32_t>& iIndex) 
: mI0(iI0)
, mIN(iIN)
, mChildL(0)
, mChildH(0)
{
	for (int t=iI0; t<iIN; t++) 
	{
		int lTriangleIndex = iIndex[t];
		mAABB.grow(iMesh[lTriangleIndex*3+0]);
		mAABB.grow(iMesh[lTriangleIndex*3+1]);
		mAABB.grow(iMesh[lTriangleIndex*3+2]);
	}
}
		
json_spirit::mObject Bvh::toJson()
{
	json_spirit::mObject lObject;
	lObject["min"] = mAABB.minJson();
	lObject["max"] = mAABB.maxJson();
	lObject["i0"] = mI0;
	lObject["iN"] = mIN;
			
	if (mChildL)
	{
		lObject["childL"] = mChildL->toJson();
	}
	if (mChildH)
	{
		lObject["childH"] = mChildH->toJson();
	}
	return lObject;
}






Bvh::Comparator::Comparator(int iAxis, Mesh& iMesh)
: mAxis(iAxis)
, mVertex(iMesh)
{
}

bool Bvh::Comparator::operator()(uint32_t i1, uint32_t i2) 
{
	double x1 = (mVertex[i1*3+0][mAxis] + mVertex[i1*3+1][mAxis] + mVertex[i1*3+2][mAxis])/3.0f;
	double x2 = (mVertex[i2*3+0][mAxis] + mVertex[i2*3+1][mAxis] + mVertex[i2*3+2][mAxis])/3.0f;
	return x1 < x2;
};

Bvh::Selector::Selector(int iAxis, Mesh& iMesh)
: mAxis(iAxis)
, mVertex(iMesh)
{
}
		
bool Bvh::Selector::select(uint32_t iI, float iSplit)
{
	return (mVertex[iI*3+0][mAxis] + mVertex[iI*3+1][mAxis] + mVertex[iI*3+2][mAxis])/3.0f < iSplit;
}
	
Bvh* Bvh::construct(Mesh& iMesh, std::vector<uint32_t>* iIndex) 
{
	std::vector<uint32_t> lIndex(iMesh.size());
	for (size_t i=0; i<lIndex.size(); i++)
	{
		lIndex[i] = i;
	}

	Bvh::Comparator lCompareX(0, iMesh);
	Bvh::Comparator lCompareY(1, iMesh);
	Bvh::Comparator lCompareZ(2, iMesh);

	Bvh::Selector lSelectX(0, iMesh);
	Bvh::Selector lSelectY(1, iMesh);
	Bvh::Selector lSelectZ(2, iMesh);

	Bvh* lRoot = new Bvh(0, lIndex.size(), iMesh, lIndex);
 	std::queue<Bvh*> lQueue;
 	lQueue.push(lRoot);
 		
   	while (!lQueue.empty()) 
  	{
  		Bvh* lNode = lQueue.front();
		lQueue.pop();
  			
		glm::dvec3& min = lNode->mAABB.min;
		glm::dvec3 dxyz = lNode->mAABB.max - lNode->mAABB.min;
  			
  		float lSplitPos;
  		Bvh::Selector* lSelector;
  		if (dxyz.x >= dxyz.y && dxyz.x >= dxyz.z) 
  		{
  			lSplitPos = min.x + 0.5*dxyz.x;
  			lSelector = &lSelectX;
			std::sort(lIndex.begin() + lNode->mI0, lIndex.begin() + lNode->mIN, lCompareX);
  		} 
  		else if (dxyz.y >= dxyz.x && dxyz.y >= dxyz.z) 
  		{
  			lSplitPos = min.y + 0.5*dxyz.y;
  			lSelector = &lSelectY;
			std::sort(lIndex.begin() + lNode->mI0, lIndex.begin() + lNode->mIN, lCompareY);
  		} 
  		else 
  		{
  			lSplitPos = min.z + 0.5*dxyz.z;
  			lSelector = &lSelectZ;
			std::sort(lIndex.begin() + lNode->mI0, lIndex.begin() + lNode->mIN, lCompareZ);
  		}
  			
  		int lSplit;
  		for (lSplit = lNode->mI0; lSplit < lNode->mIN; lSplit++) 
  		{
  			if (!lSelector->select(lIndex[lSplit], lSplitPos)) 
  			{
  				break;
  			}
  		}
  		if (lSplit == lNode->mI0 || lSplit == lNode->mIN)
  		{
  			//Log.info("Bad Split ... ");
  			lSplit = (lNode->mI0 + lNode->mIN)/2;
  		}
  			

  		if (lSplit - lNode->mI0 > SPLIT_LIMIT)
  		{
  			lNode->mChildL = new Bvh(lNode->mI0, lSplit, iMesh, lIndex);
  			lQueue.push(lNode->mChildL);
  			lNode->mI0 = lSplit;
  		}
  			
  		if (lNode->mIN - lSplit > SPLIT_LIMIT)
  		{
  			lNode->mChildH = new Bvh(lSplit, lNode->mIN, iMesh, lIndex);
  			lQueue.push(lNode->mChildH);
  			lNode->mIN = lSplit;
  		}
  	}
  		
  	// reorganize triangles to be in order with bvh
	iMesh.shuffle(lIndex);

	if (iIndex)
	{
		reMapIndex(*iIndex, lIndex);
	}

  	return lRoot;
}

void Bvh::reMapIndex(std::vector<uint32_t>& iTarget, std::vector<uint32_t>& iIndex)
{
	std::vector<uint32_t> lTarget(iTarget);
		
	for (size_t t=0; t<iIndex.size(); t++)
	{
		iTarget[t] = lTarget[iIndex[t]];
	}
}
