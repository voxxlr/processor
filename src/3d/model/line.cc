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

#include "line.h"
#include "bsp.h"
#include "aabb.h"

glm::dmat4 Line::I4(1.0);
glm::dmat3 Line::I3(1.0);

Line::Line() 
{
}

Line::Line(Line& iLine, glm::dmat4& iMatrix)
: mVertex(iLine.mVertex)
{
	if (iMatrix != I4)
	{
		transform(iMatrix);
	}
}

Line& Line::operator = (const Line& iLine)
{
	mVertex.clear();
	mVertex.reserve(iLine.mVertex.size());
	mVertex.insert(mVertex.begin(), iLine.mVertex.begin(), iLine.mVertex.end());

	return *this;
};

void Line::clear()
{
	mVertex.clear();
}

uint32_t Line::pushVertex(glm::dvec3& iV, glm::dvec3& iN)
{
	mVertex.push_back(iV);
	return mVertex.size() - 1;
};


void Line::transform(glm::dmat4& iMatrix, uint32_t iStart)
{
	for (int i=iStart; i<mVertex.size(); i++)
	{
		mVertex[i] = glm::dvec3(iMatrix*glm::dvec4(mVertex[i], 1.0));
	}
}

void Line::translate(glm::dvec3& iVector)
{
	for (int i=0; i<mVertex.size(); i++)
	{
		mVertex[i] += iVector;
	}
};

void Line::scale(glm::dvec3& iScalar)
{
	for (int i=0; i<mVertex.size(); i++)
	{
		mVertex[i] *= iScalar;
	}
}
