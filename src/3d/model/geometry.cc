#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <stdio.h>
#include <string.h>  
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "geometry.h"
#include "bvh.h"
#include "bsp.h"

#define PACKED_NORMAL 1

uint32_t Geometry::sBufferId(1);
char Geometry::FILL[4] = { 0, 0, 0, 0 };
glm::dmat4 Geometry::I(1.0);

Geometry::Geometry(std::string iName, Mesh* iMesh)
: mType(TRIANGLES)
, mName(iName)
, mRefCount(0)
, mIndex(-1)
, mMesh(iMesh)
, mLine(0)
, _Transformed(false)
{
};

Geometry::Geometry(std::string iName, Line* iLine)
: mType(LINES)
, mName(iName)
, mRefCount(0)
, mIndex(-1)
, mMesh(0)
, mLine(iLine)
, _Transformed(false)
{
};


Geometry::Geometry(Geometry& iGeometry)
: mType(iGeometry.mType)
, mName(iGeometry.mName)
, mRefCount(0)
, mIndex(-1)
, mMesh(0)
, mLine(0)
, _Transformed(iGeometry._Transformed)
{
	if (iGeometry.mMesh)
	{
		mMesh = new Mesh(*iGeometry.mMesh);
	}

	if (iGeometry.mLine)
	{
		mLine = new Line(*iGeometry.mLine);
	}
};

void Geometry::getAABB(glm::dmat4& iTransform, AABB& iAABB)
{
	std::vector<glm::dvec3>& lVertex = mMesh->vertexBuffer();
	for (size_t i=0; i<lVertex.size(); i++)
	{
		iAABB.grow(iTransform*glm::dvec4(lVertex[i], 1.0));
	}
} 

void Geometry::getAABB(AABB& iAABB)
{
	std::vector<glm::dvec3>& lVertex = mMesh->vertexBuffer();
	for (size_t i=0; i<lVertex.size(); i++)
	{
		iAABB.grow(lVertex[i]);
	}
}

void Geometry::append(Geometry* iGeometry, glm::dmat4& iMatrix)
{
	mMesh->append(*iGeometry->mMesh, iMatrix);
}

void Geometry::translate(glm::dvec3 iVector)
{
	mMesh->translate(iVector);
}

void Geometry::scale(glm::dvec3 iVector)
{
	mMesh->scale(iVector);
}

void Geometry::transform(glm::dmat4& iMatrix)
{
	if (!_Transformed)
	{
		_Transformed = true;
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << " TRANSFROMED   ________________________________________________";
	}
	mMesh->transform(iMatrix);
}

//
// REMVOE START -----------------
//


void Geometry::accumulate(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix)
{
	Mesh lMesh(*mMesh, iMatrix);

	BspTree::combine(iMesh, lMesh, iPrecision);
}

void Geometry::subtract(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix)
{
	glm::dmat4 lMatrix(glm::inverse(iMatrix));
	Mesh lMesh(iMesh, lMatrix);

	BspTree::subtract(*mMesh, lMesh, iPrecision);
}

void Geometry::combine(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix)
{
	glm::dmat4 lMatrix(glm::inverse(iMatrix));
	Mesh lMesh(iMesh, lMatrix);

	BspTree::combine(*mMesh, lMesh, iPrecision);
}

void Geometry::intersect(Mesh& iMesh, double iPrecision, glm::dmat4& iMatrix)
{
	glm::dmat4 lMatrix(glm::inverse(iMatrix));
	Mesh lMesh(iMesh, lMatrix);

	BspTree::intersect(*mMesh, lMesh, iPrecision);
};


//
// REMVOE END -----------------
//

void Geometry::pushGeometry(std::vector<Geometry*>& iGeometry)
{
	if (mRefCount)
	{
		mIndex = iGeometry.size();
		iGeometry.push_back(this);
		mRefCount = 0;
	}
};

void Geometry::computeBVH(std::vector<uint32_t>* iIndex)
{
	Bvh* lBvh = Bvh::construct(*this->mMesh, iIndex);
	std::stringstream lString;
	json_spirit::write_stream(json_spirit::mValue(lBvh->toJson()), lString);
	mBvh = lString.str();
}


json_spirit::mObject Geometry::toJson(int32_t& iBufferOffset, std::string iIndent)
{
	int lElementCount = mMesh->size()*3;

	json_spirit::mObject lObject;

	json_spirit::mObject lBuffer;
	lBuffer["id"] = std::to_string(sBufferId++);
	lBuffer["type"] = "float32";
	lBuffer["size"] = lElementCount*3;
	lBuffer["offset"] = iBufferOffset;
	lObject["vertex"] = lBuffer;
	iBufferOffset += lElementCount*3*sizeof(float);

	iBufferOffset += mBvh.length();
 	lObject["bvh"] = (int)mBvh.length();

	// align to 4 byte
	int lLength = iBufferOffset%4;
	if (lLength != 0)
	{
		iBufferOffset += 4-lLength;
	}
		
	if (mMesh->normalBuffer().size())
	{
		json_spirit::mObject lBuffer;
		lBuffer["id"] = std::to_string(sBufferId++);
		lBuffer["offset"] = iBufferOffset;
	
#if defined PACKED_NORMAL
		lBuffer["type"] = "INT_2_10_10_10_REV";
		lBuffer["size"] = lElementCount;
		iBufferOffset += lElementCount*sizeof(int32_t);
#else
		lBuffer["type"] = "float32";
		lBuffer["size"] = lElementCount*3;
		iBufferOffset += lElementCount*3*sizeof(float);
#endif
		lObject["normal"] = lBuffer;

	}
	if (mMesh->tangentBuffer().size())
	{
		json_spirit::mObject lBuffer;
		lBuffer["id"] = std::to_string(sBufferId++);
		lBuffer["type"] = "float32";
		lBuffer["size"] =  lElementCount*4;
		lBuffer["offset"] = iBufferOffset;

		iBufferOffset += lElementCount*4*sizeof(float);
		lObject["tangent"] = lBuffer;
	}
	if (mMesh->uvBuffer().size())
	{
		json_spirit::mObject lBuffer;
		lBuffer["id"] = std::to_string(sBufferId++);
		lBuffer["type"] = "float32";
		lBuffer["size"] = lElementCount*2;
		lBuffer["offset"] = iBufferOffset;

		iBufferOffset += lElementCount*2*sizeof(float);
		lObject["uv"] = lBuffer;
	}

	lObject["name"] = mName;
	return lObject;
}


std::vector<glm::vec3>& Geometry::toVec3(std::vector<glm::dvec3>& iVector)
{
	static std::vector<glm::vec3> sVector;
	sVector.clear();
	sVector.reserve(iVector.size());
	for (size_t i=0; i<iVector.size(); i++)
	{
		sVector.push_back(glm::vec3(iVector[i]));
	}
	return sVector;
}


std::vector<glm::vec4>& Geometry::toVec4(std::vector<glm::dvec4>& iVector)
{
	static std::vector<glm::vec4> sVector;
	sVector.clear();
	sVector.reserve(iVector.size());
	for (size_t i=0; i<iVector.size(); i++)
	{
		sVector.push_back(glm::vec4(iVector[i]));
	}
	return sVector;
}

void Geometry::write(FILE* iFile, int32_t& iBufferOffset)
{
	uint32_t lElementCount = mMesh->size()*3;

	std::vector<glm::vec3>& lVertex = toVec3(mMesh->vertexBuffer());
	fwrite(&lVertex[0], 1, sizeof(float)*3*lElementCount, iFile);
	iBufferOffset += lElementCount*3*sizeof(float);

	fwrite(mBvh.c_str(), 1, mBvh.length(), iFile);
	iBufferOffset += mBvh.length();

	// align to 4 byte
	int lLength = iBufferOffset%4;
	if (lLength != 0)
	{
		fwrite(FILL, 1, 4-lLength, iFile);
		iBufferOffset += 4-lLength;
	}

	std::vector<glm::vec3>& lNormal = toVec3(mMesh->normalBuffer());
	if (lNormal.size())
	{

#if defined PACKED_NORMAL

		union Vec3IntPacked {
			int i32;
			struct {
				int x:10;
				int y:10;
				int z:10;
				int a:2;
			} i32f3;
		};

		static std::vector<Vec3IntPacked> sBuffer;
		sBuffer.clear();
		sBuffer.resize(lElementCount);
		for (size_t i=0; i<lNormal.size(); i++)
		{
			Vec3IntPacked& lPacked = sBuffer[i];
			lPacked.i32f3.x = floor(lNormal[i].x*511);
			lPacked.i32f3.y = floor(lNormal[i].y*511);
			lPacked.i32f3.z = floor(lNormal[i].z*511);
			lPacked.i32f3.a = 0;
		}

		fwrite(&sBuffer[0], 1, sizeof(Vec3IntPacked)*lElementCount, iFile);
		iBufferOffset += lElementCount*sizeof(Vec3IntPacked);

#else
		fwrite(&lNormal[0], 1, sizeof(float)*3*lElementCount, iFile);
		iBufferOffset += lElementCount*3*sizeof(float);
#endif

	}

	std::vector<glm::vec4>& lTangent = toVec4(mMesh->tangentBuffer());
	if (lTangent.size())
	{
		fwrite(&lTangent[0], 1, sizeof(float)*4*lElementCount, iFile);
		iBufferOffset += lElementCount*4*sizeof(float);
	}

	std::vector<glm::vec2>& lUV = mMesh->uvBuffer();
	if (lUV.size())
	{
		fwrite(&lUV[0], 1, sizeof(float)*2*lElementCount, iFile);
		iBufferOffset += lElementCount*2*sizeof(float);
	}
};


