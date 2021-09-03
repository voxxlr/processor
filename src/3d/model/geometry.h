#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>

#include "aabb.h"
#include "mesh.h"
#include "line.h"

class Geometry
{
	public: 

		// used in Javascript
		static const int INTERNAL = 1;
		static const int TRIANGLES = 2;
		static const int LINES = 3;

		Geometry(std::string iName, Mesh* iMesh);
		Geometry(std::string iName, Line* iLine);
		Geometry(Geometry& iGeometry);

		// construction
		void append(Geometry* iGeometry, glm::dmat4& iMatrix = I);

		// geometric transforms
		void translate(glm::dvec3 iVector);
		void scale(glm::dvec3 iVector);
		void transform(glm::dmat4& iMatrix);

		// csg operations
		void accumulate(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix = I);

		void subtract(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix = I);
		void combine(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix = I);
		void intersect(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix = I);

		// exporting
		json_spirit::mObject toJson(int32_t& iBufferOffset, std::string iIndent);
		void computeBVH(std::vector<uint32_t>* iIndex = 0);
		void write(FILE* iFile, int32_t& iBufferOffset);

		void getAABB(glm::dmat4& iTransform, AABB& iAABB);
		void getAABB(AABB& iAABB);

		void pushGeometry(std::vector<Geometry*>& iGeometry);
		int32_t mIndex;

		uint32_t primitiveCount()
		{
			return mMesh->size();
		};
		uint32_t refCount()
		{
			return mRefCount;
		};

		void print()
		{
			std::cout << mMesh->vertexBuffer() << "\n";
		};
		
		void incRef()
		{
			mRefCount++;
		}

	protected: 

		std::vector<glm::vec3>& toVec3(std::vector<glm::dvec3>& iVector);
		std::vector<glm::vec4>& toVec4(std::vector<glm::dvec4>& iVector);
		
		static char FILL[4];
		static glm::dmat4 I;


		bool _Transformed;

		Mesh* mMesh;
		Line* mLine;
		int mType;

		std::string mName;
		std::string mBvh;
		int32_t mRefCount;

		static uint32_t sBufferId;

	//	void clone(std::vector<glm::vec3>& iDest, glm::mat4& iTransform);
};
