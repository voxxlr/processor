#pragma once

#define GLM_FORCE_CTOR_INIT
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>


#include <set>
#include <map>
#include <tuple>
#include <list>
#include <unordered_map>

#include "mesh.h"
#include "aabb.h"

class BspTree;

class BspVertexCache
{
	public:

		static const uint32_t SIZE = 999983;

		BspVertexCache(double iPrecision);

		uint32_t insert(glm::dvec3& iVertex);
		glm::dvec3& operator[] (uint32_t iIndex);
		double mResolution;
			
	private:

		typedef std::tuple<int64_t, int64_t, int64_t, bool, uint32_t> GridPoint;

		std::vector<GridPoint> mIndex;
		std::vector<glm::dvec3> mVertex;
};


class BspMesh
{
	public:

		typedef std::tuple<uint32_t, uint32_t, uint32_t, bool> Triangle;

		BspMesh(glm::dvec3 iN, double iW, BspVertexCache& iVertexCache);

		std::vector<Triangle> mList;

		void write(Mesh& iMesh);

	protected:

		glm::dvec3 mN;
		double mW;

		BspVertexCache& mVertexCache;
		typedef std::tuple<uint32_t, uint32_t> Edge; 

};

class BspNode
{
	friend class BspTree;

	public:

		static double EPSILON;

		typedef std::tuple<uint32_t, uint32_t, uint32_t, bool> Triangle;

		BspNode(std::list<Triangle>& iIndex, BspVertexCache& iVertexCache, std::string iTree = "");
		~BspNode();

		static const uint8_t INSIDE = 0x0;
		static const uint8_t OUTSIDE = 0x1;

		void clipBy(uint8_t iSide, BspNode& iTree);

		void invert();
		
		void collapse(std::vector<BspNode*>& iList);

	protected:
		
		std::string mTree;

		// tree
		glm::dvec3 mN;
		double mW;
		BspNode* mOutside;
		BspNode* mInside;
		std::list<Triangle> mCoplanar;

		BspVertexCache& mVertexCache;
		
		// split triangles set
		std::list<Triangle> clip(Triangle& iIndex, glm::dvec3& iNormal, uint8_t iSide);

		static const uint8_t COPLANAR = 0x0;
		static const uint8_t FRONT = 0x1;
		static const uint8_t BACK = 0x2;
		static const uint8_t SPANNING = 0x3;
		void split(Triangle iTriangle, glm::uvec3 iSide, std::list<Triangle>& iFront, std::list<Triangle>& iBack);

		glm::dvec3& intersect(uint32_t ia, uint32_t ib);
		uint32_t side(uint32_t i0);
		glm::uvec3& span(Triangle iTriangle);
};


class BspTree
{
	public:

		static void subtract(Mesh& iA, Mesh& iB, double iPrecision, int iDebug = -1);
		static void combine(Mesh& iA, Mesh& iB, double iPrecision);
		static void intersect(Mesh& iA, Mesh& iB, double iPrecision);
			
	private:

		static void write(std::vector<BspNode*>& iList0, BspVertexCache& iVertexCache, Mesh& iMesh);

		static void initIndex(Mesh& iMesh, std::list<BspNode::Triangle>& iIndex, BspVertexCache& iVertex);
};
