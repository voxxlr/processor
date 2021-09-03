#pragma once

#define GLM_FORCE_CTOR_INIT

#include <glm/glm.hpp>


class Line
{
	public:

		Line();
		Line(Line& iLine, glm::dmat4& iTransform = I4);
		Line& operator = (const Line& iLine);

		
		void clear();

		size_t size() { return mVertex.size()/3; }
		glm::dvec3& operator [] (uint32_t i) { return mVertex[i]; }

		// construction
		uint32_t pushVertex(glm::dvec3& iV0, glm::dvec3& iN);

		void transform(glm::dmat4& iMatrix, uint32_t iStart = 0);
		void translate(glm::dvec3& iVector);
		void scale(glm::dvec3& iScalar);
		void shuffle(std::vector<uint32_t>& iIndex);

		std::vector<glm::dvec3>& vertexBuffer() { return mVertex; };


	protected:

		std::vector<glm::dvec3> mVertex;

		static glm::dmat4 I4;
		static glm::dmat3 I3;

	protected:

};

