#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>

#define _USE_MATH_DEFINES
#include <math.h>

#include "mesh.h"
#include "bsp.h"
#include "aabb.h"

glm::dmat4 Mesh::I4(1.0);
glm::dmat3 Mesh::I3(1.0);

Mesh::Mesh() 
{
}

Mesh::Mesh(Mesh& iMesh, glm::dmat4& iMatrix)
: mVertex(iMesh.mVertex)
, mNormal(iMesh.mNormal)
{
	if (iMatrix != I4)
	{
		transform(iMatrix);
	}
}

Mesh& Mesh::operator = (const Mesh& iMesh)
{
	mVertex.clear();
	mVertex.reserve(iMesh.mVertex.size());
	mVertex.insert(mVertex.begin(), iMesh.mVertex.begin(), iMesh.mVertex.end());

	mNormal.clear();
	mNormal.reserve(iMesh.mNormal.size());
	mNormal.insert(mNormal.begin(), iMesh.mNormal.begin(), iMesh.mNormal.end());

	return *this;
};

void Mesh::clear()
{
	mVertex.clear();
	mNormal.clear();
	mTangent.clear();
	mUV.clear();
}

void Mesh::resize(uint32_t iTriangles)
{
	mNormal.resize(iTriangles*3);
	mVertex.resize(iTriangles*3);
}; // testing only


uint32_t Mesh::pushVertex(glm::dvec3& iV, glm::dvec3& iN)
{
	mVertex.push_back(iV);
	mNormal.push_back(iN);
	return mVertex.size() - 1;
};

uint32_t Mesh::pushTriangle(glm::dvec3& iV0, glm::dvec3& iV1, glm::dvec3& iV2, glm::dvec3& iN)
{
	mVertex.push_back(iV0);
	mVertex.push_back(iV1);
	mVertex.push_back(iV2);
	mNormal.push_back(iN);
	mNormal.push_back(iN);
	mNormal.push_back(iN);
	return mVertex.size()/3 - 1;
}

void Mesh::pushNormals(glm::dvec3& iN0, glm::dvec3& iN1, glm::dvec3& iN2)
{
	mNormal.push_back(iN0);
	mNormal.push_back(iN1);
	mNormal.push_back(iN2);
}

uint32_t Mesh::pushTriangle(glm::dvec3& iV0, glm::dvec3& iV1, glm::dvec3& iV2, bool iNormal)
{
	mVertex.push_back(iV0);
	mVertex.push_back(iV1);
	mVertex.push_back(iV2);
	if (iNormal)
	{
		glm::dvec3 lN = normal(iV0, iV1, iV2);
		mNormal.push_back(lN);
		mNormal.push_back(lN);
		mNormal.push_back(lN);
	}
	return mVertex.size()/3 - 1;
}


void Mesh::appendVertex(float* iBuffer, uint32_t iCount)
{
	mVertex.reserve(mVertex.size() + iCount);
	for (int i=0; i<iCount; i++)
	{
		mVertex.push_back(glm::dvec3(iBuffer[i*3+0], iBuffer[i*3+1], iBuffer[i*3+2]));
	}
}

void Mesh::appendNormal(float* iBuffer, uint32_t iCount)
{
	mNormal.reserve(mVertex.size());
	for (int i=0; i<iCount; i++)
	{
		mNormal.push_back(glm::dvec3(iBuffer[i*3+0], iBuffer[i*3+1], iBuffer[i*3+2]));
	}
}

void Mesh::appendTangent(float* iBuffer, uint32_t iCount)
{
	mTangent.reserve(mVertex.size());
	for (int i=0; i<iCount; i++)
	{
		mTangent.push_back(glm::dvec4(iBuffer[i*4+0], iBuffer[i*4+1], iBuffer[i*4+2], iBuffer[i*4+3]));
	}
}

void Mesh::appendUv(float* iBuffer, uint32_t iCount)
{
	mUV.reserve(mVertex.size());
	for (int i=0; i<iCount; i++)
	{
		mUV.push_back(glm::make_vec2(&iBuffer[i*2]));
	}
}








void Mesh::append(Mesh& iMesh, glm::dmat4& iMatrix)
{
	uint32_t lStart = mVertex.size();
	mVertex.insert(mVertex.end(), iMesh.mVertex.begin(),  iMesh.mVertex.end());
	mNormal.insert(mNormal.end(), iMesh.mNormal.begin(),  iMesh.mNormal.end());
	mUV.insert(mUV.end(), iMesh.mUV.begin(),  iMesh.mUV.end());
	mTangent.insert(mTangent.end(), iMesh.mTangent.begin(),  iMesh.mTangent.end());

	if (iMatrix != I4)
	{
		transform(iMatrix, lStart);
	}
};





void Mesh::computeNormals(uint32_t iStart)
{
	// compute Normals
	for (int i=iStart/3; i<mVertex.size()/3; i++)
	{
		int lV0 = i*3+0;
		int lV1 = i*3+1;
		int lV2 = i*3+2;
		glm::dvec3 lV10(mVertex[lV1].x - mVertex[lV0].x, mVertex[lV1].y - mVertex[lV0].y, mVertex[lV1].z - mVertex[lV0].z);
		glm::dvec3 lV20(mVertex[lV2].x - mVertex[lV0].x, mVertex[lV2].y - mVertex[lV0].y, mVertex[lV2].z - mVertex[lV0].z);
		glm::dvec3 lN = glm::normalize(glm::cross(lV10, lV20));
		mNormal.push_back(lN);
		mNormal.push_back(lN);
		mNormal.push_back(lN);
	}
}


void Mesh::transform(const glm::dmat4& iMatrix, uint32_t iStart)
{
	for (int i=iStart; i<mVertex.size(); i++)
	{
		mVertex[i] = glm::dvec3(iMatrix*glm::dvec4(mVertex[i], 1.0));
	}

	glm::dmat3 lR(iMatrix);
	if (lR != I3)
	{
		lR[0] = glm::normalize(lR[0]);
		lR[1] = glm::normalize(lR[1]);
		lR[2] = glm::normalize(lR[2]);
		for (int i=iStart; i<mNormal.size(); i++)
		{
			mNormal[i] = lR*mNormal[i];
		}
		for (int i=iStart; i<mTangent.size(); i++)
		{
			glm::dvec3 lTangent = lR*glm::dvec3(mTangent[i]);
			mTangent[i].x = lTangent[0];
			mTangent[i].y = lTangent[1];
			mTangent[i].z = lTangent[2];
		}
	}
}

void Mesh::translate(const glm::dvec3& iVector)
{
	for (int i=0; i<mVertex.size(); i++)
	{
		mVertex[i] += iVector;
	}
};

void Mesh::scale(glm::dvec3& iScalar)
{
	for (int i=0; i<mVertex.size(); i++)
	{
		mVertex[i] *= iScalar;
	}
}

void Mesh::shuffle(std::vector<uint32_t>& iIndex)
{
	std::vector<glm::dvec3> lVertex;
	lVertex.insert(lVertex.begin(), mVertex.begin(), mVertex.end());
	shuffle(mVertex, lVertex, iIndex);

	if (mNormal.size())
	{
		std::vector<glm::dvec3> lNormal;
		lNormal.insert(lNormal.begin(), mNormal.begin(), mNormal.end());
		shuffle(mNormal, lNormal, iIndex);
	}

	if (mTangent.size())
	{
		std::vector<glm::dvec4> lTangent;
		lTangent.insert(lTangent.begin(), mTangent.begin(), mTangent.end());
		shuffle(mTangent, lTangent, iIndex);
	}

	if (mUV.size())
	{
		std::vector<glm::vec2> lUV;
		lUV.insert(lUV.begin(), mUV.begin(), mUV.end());
		shuffle(mUV, lUV, iIndex);
	}
}



void Mesh::sphere(double iRadius)
{
	Mesh::sphere(*this, glm::dvec3(0, 0, 0), glm::dvec3(iRadius, iRadius, iRadius));
}

void Mesh::block(double iDx, double iDy, double iDz)
{
	Mesh::box(*this, glm::dvec3(iDx/2, iDy/2, iDz/2), glm::dvec3(iDx, iDy, iDz));
}








void Mesh::sphere(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec3 iDim)
{
    static int sIndecies = 0;
	if (!sIndecies)
	{
		int lVertices = 0;
		Sphere::createIcosahedron(1, lVertices, Sphere::sVertex, sIndecies, Sphere::sIndex);
	}

	for (int i=0; i<sIndecies/3; i++)
	{
		glm::dvec3 lN0 = glm::make_vec3(&Sphere::sVertex[Sphere::sIndex[i*3+0]*3]);
		glm::dvec3 lN1 = glm::make_vec3(&Sphere::sVertex[Sphere::sIndex[i*3+1]*3]);
		glm::dvec3 lN2 = glm::make_vec3(&Sphere::sVertex[Sphere::sIndex[i*3+2]*3]);

		glm::dvec3 lV0 = iCenter + glm::make_vec3(&Sphere::sVertex[Sphere::sIndex[i*3+0]*3])*iDim;
		glm::dvec3 lV1 = iCenter + glm::make_vec3(&Sphere::sVertex[Sphere::sIndex[i*3+1]*3])*iDim;
		glm::dvec3 lV2 = iCenter + glm::make_vec3(&Sphere::sVertex[Sphere::sIndex[i*3+2]*3])*iDim;

		iMesh.pushTriangle(lV0, lV1, lV2, false);
		iMesh.pushNormals(lN0, lN1, lN2);
	}
   // GeoGlobal::computeTriTangents(sIndecies/3,sIndex,sVertexCount,sVertex,sNormal,sTexture,sTangent);
};



void Mesh::box(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec3 iDim)
{
	for (int i=0; i<12; i++)
	{
		glm::dvec3 lV0 = iCenter + glm::make_vec3(&Box::sVertex[Box::sIndex[i*3+0]*3])*iDim;
		glm::dvec3 lV1 = iCenter + glm::make_vec3(&Box::sVertex[Box::sIndex[i*3+1]*3])*iDim;
		glm::dvec3 lV2 = iCenter + glm::make_vec3(&Box::sVertex[Box::sIndex[i*3+2]*3])*iDim;

		iMesh.pushTriangle(lV0, lV1, lV2, true);
	}
};



glm::dmat3 Mesh::XY_PLANE(1, 0, 0,
						  0, 1, 0,
						  0, 0, 1);

glm::dmat3 Mesh::ZX_PLANE(1, 0, 0,
						  0, 0,-1,
						  0, 1, 0);

glm::dmat3 Mesh::YZ_PLANE(0, 0,-1,
						  0, 1, 0,
						  1, 0, 0);

/*
glm::dmat3 Mesh::ZX_PLANE(1, 0, 0,
						  0, 1, 0,
						  0, 0, 1);

glm::dmat3 Mesh::XY_PLANE(1, 0, 0,
						  0, 0, 1,
						  0,-1, 0);

glm::dmat3 Mesh::YZ_PLANE(0, 0,-1,
						  0, 1, 0,
						  1, 0, 0);
						*/

void Mesh::rect(Mesh& iMesh, glm::dmat3 iPlane, glm::dvec3 iCenter, glm::dvec2 iDim)
{
	glm::dvec3 lNormal = glm::dvec3(iPlane[2]);

	glm::dvec3 lV0 = iCenter + glm::dvec3(iPlane*glm::dvec3(-iDim.x/2, -iDim.y/2, 0.0));
	glm::dvec3 lV1 = iCenter + glm::dvec3(iPlane*glm::dvec3( iDim.x/2, -iDim.y/2, 0.0));
	glm::dvec3 lV2 = iCenter + glm::dvec3(iPlane*glm::dvec3( iDim.x/2,  iDim.y/2, 0.0));
	glm::dvec3 lV3 = iCenter + glm::dvec3(iPlane*glm::dvec3(-iDim.x/2,  iDim.y/2, 0.0));
	
	iMesh.pushTriangle(lV0, lV1, lV2, true);
	iMesh.pushTriangle(lV2, lV3, lV0, true);
}

void Mesh::triangle(Mesh& iMesh, glm::dmat3 iPlane, glm::dvec3 iCenter, glm::dvec2 iDim)
{
	glm::dvec3 lNormal = glm::dvec3(iPlane[2]);

	glm::dvec3 lV0 = iCenter + glm::dvec3(iPlane*glm::dvec3(-iDim.x/2, -iDim.y/2, 0.0));
	glm::dvec3 lV1 = iCenter + glm::dvec3(iPlane*glm::dvec3( iDim.x/2, -iDim.y/2, 0.0));
	glm::dvec3 lV2 = iCenter + glm::dvec3(iPlane*glm::dvec3( 0,  iDim.y/2, 0.0));
	
	iMesh.pushTriangle(lV0, lV1, lV2, true);
}

void Mesh::cylinder(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec2 iDim)
{
	static const int kN = 12;
	// body
	double lDr = M_PI*2.0/kN;
	glm::dvec3 lTop(iCenter.x, iCenter.y, iCenter.z+iDim[1]);
    for (int i=0; i<kN; i++) 
    {
        double lL0 = lDr*i;
        double lL1 = lDr*(i+1);
		
		glm::dvec3 lV0 = iCenter + glm::dvec3(sin(lL0)*iDim[0], cos(lL0)*iDim[0], 0.0);
		glm::dvec3 lV1 = iCenter + glm::dvec3(sin(lL1)*iDim[0], cos(lL1)*iDim[0], 0.0);
		glm::dvec3 lV2 = iCenter + glm::dvec3(sin(lL1)*iDim[0], cos(lL1)*iDim[0], iDim[1]);
		glm::dvec3 lV3 = iCenter + glm::dvec3(sin(lL0)*iDim[0], cos(lL0)*iDim[0], iDim[1]);

		iMesh.pushTriangle(lV2, lV1, lV0, true);
		iMesh.pushTriangle(lV0, lV3, lV2, true);

		// top
		iMesh.pushTriangle(lTop, lV2, lV3, true);
		// bottom
		iMesh.pushTriangle(lV0, lV1, iCenter, true);

    }
};

void Mesh::pyramid(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec3 iDim)
{
	double dx = iDim[0];
	double dy = iDim[1];
	double dh = iDim[2];

	glm::dvec3 v0 = glm::dvec3(0,0,0);
	glm::dvec3 v1 = glm::dvec3(dx,0,0);
	glm::dvec3 v2 = glm::dvec3(dx,dy,0);
	glm::dvec3 v3 = glm::dvec3(0,dy,0);
	glm::dvec3 vt = glm::dvec3(dx/2,dy/2,dy);

	iMesh.pushTriangle(v0,v1,vt,true);
	iMesh.pushTriangle(v1,v2,vt,true);
	iMesh.pushTriangle(v2,v3,vt,true);
	iMesh.pushTriangle(v3,v0,vt,true);

	iMesh.pushTriangle(v2,v1,v0,true);
	iMesh.pushTriangle(v0,v3,v2,true);
}

void Mesh::cone(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec2 iDim)
{
	static double kN = 12;
	// body
	double lDr = M_PI*2.0/kN;
	glm::dvec3 lTop(iCenter.x, iCenter.y+iDim[1], iCenter.z);
    for (int i=0; i<kN; i++) 
    {
        double lL0 = lDr*i;
        double lL1 = lDr*(i+1);
		
		glm::dvec3 lV0 = iCenter + glm::dvec3(sin(lL0)*iDim[0], cos(lL0)*iDim[0], 0.0);
		glm::dvec3 lV1 = iCenter + glm::dvec3(sin(lL1)*iDim[0], cos(lL1)*iDim[0], 0.0);

		iMesh.pushTriangle(lTop, lV1, lV0, true);
		iMesh.pushTriangle(lV0, lV1, iCenter, true);
    }
}

//
//
//
//
//
//





	
double Mesh::Box::sVertex[24*3] =  {
                              0.5,-0.5,-0.5,
                              0.5, 0.5,-0.5, 
                              0.5, 0.5, 0.5, 
                              0.5,-0.5, 0.5, 

                             -0.5, 0.5,-0.5, 
                             -0.5,-0.5,-0.5,
                             -0.5,-0.5, 0.5,
                             -0.5, 0.5, 0.5,

                              0.5, 0.5,-0.5, 
                             -0.5, 0.5,-0.5, 
                             -0.5, 0.5, 0.5,
                              0.5, 0.5, 0.5, 

                             -0.5,-0.5,-0.5,
                              0.5,-0.5,-0.5,
                              0.5,-0.5, 0.5, 
                             -0.5,-0.5, 0.5,

                             -0.5,-0.5, 0.5,
                              0.5,-0.5, 0.5, 
                              0.5, 0.5, 0.5, 
                             -0.5, 0.5, 0.5,

                              0.5,-0.5,-0.5,
                             -0.5,-0.5,-0.5,
                             -0.5, 0.5,-0.5, 
                              0.5, 0.5,-0.5 
                            };

int Mesh::Box::sIndex[36]  = {0,1,2, 
                              0,2,3, 
                              4,5,6, 
                              4,6,7, 
                              8,9,10, 
                              8,10,11, 
                              12,13,14, 
                              12,14,15, 
                              16,17,18, 
                              16,18,19, 
                              20,21,22,
                              20,22,23
                              };




//
//
// 
//  Icosahedron
//
//

#define R  2*sqrt(5.0)/5.0
#define H  sqrt(5.0)/5.0

#define TOP   0,0,1  	//  + 
#define PT1   cos(72*0.0*M_PI/180)*R,sin(72*0.0*M_PI/180)*R,H 	// 1  
#define PT2   cos(72*1.0*M_PI/180)*R,sin(72*1.0*M_PI/180)*R,H 	// 2  
#define PT3   cos(72*2.0*M_PI/180)*R,sin(72*2.0*M_PI/180)*R,H 	// 3  
#define PT4   cos(72*3.0*M_PI/180)*R,sin(72*3.0*M_PI/180)*R,H 	// 5  
#define PT5   cos(72*4.0*M_PI/180)*R,sin(72*4.0*M_PI/180)*R,H 	// 5  

#define BOT   0,0,-1  	//  - 
#define PT7   cos(72*0.5*M_PI/180)*R,sin(72*0.5*M_PI/180)*R,-H 	// 7  
#define PT8   cos(72*1.5*M_PI/180)*R,sin(72*1.5*M_PI/180)*R,-H 	// 8  
#define PT9   cos(72*2.5*M_PI/180)*R,sin(72*2.5*M_PI/180)*R,-H 	// 9  
#define PT10  cos(72*3.5*M_PI/180)*R,sin(72*3.5*M_PI/180)*R,-H 	// 10
#define PT11  cos(72*4.5*M_PI/180)*R,sin(72*4.5*M_PI/180)*R,-H 	// 11 

double Mesh::Sphere::sIcosahedron[3*3*20] = 
{
    PT2, PT7, PT1, 
    PT3, PT8, PT2, 
    PT4, PT9, PT3, 
    PT5, PT10,PT4,  
    PT1, PT11, PT5,  

    PT7, PT2,PT8,   
    PT8, PT3,PT9,   
    PT9, PT4,PT10,  
    PT10,PT5,PT11,  
    PT11,PT1,PT7,  

    TOP, PT2,PT1,  
    TOP, PT3,PT2,  
    TOP, PT4,PT3,  
    TOP, PT5,PT4,  
    TOP, PT1,PT5,  

    BOT, PT7, PT8,  
    BOT, PT8, PT9,  
    BOT, PT9, PT10, 
    BOT, PT10,PT11,  
    BOT, PT11,PT7,   
};

double Mesh::Sphere::sVertex[Mesh::Sphere::sVertexCount*3];
int Mesh::Sphere::sIndex[Mesh::Sphere::sTriangleCount*3];

void Mesh::Sphere::normalize(double* v)
{
    double mag;
    mag = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (mag != 0.0) 
    {
	    mag = 1.0 / sqrt(mag);
	    v[0] *= mag;
	    v[1] *= mag;
	    v[2] *= mag;
    }
}

void Mesh::Sphere::midpoint(double* a, double* b, double* m)
{
    m[0] = (a[0] + b[0]) * 0.5;
    m[1] = (a[1] + b[1]) * 0.5;
    m[2] = (a[2] + b[2]) * 0.5;
}

void Mesh::Sphere::subdivide(int iLevel, int i0, int i1, int i2, int &iVertices, double* iVertex, int& iIndices, int* iIndex)
{
    int a = iVertices++;
    int b = iVertices++;
    int c = iVertices++;

    midpoint(&iVertex[i0*3], &iVertex[i2*3], &iVertex[a*3]);
	normalize(&iVertex[a*3]);

    midpoint(&iVertex[i0*3], &iVertex[i1*3], &iVertex[b*3]);
	normalize(&iVertex[b*3]);

    midpoint(&iVertex[i1*3], &iVertex[i2*3], &iVertex[c*3]);
	normalize(&iVertex[c*3]);

    if (iLevel == 0)
    {
        iIndex[iIndices++] = a;
        iIndex[iIndices++] = b;
        iIndex[iIndices++] = i0;

        iIndex[iIndices++] = i2;
        iIndex[iIndices++] = c;
        iIndex[iIndices++] = a;

        iIndex[iIndices++] = c;
        iIndex[iIndices++] = i1;
        iIndex[iIndices++] = b;

        iIndex[iIndices++] = c;
        iIndex[iIndices++] = b;
        iIndex[iIndices++] = a;
    }
    else
    {
        subdivide(iLevel-1,i0,b,a, iVertices, iVertex, iIndices, iIndex);
        subdivide(iLevel-1,a,c,i2, iVertices, iVertex, iIndices, iIndex);
        subdivide(iLevel-1,b,i1,c, iVertices, iVertex, iIndices, iIndex);
        subdivide(iLevel-1,a,b,c, iVertices, iVertex, iIndices, iIndex);
    }
}

void Mesh::Sphere::merge(double* iVertex, int &iMergedCount, double* iMerged, int& iIndices, int* iIndex)
{
    iMergedCount = 0;
    for (int i=0; i<iIndices; i++)
    {
		int j;

        for (j=0; j<iMergedCount; j++)
        {
            if (fabs(iMerged[j*3+0]-iVertex[iIndex[i]*3+0]) < 0.00001 && fabs(iMerged[j*3+1]-iVertex[iIndex[i]*3+1]) < 0.00001  && fabs(iMerged[j*3+2]-iVertex[iIndex[i]*3+2]) < 0.00001)
            {
                break;
            }
        }

        if (j==iMergedCount)
        {
            iMerged[iMergedCount*3+0] = iVertex[iIndex[i]*3+0];
            iMerged[iMergedCount*3+1] = iVertex[iIndex[i]*3+1];
            iMerged[iMergedCount*3+2] = iVertex[iIndex[i]*3+2];
            iMergedCount++;
        }

        iIndex[i] = j;
    }
}


void Mesh::Sphere::createIcosahedron(int iLevel, int &iVertices, double* iVertex, int& iIndices, int* iIndex)
{
    static double lVertex[900000];
    int lVertices = sizeof(sIcosahedron)/(sizeof(double)*3);

    memcpy(lVertex, sIcosahedron, sizeof(sIcosahedron));
    for (int i=0; i<20; i++)
    {
        subdivide(iLevel, i*3+0, i*3+1, i*3+2, lVertices, lVertex, iIndices, iIndex);
    }
    merge(lVertex, iVertices, iVertex, iIndices, iIndex);
}

void Mesh::Sphere::createIcosahedron(int iLevel, int &iVertices, double* iVertex)
{
    int lIndex[900000];
    int lIndices = 0;
    iVertices = 0;
    createIcosahedron(iLevel, iVertices, iVertex, lIndices, lIndex);
}


