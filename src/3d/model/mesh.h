#pragma once

#include <glm/glm.hpp>

class Mesh
{
	public:

		Mesh();
		Mesh(Mesh& iMesh, glm::dmat4& iTransform = Mesh::I4);
		Mesh& operator = (const Mesh& iMesh);

		static const uint8_t FRONT = 0x01;
		static const uint8_t BACK = 0x02;
		
		void clear();
		void resize(uint32_t iTriangles); // testing only

		size_t size() { return mVertex.size()/3; }
		glm::dvec3& operator [] (uint32_t i) { return mVertex[i]; }

		// construction
		uint32_t pushVertex(glm::dvec3& iV0, glm::dvec3& iN);
		uint32_t pushTriangle(glm::dvec3& iV0, glm::dvec3& iV1, glm::dvec3& iV2, bool iNormalize);
		uint32_t pushTriangle(glm::dvec3& iV0, glm::dvec3& iV1, glm::dvec3& iV2, glm::dvec3& iN);
		void pushNormals(glm::dvec3& iN0, glm::dvec3& iN1, glm::dvec3& iN2);

		void appendVertex(float* iBuffer, uint32_t iCount);
		void appendNormal(float* iBuffer, uint32_t iCount);
		void appendTangent(float* iBuffer, uint32_t iCount);
		void appendUv(float* iBuffer, uint32_t iCount);
		void append(Mesh& iMesh, glm::dmat4& iMatrix = I4);

		void computeNormals(uint32_t iStart);

		void transform(const glm::dmat4& iMatrix, uint32_t iStart = 0);
		void translate(const glm::dvec3& iVector);
		void scale(glm::dvec3& iScalar);
		void shuffle(std::vector<uint32_t>& iIndex);

		std::vector<glm::dvec3>& vertexBuffer() { return mVertex; };
		std::vector<glm::dvec3>& normalBuffer() { return mNormal; };
		std::vector<glm::dvec4>& tangentBuffer() { return mTangent; };
		std::vector<glm::vec2>& uvBuffer() { return mUV; };

		static inline glm::dvec3 normal(glm::dvec3& iV0, glm::dvec3& iV1, glm::dvec3& iV2)
		{
			glm::dvec3 lE0 = iV1 - iV0;
			glm::dvec3 lE1 = iV2 - iV0;
			return glm::normalize(glm::cross(lE0, lE1));
		}

		void block(double iDx, double iDy, double iDz);
		void sphere(double iRadius);

	protected:

		std::vector<glm::dvec3> mVertex;
		std::vector<glm::dvec3> mNormal;
		std::vector<glm::dvec4> mTangent;
		std::vector<glm::vec2> mUV;
		
		template <class T> void shuffle(std::vector<T>& iDest, std::vector<T>& iSrce, std::vector<uint32_t>& iIndex)
		{
			for (int i=0; i<iIndex.size(); i++)
			{
				iDest[i*3+0] = iSrce[iIndex[i]*3+0];
				iDest[i*3+1] = iSrce[iIndex[i]*3+1];
				iDest[i*3+2] = iSrce[iIndex[i]*3+2];
			}
		};

		static glm::dmat4 I4;
		static glm::dmat3 I3;

	protected:

		class Box
		{
			public:

				static double sVertex[24*3];
				static int sIndex[36];
		};

		class Sphere
		{
			public:

				static void createIcosahedron(int iLevel, int &iVertices, double* iVertex);
				static void createIcosahedron(int iLevel, int &iVertices, double* iVertex, int& iIndices, int* iIndex);

				static const int sTriangleCount = 1280;
				static const int sVertexCount = 642;

				static int sIndex[sTriangleCount*3];
				static double sVertex[sVertexCount*3];

		        static double sIcosahedron[3*3*20]; 

			private:

				static void normalize(double* v);
				static void midpoint(double* a, double* b, double* m);
				static void subdivide(int iLevel, int i0, int i1, int i2, int &iVertices, double* iVertex, int& iIndices, int* iIndex);
				static void merge(double* iVertex, int &iMergedCount, double* iMerged, int& iIndices, int* iIndex);
		};




	public: 

		static void box(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec3 iDim);
		static void sphere(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec3 iDim);
		static void cylinder(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec2 iDim);
		static void cone(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec2 iDim);
		static void pyramid(Mesh& iMesh, glm::dvec3 iCenter, glm::dvec3 iDim);

		static glm::dmat3 XY_PLANE;
		static glm::dmat3 ZX_PLANE;
		static glm::dmat3 YZ_PLANE;
		static void rect(Mesh& iMesh, glm::dmat3 iPlane, glm::dvec3 iCenter, glm::dvec2 iDim);
		static void triangle(Mesh& iMesh, glm::dmat3 iPlane, glm::dvec3 iCenter, glm::dvec2 iDim);
};

