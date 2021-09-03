#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#define NOMINMAX

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <stdio.h>
#include <string.h>  
#include <stdlib.h>
#include <queue>
#include <set>
#include <map>
#include <tuple>
#include <algorithm>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/common.hpp> 
#include <glm/common.hpp>

#include "bsp.h"
#include "aabb.h"

bool equal(double a, double b, double epsilon) { return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon); }
								
double BspNode::EPSILON = 0.001;  // THIS IS IMPORTANT make sure this comes from FILE

static int s = 0;

BspNode::BspNode(std::list<Triangle>& iIndex, BspVertexCache& iVertexCache, std::string iTree)
: mOutside(0)
, mInside(0)
, mVertexCache(iVertexCache)
, mTree(iTree)
{
	// find largest surface
	typedef std::tuple<glm::dvec4, double> Plane;  
	std::vector<Plane> lTriangles;
	lTriangles.reserve(iIndex.size());
	for (std::list<Triangle>::iterator lIter = iIndex.begin(); lIter != iIndex.end(); lIter++)
	{
		glm::dvec3& v0 = iVertexCache[std::get<0>(*lIter)];
		glm::dvec3& v1 = iVertexCache[std::get<1>(*lIter)];
		glm::dvec3& v2 = iVertexCache[std::get<2>(*lIter)];
		glm::dvec3 e10 = v1 - v0;
		glm::dvec3 e21 = v2 - v0;
		glm::dvec3 n = glm::cross(e10, e21);

		double l = glm::length(n);
		n/=l;
		lTriangles.push_back(Plane(glm::dvec4(n, glm::dot(n, glm::dvec3(v0))), 0.5*l));  // plane + triangle area
	}

	// will speed up search a bit
	std::sort (lTriangles.begin(), lTriangles.end(), [](Plane& a, Plane& b) { return std::get<0>(a).w < std::get<0>(b).w; });

	// sum up area of coplanar triangles
	std::vector<Plane>::iterator lTriangle = lTriangles.begin();
	std::vector<Plane> lPlanes;

	lPlanes.push_back(Plane(std::get<0>(*lTriangle), std::get<1>(*lTriangle)));
	lTriangle = lTriangles.erase(lTriangle);
	while (lTriangle != lTriangles.end())
	{
		do 
		{
			//std::cout << std::get<0>(lPlanes.back()) << "\n";
			if (equal(std::get<0>(lPlanes.back()).w, std::get<0>(*lTriangle).w, EPSILON))
			{
				if (equal(fabs(glm::dot(glm::dvec3(std::get<0>(lPlanes.back())), glm::dvec3(std::get<0>(*lTriangle)))), 1, EPSILON))
				{
					// add area to back
					std::get<1>(lPlanes.back()) += std::get<1>(*lTriangle);
					lTriangle = lTriangles.erase(lTriangle);
				}
				else
				{
					lTriangle++;
				}
			}
			else
			{
				break;
			}
		}
		while (lTriangle != lTriangles.end());

		lTriangle = lTriangles.begin();
		if (lTriangle != lTriangles.end())
		{
			lPlanes.push_back(Plane(std::get<0>(*lTriangle), std::get<1>(*lTriangle)));
			lTriangle = lTriangles.erase(lTriangle);
		}
	}

	// sort by area 
	std::sort (lPlanes.begin(), lPlanes.end(), [](Plane& a, Plane& b) { return std::get<1>(a) > std::get<1>(b); });

	// randomly select from equal sized split planes
	std::vector<Plane>::iterator lPlane = lPlanes.begin();
	double lMax = std::get<1>(*lPlane++);
	uint32_t lRange = 1;
	while (lPlane != lPlanes.end())
	{
		if (!equal(lMax, std::get<1>(*lPlane++), EPSILON))
		{
			break;
		}
		lRange++;
	}

	//glm::dvec4 lSplit = std::get<0>(lPlanes[rand()%lRange]); 
	glm::dvec4 lSplit = std::get<0>(lPlanes[3%lRange]); 
	mN = glm::dvec3(lSplit);
	mW = lSplit.w;
	
	std::list<Triangle> lOutside;
	std::list<Triangle> lInside;
	for (std::list<Triangle>::iterator lIter = iIndex.begin(); lIter != iIndex.end(); lIter++)
	{
		glm::uvec3 lSides = span(*lIter);
		switch (lSides[0] | lSides[1] | lSides[2])
		{
			case COPLANAR: 
				{
					// record direction of triangle on plane
					glm::dvec3& v0 = iVertexCache[std::get<0>(*lIter)];
					glm::dvec3& v1 = iVertexCache[std::get<1>(*lIter)];
					glm::dvec3& v2 = iVertexCache[std::get<2>(*lIter)];
					glm::dvec3 e10 = v1 - v0;
					glm::dvec3 e21 = v2 - v0;
					glm::dvec3 n = glm::normalize(glm::cross(e10, e21));

					std::get<3>(*lIter) = glm::dot(n, mN) > 0 ? true : false;
					mCoplanar.push_back(*lIter);
				}
				break;
			case FRONT: 
				lOutside.push_back(*lIter); 
				break;
			case BACK: 
				lInside.push_back(*lIter); 
				break;
			case SPANNING: 
				split(*lIter, lSides, lOutside, lInside); 
				break;
		}
	}

	if (lOutside.size())
	{
		mOutside = new BspNode(lOutside, iVertexCache, mTree);
   }

	if (lInside.size())
	{
		mInside = new BspNode(lInside, iVertexCache, mTree);
	}
}

BspNode::~BspNode()
{
	if (mOutside)
	{
		delete mOutside;
	};

	if (mInside)
	{
		delete mInside;
	};
}

glm::dvec3& BspNode::intersect(uint32_t ia, uint32_t ib)
{ 
	static glm::dvec3 sVertex;
	glm::dvec3& va = mVertexCache[ia]; 
	glm::dvec3& vb = mVertexCache[ib];
	glm::dvec3 e = vb - va;
	double t = (mW - glm::dot(mN,va)) / glm::dot(mN, e);
	sVertex = va + t*e;
	return sVertex;
};

uint32_t BspNode::side(uint32_t i0)
{
	double lT = glm::dot(mN, mVertexCache[i0]) - mW;
	return lT < -EPSILON ? BACK : lT > EPSILON ? FRONT : COPLANAR;
}

glm::uvec3& BspNode::span(Triangle iTriangle)
{
	static glm::uvec3 sSide;
	sSide[0]  = side(std::get<0>(iTriangle));
	sSide[1]  = side(std::get<1>(iTriangle));
	sSide[2]  = side(std::get<2>(iTriangle));
	return sSide;
}

void BspNode::split(Triangle iTriangle, glm::uvec3 iSides, std::list<Triangle>&  iOutside, std::list<Triangle>&  iInside)
{
	glm::uvec3 ijk(std::get<0>(iTriangle), std::get<1>(iTriangle), std::get<2>(iTriangle));

	// split int 2 or 3 triangles
	for (int i=0; i<3; i++)
	{
		int j=(i+1)%3;
		int k=(i+2)%3;

		if ( (iSides[i] | iSides[j]) == SPANNING)
		{
			uint32_t vi = ijk[i];
			uint32_t vj = ijk[j];
			uint32_t vk = ijk[k];

			uint32_t vij = mVertexCache.insert(intersect(vi, vj));

			if (iSides[k] == COPLANAR)
			{
				if (iSides[i] == FRONT)
				{
					iOutside.push_back(Triangle(vi, vij, vk, std::get<3>(iTriangle)));
					iInside.push_back(Triangle(vij, vj, vk, std::get<3>(iTriangle)));
				}
				else
				{
					iInside.push_back(Triangle(vi, vij, vk, std::get<3>(iTriangle)));
					iOutside.push_back(Triangle(vij, vj, vk, std::get<3>(iTriangle)));
				}
			}
			else if (iSides[k] == iSides[i])
			{
				uint32_t vjk = mVertexCache.insert(intersect(vj, vk));

				#define SPLIT(listA,listB) \
					listA.push_back(Triangle(vij, vj, vjk, std::get<3>(iTriangle))); \
					if (abs(glm::dot(mN, mVertexCache[i])) < abs(glm::dot(mN, mVertexCache[k]))) \
					{ \
						listB.push_back(Triangle(vi, vij, vjk, std::get<3>(iTriangle))); \
						listB.push_back(Triangle(vjk, vk, vi, std::get<3>(iTriangle))); \
					} \
					else \
					{ \
						listB.push_back(Triangle(vjk, vk, vij, std::get<3>(iTriangle))); \
						listB.push_back(Triangle(vi, vij, vk, std::get<3>(iTriangle))); \
					} \

				if (iSides[i] == FRONT)
				{
					SPLIT(iInside, iOutside)
				}
				else
				{
					SPLIT(iOutside, iInside)
				}

				#undef	SPLIT
			}
			else if (iSides[k] == iSides[j])
			{
				uint32_t vki = mVertexCache.insert(intersect(vk, vi));

				#define SPLIT(listA,listB) \
					listA.push_back(Triangle(vi, vij, vki, std::get<3>(iTriangle))); \
					if (abs(glm::dot(mN, mVertexCache[j])) < abs(glm::dot(mN, mVertexCache[k]))) \
					{ \
						listB.push_back(Triangle(vij, vj, vki, std::get<3>(iTriangle))); \
						listB.push_back(Triangle(vk, vki, vj, std::get<3>(iTriangle))); \
					} \
					else \
					{ \
						listB.push_back(Triangle(vk, vki, vij, std::get<3>(iTriangle))); \
						listB.push_back(Triangle(vij, vj, vk, std::get<3>(iTriangle))); \
					} \

				if (iSides[i] == FRONT)
				{
					SPLIT(iOutside, iInside)
				}
				else
				{
					SPLIT(iInside, iOutside)
				}
				
				#undef	SPLIT
			}
			break;

			#undef TRIANGLE
		}
	}
}

void BspNode::invert()
{
	for (std::list<Triangle>::iterator lIter = mCoplanar.begin(); lIter != mCoplanar.end(); lIter++)
	{
		std::swap(std::get<2>(*lIter), std::get<0>(*lIter));
		//std::get<3>(*lIter) = !std::get<3>(*lIter);
	}

	mN = -mN;
	mW = -mW;

	if (mOutside)
	{
		mOutside->invert();
	}

	if (mInside)
	{
		mInside->invert();
	}

	std::swap(mOutside, mInside);
};

void BspNode::clipBy(uint8_t iSide, BspNode& iTree)
{
	std::list<Triangle> lCoplanar;
	for (std::list<Triangle>::iterator lIter = mCoplanar.begin(); lIter != mCoplanar.end(); lIter++)
	{
		glm::dvec3 lN = std::get<3>(*lIter) ? mN : -mN;
		lCoplanar.splice(lCoplanar.end(), iTree.clip(*lIter, lN, iSide));
	}
	mCoplanar.clear();
	mCoplanar.splice(mCoplanar.end(), lCoplanar);

	if (mOutside)
	{
		mOutside->clipBy(iSide, iTree);
	}

	if (mInside)
	{
		mInside->clipBy(iSide, iTree);
	}
}

std::list<BspNode::Triangle> BspNode::clip(Triangle& iTriangle, glm::dvec3& iNormal, uint8_t iSide) 
{
	std::list<Triangle> lResult;

	glm::uvec3 lSpan = span(iTriangle);

	switch (lSpan[0] | lSpan[1] | lSpan[2])
	{
		case COPLANAR: 
			if (glm::dot(iNormal, mN) > 0)
			{
				if (mOutside)
				{
					return mOutside->clip(iTriangle, iNormal, iSide);
				}
				else if (iSide == INSIDE)
				{
					lResult.push_back(iTriangle);
				}
			}
			else
			{
				if (mInside)
				{
					return mInside->clip(iTriangle, iNormal, iSide);
				}
				else if (iSide == OUTSIDE)
				{
					lResult.push_back(iTriangle);
				}
			}
			break;
		case FRONT: 
			if (mOutside)
			{
				return mOutside->clip(iTriangle, iNormal, iSide);
			}
			else if (iSide == INSIDE)
			{
				lResult.push_back(iTriangle);
			}
			break;
		case BACK: 
			if (mInside)
			{
				return mInside->clip(iTriangle, iNormal, iSide);
			}
			else if (iSide == OUTSIDE)
			{
				lResult.push_back(iTriangle);
			}
			break;
		case SPANNING: 
			{
				bool lClipped = false;
			    std::list<Triangle> lFrontSplit; 
				std::list<Triangle> lBackSplit;
				split(iTriangle, lSpan, lFrontSplit, lBackSplit); 

				std::list<Triangle> lFront;
				if (mOutside)
				{
					for (std::list<Triangle>::iterator lIter = lFrontSplit.begin(); lIter != lFrontSplit.end(); lIter++)
					{
						lFront.splice(lFront.end(), mOutside->clip(*lIter, iNormal, iSide));
					}

					if (lFront != lFrontSplit)
					{
						lClipped = true;
					}
				}
				else if (iSide == OUTSIDE)
				{
					lFrontSplit.clear();
					lClipped = true;
				}
				else
				{
					lFront.splice(lFront.end(), lFrontSplit);
				}

				std::list<Triangle> lBack;
				if (mInside)
				{
					for (std::list<Triangle>::iterator lIter = lBackSplit.begin(); lIter != lBackSplit.end(); lIter++)
					{
						lBack.splice(lBack.end(), mInside->clip(*lIter, iNormal, iSide));
					}

					if (lBack != lBackSplit)
					{
						lClipped = true;
					}
				}
				else if (iSide == INSIDE)
				{
					lBackSplit.clear();
					lClipped = true;
				}
				else
				{
					lBack.splice(lBack.end(), lBackSplit);
				}
				
				if (lClipped)
				{
					lResult.splice(lResult.end(), lFront);
					lResult.splice(lResult.end(), lBack);
				}
				else 
				{
					lResult.push_back(iTriangle);
				}
			}
			break;
	}

	return lResult;
};

void BspNode::collapse(std::vector<BspNode*>& iList) 
{
	iList.push_back(this);

	if (mOutside)
	{
		mOutside->collapse(iList);
	}

	if (mInside)
	{
		mInside->collapse(iList);
	}
}



//#define DEBUG 1

#define M_QRT2 1.25992104989

BspMesh::BspMesh(glm::dvec3 iN, double iW, BspVertexCache& iVertexCache)
: mN(iN)
, mW(iW)
, mVertexCache(iVertexCache)
{
}

void BspMesh::write(Mesh& iMesh)
{
	if (mList.size() > 2)//  && glm::all(glm::epsilonEqual(mN, glm::dvec3(0, 0, -1), 0.00001)))
	{
		#ifdef DEBUG
		BOOST_LOG_TRIVIAL(info) << mN.x << ", " << mN.y << ", " << mN.z << " --- " << "   with " << mList.size() << " triangles";
		#endif
		
		// create graph
		std::map<Edge, uint32_t> lGraph;
		for (std::vector<Triangle>::iterator lIter = mList.begin(); lIter != mList.end(); lIter++)
		{
			uint32_t i0 = std::get<0>(*lIter);
			uint32_t i1 = std::get<1>(*lIter);
			uint32_t i2 = std::get<2>(*lIter);

			if (i0 != i1 && i1 != i2 && i2 != i0)
			{
				lGraph[Edge(i0, i1)] = i2;
				lGraph[Edge(i1, i2)] = i0;
				lGraph[Edge(i2, i0)] = i1;
			}
		}

		// PASS 1 ---  find all T junctions

		// find all outer edges
		std::set<Edge> lOuterEdges;
		for (std::map<Edge, uint32_t>::iterator lIter = lGraph.begin(); lIter != lGraph.end(); lIter++)
		{
			Edge lEdge = lIter->first;
			if (lGraph.find(Edge(std::get<1>(lEdge), std::get<0>(lEdge))) == lGraph.end()) 
			{
				lOuterEdges.insert(lEdge);
			}
		}

		std::set<Edge> lPending;
		lPending.insert(lOuterEdges.begin(), lOuterEdges.end());
	
		while (!lPending.empty())   // JSTIER 
		{
			Edge lEdge = *lPending.begin();
			lPending.erase(lPending.begin());
			uint32_t i0 = std::get<0>(lEdge);
			uint32_t i1 = std::get<1>(lEdge);
			uint32_t i2 = lGraph[lEdge];

			glm::dvec3 d = mVertexCache[i1] - mVertexCache[i0];
			glm::dvec3 v0 = mVertexCache[i0];
			double l = glm::dot(d,d);

			std::set<Edge>::iterator lIter = lOuterEdges.begin();
			while (lIter != lOuterEdges.end())
			{
				if (*lIter != Edge(i0,i1) && *lIter != Edge(i1,i2) && *lIter != Edge(i2,i0))
				{
					Edge lSample = *lIter;
					uint32_t s0 = std::get<0>(lSample);
					uint32_t s1 = std::get<1>(lSample);
					uint32_t s2 = lGraph[lSample];

					#define	TRIANGLE(a,b,c) lGraph[Edge(a, b)] = c; lGraph[Edge(b, c)] = a;  lGraph[Edge(c, a)] = b; //std::cout << "adding (" << a <<"," << b << "," << c << ")\n";

					#define	SPLIT(va,vb,vc,vs) TRIANGLE(va,vs,vc) TRIANGLE(vs,vb,vc);
					#define	ERASE(a,b)  lOuterEdges.erase(Edge(a, b));  lGraph.erase(Edge(a,b)); 
					#define	SCHEDULE(a,b)  lOuterEdges.insert(Edge(a, b));  lPending.insert(Edge(a, b));  

					if (s0 == i1)
					{
						// is s1 close 
						double t = glm::dot(mVertexCache[s1]-v0,d)/l;
						if (t > 0.0 && t < 1.0)
						{
							if (glm::distance(mVertexCache[s1], v0 + t*d) < mVertexCache.mResolution)
							{
								if (s2 != i2)
								{
									SPLIT(i0,i1,i2,s1)
									ERASE(i0,i1);
									SCHEDULE(i0, s1);
								}
								break;
							}
						}
					}
					else if (s1 == i0)
					{
						// is s0 close 
						double t = glm::dot(mVertexCache[s0]-v0,d)/l;
						if (t > 0.0 && t < 1.0) 
						{
							if (glm::distance(mVertexCache[s0], v0 + t*d) < mVertexCache.mResolution)
							{
								if (s2 != i2)
								{
									SPLIT(i0,i1,i2,s0)
									ERASE(i0,i1);
									SCHEDULE(s0, i1);
								}
								break;
							}
						}
					}

					#undef	TRIANGLE
					#undef	ERASE
					#undef	SCHEDULE
				}
				lIter++;
			}
		}

		// PASS 2 --- Remove all nestest triangles
		int lMerged = 0;
		do
		{
			lMerged = 0;

			// find inner edges
			std::set<Edge> lInnerEdges;
			for (std::map<Edge, uint32_t>::iterator lIter = lGraph.begin(); lIter != lGraph.end(); lIter++)
			{
				Edge lEdge = lIter->first;
				if (lGraph.find(Edge(std::get<1>(lEdge), std::get<0>(lEdge))) != lGraph.end()) 
				{
					lInnerEdges.insert(lEdge);
				}
			}

			while (!lInnerEdges.empty())   
			{
				Edge lEdge = *lInnerEdges.begin();

				uint32_t a0 = std::get<0>(lEdge);
				uint32_t a1 = std::get<1>(lEdge);
				uint32_t a2 = lGraph[Edge(a0,a1)];
				lInnerEdges.erase(Edge(a0,a1));

				uint32_t b0 = std::get<1>(lEdge);
				uint32_t b1 = std::get<0>(lEdge);
				uint32_t b2 = lGraph[Edge(b0,b1)]; 
				lInnerEdges.erase(Edge(b0,b1));
	
				#define COLINEAR(e0,e1) (glm::length2(glm::cross(e1, e0))/glm::length2(e0) < BspNode::EPSILON*BspNode::EPSILON)
	
				glm::dvec3 eA = mVertexCache[a0]-mVertexCache[a2];
				glm::dvec3 eB = mVertexCache[b2]-mVertexCache[b1];
				if (COLINEAR(eA,eB))
				{
					lGraph.erase(Edge(a0,a1));
					lGraph.erase(Edge(b0,b1));

					lGraph.erase(Edge(a2,a0));
					lInnerEdges.erase(Edge(a2,a0));
					lInnerEdges.erase(Edge(a0,a2));
					lGraph.erase(Edge(b1,b2));
					lInnerEdges.erase(Edge(b1,b2));
					lInnerEdges.erase(Edge(b2,b1));

					lGraph[Edge(a1,a2)] = b2;
					lGraph[Edge(a2,b2)] = b0;
					lGraph[Edge(b2,b0)] = a2;

					lMerged++;
					continue;
				}

				eA = mVertexCache[a1]-mVertexCache[a2];
				eB = mVertexCache[b2]-mVertexCache[b0];
				if (COLINEAR(eA,eB))
				{
					lGraph.erase(Edge(a0,a1));
					lGraph.erase(Edge(b0,b1));

					lGraph.erase(Edge(a1,a2));
					lInnerEdges.erase(Edge(a1,a2));
					lInnerEdges.erase(Edge(a2,a1));

					lGraph.erase(Edge(b2,b0));
					lInnerEdges.erase(Edge(b2,b0));
					lInnerEdges.erase(Edge(b0,b2));

					lGraph[Edge(b1,b2)] = a2;
					lGraph[Edge(b2,a2)] = a0;
					lGraph[Edge(a2,a0)] = b2;

					lMerged++;
					continue;
				}

				// quad
				if (lGraph.find(Edge(b0,b2)) != lGraph.end())
				{
					uint32_t c0 = b0;
					uint32_t c1 = b2;
					uint32_t c2 = lGraph[Edge(c0,c1)]; 

					eA = mVertexCache[a1]-mVertexCache[a2];
					glm::dvec3 eC = mVertexCache[c2]-mVertexCache[c0];

					if (COLINEAR(eA,eC))
					{
						lGraph.erase(Edge(a0,a1));
						lGraph.erase(Edge(b0,b1));

						lGraph.erase(Edge(b2,b0));
						lInnerEdges.erase(Edge(b2,b0));
						lGraph.erase(Edge(c0,c1));
						lInnerEdges.erase(Edge(c0,c1));

						lGraph.erase(Edge(a1,a2));
						lInnerEdges.erase(Edge(a1,a2));
						lInnerEdges.erase(Edge(a2,a1));

						lGraph.erase(Edge(c2,c0));
						lInnerEdges.erase(Edge(c2,c0));
						lInnerEdges.erase(Edge(c0,c2));

						if (glm::length(mVertexCache[c1]-mVertexCache[c0]) > glm::length(mVertexCache[a1]-mVertexCache[a0]))
						{
							lGraph[Edge(a0,c1)] = c2;
							lGraph[Edge(c1,c2)] = a0;
							lGraph[Edge(c2,a0)] = c1;

							lGraph[Edge(a0,c2)] = a2;
							lGraph[Edge(c2,a2)] = a0;
							lGraph[Edge(a2,a0)] = c2;
						}
						else
						{
							lGraph[Edge(c1,a2)] = a0;
							lGraph[Edge(a2,a0)] = c1;
							lGraph[Edge(a0,c1)] = a2;

							lGraph[Edge(a2,c1)] = c2;
							lGraph[Edge(c1,c2)] = a2;
							lGraph[Edge(c2,a2)] = c1;
						}
						lMerged++;
						continue;
					}
				}
			}
		}
		while  (lMerged != 0);

		while (!lGraph.empty())
		{
			std::map<Edge, uint32_t>::iterator lIter = lGraph.begin();

			uint32_t i0 = std::get<0>(lIter->first);
			uint32_t i1 = std::get<1>(lIter->first);
			uint32_t i2 = lIter->second;

			iMesh.pushTriangle(mVertexCache[i0], mVertexCache[i1], mVertexCache[i2], mN);

			lGraph.erase(Edge(i0, i1));
			lGraph.erase(Edge(i1, i2));
			lGraph.erase(Edge(i2, i0));
		}
	}
	else
	{
		for (std::vector<Triangle>::iterator lIter = mList.begin(); lIter != mList.end(); lIter++)
		{
			iMesh.pushTriangle(mVertexCache[std::get<0>(*lIter)], mVertexCache[std::get<1>(*lIter)], mVertexCache[std::get<2>(*lIter)], mN);
		}
	}
};


void BspTree::initIndex(Mesh& iMesh, std::list<BspNode::Triangle>& iIndex, BspVertexCache& iVertexCache)
{
	for (size_t i=0; i<iMesh.size(); i++)
	{
		uint32_t i0 = iVertexCache.insert(iMesh[i*3+0]);
		uint32_t i1 = iVertexCache.insert(iMesh[i*3+1]);
		uint32_t i2 = iVertexCache.insert(iMesh[i*3+2]);

		if (i0 != i1 && i1 != i2 && i2 != i0)
		{
			BspNode::Triangle lTriangle(i0,i1,i2, true);
			iIndex.push_back(lTriangle);
		}
	}
}

void BspTree::write(std::vector<BspNode*>& iList0, BspVertexCache& iVertexCache, Mesh& iMesh)
{
	std::sort (iList0.begin(), iList0.end(), [](BspNode* a, BspNode* b) { return a->mW < b->mW; });    

	std::vector<BspNode*> lList1;

	// group coplanar nodes
	std::vector<BspNode*>::iterator lIter = iList0.begin();
	BspNode* lCurrent = *lIter;
	lList1.push_back(lCurrent);
	lIter = iList0.erase(lIter);
	while (lIter != iList0.end())
	{
		do 
		{
			BspNode* lNext = *lIter;
			if (equal(lCurrent->mW, lNext->mW, 0.00001) && equal(glm::dot(lCurrent->mN, lNext->mN), 1, 0.00001))
			{
				lCurrent->mCoplanar.insert(lCurrent->mCoplanar.end(), lNext->mCoplanar.begin(), lNext->mCoplanar.end());
				lIter = iList0.erase(lIter);
			}
			else
			{
				lIter++;
			}
		}
		while (lIter != iList0.end());

		lIter = iList0.begin();
		if (lIter != iList0.end())
		{
			lCurrent = *lIter;
			lList1.push_back(lCurrent);
			lIter = iList0.erase(lIter);
		}
	}

	// split coplanar nodes into front an back facing
	iMesh.clear();
	for (size_t i=0; i<lList1.size(); i++)
	{
		BspNode* lNode = lList1[i];

		BspMesh lFront(lNode->mN, lNode->mW, iVertexCache);
		BspMesh lBack(-lNode->mN, lNode->mW, iVertexCache);
		for (std::list<BspNode::Triangle>::iterator lIter = lNode->mCoplanar.begin(); lIter != lNode->mCoplanar.end(); lIter++)
		{
			if (std::get<3>(*lIter))
			{
				lFront.mList.push_back(BspMesh::Triangle(*lIter));
			}
			else
			{
				lBack.mList.push_back(BspMesh::Triangle(*lIter));
			}
		}

		lFront.write(iMesh);
		lBack.write(iMesh);
	}
};


// Replace iA with CSG solid representing space in either this solid or in the
// solid `csg`. 
// 
//     A.union(B)
// 
//     +-------+            +-------+
//     |       |            |       |
//     |   A   |            |       |
//     |    +--+----+   =   |       +----+
//     +----+--+    |       +----+       |
//          |   B   |            |       |
//          |       |            |       |
//          +-------+            +-------+
// 

void BspTree::combine(Mesh& iA, Mesh& iB, double iPrecision)
{
	/* Test 1 A intersects B
	iA.clear();
	Mesh::box(iA, glm::dvec3(-0.5,-0.5,-0.5), glm::dvec3(2,2,2));
	iB.clear();
	Mesh::box(iB, glm::dvec3( 0.5, 0.5, 0.5), glm::dvec3(2,2,2));
	 */
	/* Test 2a B in A same height 	
	iA.clear();
	Mesh::box(iA, glm::dvec3(-1.0,0,0), glm::dvec3(3,1,1));
	iB.clear();
	Mesh::box(iB, glm::dvec3(-0.5,0,0), glm::dvec3(1,1,1));
	 */
	/* Test 2b A in B same height 	
	iA.clear();
	Mesh::box(iA, glm::dvec3(-0.5,0,0), glm::dvec3(1,1,1));
	iB.clear();
	Mesh::box(iB, glm::dvec3(-1.0,0,0), glm::dvec3(3,1,1));
	*/
	/* Test 3 B entirely inside A 
	iA.clear();
	Mesh::box(iA, glm::dvec3(-1.0,-1.0,-1.0), glm::dvec3(3,3,3));
	iB.clear();
	Mesh::box(iB, glm::dvec3(-0.5,-0.5,-0.5), glm::dvec3(1,1,1));
	*/
	/* Test 4 B adjacent to A 
	iA.clear();
	Mesh::box(iA, glm::dvec3(-0.5,0,0), glm::dvec3(1,1,1));
	iB.clear();
	Mesh::box(iB, glm::dvec3( 0.5,0,0), glm::dvec3(1,1,1));
	*/
	/* Test 5 B == A 
	iA.clear();
	Mesh::box(iA, glm::dvec3(0.0,0,0), glm::dvec3(1,1,1));
	iB.clear();
	Mesh::box(iB, glm::dvec3(0.0,0,0), glm::dvec3(1,1,1));
	*/
	/*
	iA.clear();
	//Mesh::rect(iA, Mesh::XY_PLANE,  glm::dvec3(0, 0, 0.5), glm::dvec2(20));
	Mesh::triangle(iA, Mesh::XY_PLANE,  glm::dvec3(0, 0, 0.5), glm::dvec3(20, 20, 20));
	//Mesh::box(iA, glm::dvec3(0), glm::dvec3(20));
	iB.clear();
	Mesh::cylinder(iB,glm::dvec3(0,0,1), glm::dvec2(2.5,14));
	//Mesh::cylinder(iB,glm::dvec3(5.0,-0.185,1.3), glm::dvec2(0.5,0.485));
	//Mesh::cylinder(iB,glm::dvec3(0,0,-25), glm::dvec2(0.5,50));
	//Mesh::sphere(iB, glm::dvec3(-10,-10,-10), glm::dvec3(20));
	*/
	//iA.clear();
	//Mesh::box(iA, glm::dvec3(-0.5,-0.5,-0.5), glm::dvec3(0.5,0.5,0.5));
	//iB.clear();
	//Mesh::box(iB, glm::dvec3( 0.5, 0.5, 4.5), glm::dvec3(1,1,1));
//	BOOST_LOG_TRIVIAL(info) << "-----combining---------------------";
	srand(13);
//	iA.append(iB);
//	return;

	if (iA.size() && iB.size())
	{
		BspVertexCache lVertexCache(iPrecision);

		BspNode::EPSILON = sqrt(iPrecision);

		std::list<BspNode::Triangle> lIa;
		initIndex(iA, lIa, lVertexCache); 
		BspNode lA(lIa, lVertexCache);
		AABB lAbox(iA.vertexBuffer());

		std::list<BspNode::Triangle> lIb;
		initIndex(iB, lIb, lVertexCache);
		BspNode lB(lIb, lVertexCache);
		AABB lBbox(iB.vertexBuffer());
		
		// combine
		lA.clipBy(BspNode::INSIDE, lB);
		lB.clipBy(BspNode::INSIDE, lA);
		
		// cleanup coplanar
		lB.invert();
		lB.clipBy(BspNode::INSIDE, lA);
		lB.invert();

		std::vector<BspNode*> lList;
		lA.collapse(lList);
		lB.collapse(lList);
		BspTree::write(lList, lVertexCache, iA);
	}
	else if (iB.size())
	{
		iA.append(iB);
	}
};

// Replace iA with CSG solid representing space in this solid but not in the
// solid `csg`. 
// 
//     A.subtract(B)
// 
//     +-------+            +-------+
//     |       |            |       |
//     |   A   |            |       |
//     |    +--+----+   =   |    +--+
//     +----+--+    |       +----+
//          |   B   |
//          |       |
//          +-------+
// 


void BspTree::subtract(Mesh& iA, Mesh& iB, double iPrecision, int iDebug)
{
	/* TEST 1
	iA.clear();
	iB.clear();
	Mesh::box(iA, glm::dvec3(0), glm::dvec3(1,1,2));
	//Mesh::rect(iA, Mesh::XY_PLANE,  glm::dvec3(0), glm::dvec2(3));
	Mesh::rect(iB, Mesh::YZ_PLANE,  glm::dvec3(0), glm::dvec2(3));
	 
	iA.clear();
	iB.clear();
	Mesh::box(iA, glm::dvec3(0), glm::dvec3(1.0));
	Mesh::box(iB, glm::dvec3(0,0,1), glm::dvec3(1.0));
	*/
	/*
	iA.clear();
	iB.clear();
	Mesh::box(iA, glm::dvec3(0), glm::dvec3(1.3));
	Mesh::sphere(iB, glm::dvec3(0.0,0.0,0.0), glm::dvec3(0.8));
		*/  
	
	/*
	This will create degenerates with BspNode::EPSILON = 0.0001;  --> 	remove fricking hack test
			iA.clear();
			iB.clear();
			Mesh::box(iA, glm::dvec3(0), glm::dvec3(1.3));
			Mesh::sphere(iB, glm::dvec3(-0.5,-0.5,-0.5), glm::dvec3(0.6));
			Mesh::sphere(iB, glm::dvec3(0.5,0.5,0.5), glm::dvec3(1));
			 */
			
	/* DEMO 3
	// no operatioons only wire only write B 
	iB.clear();
	Mesh::sphere(iB, glm::dvec3(0.5,0.5,0.5), glm::dvec3(1));
	Mesh::sphere(iB, glm::dvec3(-0.5,-0.5,-0.5), glm::dvec3(0.6));
	*/

	//iA.clear();
	//Mesh::rect(iA, Mesh::XY_PLANE,  glm::dvec3(0,-0.5, 0), glm::dvec2(20));

	//iA.clear();
	//Mesh::rect(iA, Mesh::XY_PLANE,  glm::dvec3(0, 0, 0.5), glm::dvec2(20));
	//Mesh::triangle(iA, Mesh::XY_PLANE,  glm::dvec3(0, 0, 0.5), glm::dvec3(20, 20, 20));
	//Mesh::box(iA, glm::dvec3(0), glm::dvec3(20));
	//iB.clear();
	//Mesh::cylinder(iB,glm::dvec3(0,0,-25), glm::dvec2(2.5,50));
	//Mesh::cylinder(iB,glm::dvec3(5.0,-0.185,1.3), glm::dvec2(0.5,0.485));
	//Mesh::cylinder(iB,glm::dvec3(0,0,-25), glm::dvec2(0.5,50));
	//Mesh::sphere(iB, glm::dvec3(-10,-10,-10), glm::dvec3(20));
	/*
	iA.clear();
	Mesh::box(iA, glm::dvec3(-0.5,0,0), glm::dvec3(1,1,1));
	iB.clear();
	Mesh::box(iB, glm::dvec3(0.5,0,0), glm::dvec3(1,1,1));
	*/

	/*
	if (iDebug != -1)
	{
	}
	else
	{
	}
	iA.append(iB);
	return;
	*/
	/*
	if (s > 4)
	{
		if (s==5)
		{
			iA.clear();
			iA.append(iB);
		}
		return;
	}
	*/

	/*
	s++;

	if (s == 1 || s == 3)
	{
		return;
	}

	AABB laa(iA.vertexBuffer());
	std::cout << " A \n" << laa << "\n";
	AABB lbb(iB.vertexBuffer());
	std::cout << " B \n" << lbb << "\n";
	*/

	
	//if (iDebug== -1)
	//{
		//iA.clear();
		//iA.append(iB);
		//return;
	//}
	  
	srand(12);
	if (iA.size() && iB.size())
	{
		BspVertexCache lVertexCache(iPrecision);

		BspNode::EPSILON = sqrt(iPrecision);

		std::list<BspNode::Triangle> lIa;
		initIndex(iA, lIa, lVertexCache);
		BspNode lA(lIa, lVertexCache,"A");
		std::list<BspNode::Triangle> lIb;
		initIndex(iB, lIb, lVertexCache);
		BspNode lB(lIb, lVertexCache,"B");
		
		// subtract
		lA.clipBy(BspNode::INSIDE, lB);
		lB.clipBy(BspNode::OUTSIDE, lA);
		lB.invert();
		
		// cleanup coplanar
		lA.invert();
		lA.clipBy(BspNode::OUTSIDE, lB);
		lA.invert();
		
		std::vector<BspNode*> lList;
		lA.collapse(lList);
		lB.collapse(lList);
		BspTree::write(lList, lVertexCache, iA);
	}
};


  // Replace iA with CSG solid representing space both this solid and in the
  // solid `csg`. Neither this solid nor the solid `csg` are modified.
  // 
  //     A.intersect(B)
  // 
  //     +-------+
  //     |       |
  //     |   A   |
  //     |    +--+----+   =   +--+
  //     +----+--+    |       +--+
  //          |   B   |
  //          |       |
  //          +-------+
  // 

void BspTree::intersect(Mesh& iA, Mesh& iB, double iPrecision)
{
	srand(12);
	if (iA.size() && iB.size())
	{
		BspVertexCache lVertexCache(iPrecision);

		BspNode::EPSILON = sqrt(iPrecision);

		std::list<BspNode::Triangle> lIa;
		initIndex(iA, lIa, lVertexCache);
		BspNode lA(lIa, lVertexCache);
		std::list<BspNode::Triangle> lIb;
		initIndex(iB, lIb, lVertexCache);
		BspNode lB(lIb, lVertexCache);

		// intersect
		lA.invert();
		lA.clipBy(BspNode::OUTSIDE, lB);
		lA.invert();
		lB.clipBy(BspNode::OUTSIDE, lA);
		
		// cleanup coplanar
		lB.invert();
		lB.clipBy(BspNode::OUTSIDE, lA);
		lB.invert();

		std::vector<BspNode*> lList;
		lA.collapse(lList);
		lB.collapse(lList);
		BspTree::write(lList, lVertexCache, iA);
	}
};


BspVertexCache::BspVertexCache(double iPrecision)
: mIndex(SIZE, GridPoint(0,0,0,false, 0))
, mResolution(iPrecision)
{
	//std::cout << iPrecision << "\n";
};

//const double BspVertexCache::RESOLUTION = 0.0001;  // TODO make this dependant on AABB size

uint32_t BspVertexCache::insert(glm::dvec3& iVertex)
{
	int64_t gx = floor(iVertex.x/mResolution +0.5);
	int64_t gy = floor(iVertex.y/mResolution +0.5);
	int64_t gz = floor(iVertex.z/mResolution +0.5);

	iVertex.x = gx*mResolution;
	iVertex.y = gy*mResolution;
	iVertex.z = gz*mResolution;
	uint32_t lIndex = ((uint64_t)((gx)^(gy)^(gz)))%SIZE;
	while (1)
	{
		GridPoint lGridPoint = mIndex[lIndex];
		if (std::get<3>(lGridPoint))
		{
			if (std::get<0>(lGridPoint) == gx && std::get<1>(lGridPoint) == gy && std::get<2>(lGridPoint) == gz)
			{
				return std::get<4>(lGridPoint);
			}
		}
		else
		{
			mIndex[lIndex] = GridPoint(gx,gy,gz,true, mVertex.size());
			mVertex.push_back(iVertex);
			return mVertex.size()-1;
		}
		lIndex = (lIndex+1)%SIZE;
	}
};

glm::dvec3& BspVertexCache::operator[] (uint32_t iIndex)
{
	return mVertex[iIndex];
};







				/*
				if (s==3 && ll == 13)
				{
					BOOST_LOG_TRIVIAL(info)  << "B  s==3 && ll == 13 \n";
				}
				if (s==3 && ll == 14)
				{
					BOOST_LOG_TRIVIAL(info)  << "B  s==3 && ll == 14 \n";

					BOOST_LOG_TRIVIAL(info)  << "|eA| " << glm::length(eA);
					BOOST_LOG_TRIVIAL(info)  << "|eB| " << glm::length(eB);

					glm::dvec3 a0v = mVertexCache[a0];
					glm::dvec3 a1v = mVertexCache[a1];
					glm::dvec3 a2v = mVertexCache[a2];
					BOOST_LOG_TRIVIAL(info)  << "a0(" <<a0<<")" << a0v.x << " " << a0v.y << " " << a0v.z;
					BOOST_LOG_TRIVIAL(info)  << "a1(" <<a1<<")" << a1v.x << " " << a1v.y << " " << a1v.z;
					BOOST_LOG_TRIVIAL(info)  << "a2(" <<a2<<")" << a2v.x << " " << a2v.y << " " << a2v.z;

					glm::dvec3 b0v = mVertexCache[b0];
					glm::dvec3 b1v = mVertexCache[b1];
					glm::dvec3 b2v = mVertexCache[b2];
					BOOST_LOG_TRIVIAL(info)  << "b0(" <<b0<<")" << b0v.x << " " << b0v.y << " " << b0v.z;
					BOOST_LOG_TRIVIAL(info)  << "b1(" <<b1<<")" << b1v.x << " " << b1v.y << " " << b1v.z;
					BOOST_LOG_TRIVIAL(info)  << "b2(" <<b2<<")" << b2v.x << " " << b2v.y << " " << b2v.z;
				}
				*/


	// PASS 2 --- connect all adacent external edges

	/*


void BspNode::print(std::string iIndent) 
{
	std::cout << iIndent << mN << "    --  " << mW << "\n";
	std::cout << iIndent<< mCoplanar.size() << " c \n";
	for (std::list<Triangle>::iterator lIter = mCoplanar.begin(); lIter != mCoplanar.end(); lIter++)
	{
		//std::cout << iIndent<< mVertex[mCoplanar[i]*3+0] << "\n";
		//std::cout << iIndent<< mVertex[mCoplanar[i]*3+1] << "\n";
		//std::cout << iIndent<< mVertex[mCoplanar[i]*3+2] << "\n";
		//std::cout << iIndent<< "\n";
	}

	iIndent += " ";
	if (mOutside)
	{
		std::cout << iIndent << "FRONT\n";
		mOutside->print(iIndent);
	}
	if (mInside)
	{
		std::cout << iIndent << "BACK\n";
		mInside->print(iIndent);
	}
}


	*/