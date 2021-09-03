#include <iostream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include "normalProcessor.h"

#define PACKED_NORMAL 1


NormalProcessor::NormalProcessor()
: InorderOperation("Normal")
{
};

void NormalProcessor::initTraveral(PointCloudAttributes& iAttributes)
{
#if defined PACKED_NORMAL
	PackedNormalType lAttribute;
#else
	NormalType lAttribute;
#endif	

	iAttributes.createAttribute(Attribute::NORMAL, lAttribute);
}

void NormalProcessor::processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	computeNormals(iCloud, iCloud.getAttributeIndex(Attribute::NORMAL));

	iCloud.writeFile(iNode.mPath);
};

void NormalProcessor::processInternal(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	computeNormals(iCloud, iCloud.getAttributeIndex(Attribute::NORMAL));

	iCloud.writeFile(iNode.mPath);
};


void NormalProcessor::computeNormals(PointCloud& iPoints, uint32_t iNormalIndex)
{
	KdTree<KdSpatialDomain> lTree(iPoints, 100);
	lTree.construct();

	//
	// compute normals
	//
	std::vector<std::pair<uint32_t, float>> lRadiusSearch;
	for (size_t i=0; i<iPoints.size(); i++)
	{
		Point& lPoint = *iPoints[i];

		lRadiusSearch.clear();
		lTree.knn<7>(i, lRadiusSearch);

		glm::dmat3 lCovariance;
		computeCovariance(iPoints, lRadiusSearch, lCovariance);

		glm::dvec3 lNormal;
		glm::dmat3 lEigenVectors;
		glm::dvec3 lEigenValues;
		if (computeEigen(lCovariance, lEigenVectors, lEigenValues))
		{
			lEigenValues[0] = fabs(lEigenValues[0]);
			lEigenValues[1] = fabs(lEigenValues[1]);
			lEigenValues[2] = fabs(lEigenValues[2]);

			int lMinimum = 0;
			for (int m=1; m<3; m++)
			{
				if (lEigenValues[m] < lEigenValues[lMinimum])
				{
					lMinimum = m;
				}
			}

			lNormal[0] = lEigenVectors[0][lMinimum];
			lNormal[1] = lEigenVectors[1][lMinimum];
			lNormal[2] = lEigenVectors[2][lMinimum];
		}
		else
		{
			lPoint.set(Point::DELETED);
		}

		lNormal = glm::normalize(lNormal);
		
		#if defined PACKED_NORMAL

			PackedNormalType& lNormalAttribute = *(PackedNormalType*)lPoint.getAttribute(iNormalIndex);

			Vec3IntPacked& lPacked = lNormalAttribute.mValue;
			lPacked.i32f3.x = floor(lNormal[0]*511);
			lPacked.i32f3.y = floor(lNormal[1]*511);
			lPacked.i32f3.z = floor(lNormal[2]*511);
			lPacked.i32f3.a = 0;

			#undef CLAMP

		#else

			NormalType& lNormalAttribute = *(NormalType*)lPoint.getAttribute(iNormalIndex);
			lNormalAttribute.mValue[0] = lNormal[0];
			lNormalAttribute.mValue[1] = lNormal[1];
			lNormalAttribute.mValue[2] = lNormal[2];

		#endif
	}
}

void NormalProcessor::computeCovariance(PointCloud& iCloud, std::vector<std::pair<uint32_t, float>>& iIndex, glm::dmat3& iMatrix)
{
	double lAccum[9];
	memset(lAccum,0,sizeof(lAccum));
	
	for (std::vector<std::pair<uint32_t, float>>::iterator lIter = iIndex.begin(); lIter != iIndex.end(); lIter++)
	{
		Point* lPoint = iCloud[lIter->first];
		double px = lPoint->position[0];
		double py = lPoint->position[1];
		double pz = lPoint->position[2];

		lAccum[0] += px*px;
		lAccum[1] += px*py;
		lAccum[2] += px*pz;
		lAccum[3] += py*py; // 4
		lAccum[4] += py*pz; // 5
		lAccum[5] += pz*pz; // 8
		lAccum[6] += px;
		lAccum[7] += py;
		lAccum[8] += pz;
	}

	uint32_t lSize = iIndex.size ();
	for (int i=0; i<9; i++)
	{
		lAccum [i] /= lSize;
	}

	iMatrix[0][0] = lAccum[0] - lAccum[6] * lAccum[6];
	iMatrix[1][1] = lAccum[3] - lAccum[7] * lAccum[7];
	iMatrix[2][2] = lAccum[5] - lAccum[8] * lAccum[8];
	iMatrix[1][0] = iMatrix[0][1] = lAccum[1] - lAccum[6]*lAccum[7];
	iMatrix[2][0] = iMatrix[0][2] = lAccum[2] - lAccum[6]*lAccum[8];
	iMatrix[2][1] = iMatrix[1][2] = lAccum[4] - lAccum[7]*lAccum[8];
}

bool NormalProcessor::computeEigen(glm::dmat3& iMatrix, glm::dmat3& iVectors, glm::dvec3& iValues, unsigned maxIterationCount)
{
	#define ROTATE(a,i,j,k,l) { double g = a[i][j]; h = a[k][l]; a[i][j] = g-s*(h+g*tau); a[k][l] = h+s*(g-h*tau); }

	iVectors = glm::dmat3(1.0);

	glm::vec3 b, z;

	for (int ip = 0; ip < 3; ip++)
	{
		iValues[ip] = iMatrix[ip][ip]; //Initialize b and d to the diagonal of a.
		b[ip] = iMatrix[ip][ip];
		z[ip] = 0; //This vector will accumulate terms of the form tapq as in equation (11.1.14)
	}

	for (size_t i = 1; i <= maxIterationCount; i++)
	{
		//Sum off-diagonal elements
		double sm = 0;
		for (int ip = 0; ip < 3 - 1; ip++)
		{
			for (int iq = ip + 1; iq < 3; iq++)
			{
				sm += fabs(iMatrix[ip][iq]);
			}
		}

		if (sm == 0) //The normal return, which relies on quadratic convergence to machine underflow.
		{
			return true;
		}

		double tresh = 0;
		if (i < 4)
		{
			tresh = sm / static_cast<double>(5 * 3*3); //...on the first three sweeps.
		}

		for (int ip = 0; ip < 3 - 1; ip++)
		{
			for (int iq = ip + 1; iq < 3; iq++)
			{
				double g = fabs(iMatrix[ip][iq]) * 100;
				//After four sweeps, skip the rotation if the off-diagonal element is small.
				if (i > 4 && fabs(iValues[ip] + g) == fabs(iValues[ip]) && fabs(iValues[iq]) + g == fabs(iValues[iq]))
				{
					iMatrix[ip][iq] = 0;
				}
				else if (fabs(iMatrix[ip][iq]) > tresh)
				{
					double h = iValues[iq] - iValues[ip];
					double t = 0;
					if (fabs(h) + g == fabs(h))
					{
						t = iMatrix[ip][iq] / h;
					}
					else
					{
						double theta = h / (2 * iMatrix[ip][iq]); //Equation (11.1.10).
						t = 1 / (fabs(theta) + sqrt(1 + theta*theta));
						if (theta < 0)
							t = -t;
					}

					double c = 1 / sqrt(t*t + 1);
					double s = t*c;
					double tau = s / (1 + c);
					h = t * iMatrix[ip][iq];
					z[ip] -= h;
					z[iq] += h;
					iValues[ip] -= h;
					iValues[iq] += h;
					iMatrix[ip][iq] = 0;

					//Case of rotations 1 <= j < p
					for (int j = 0; j + 1 <= ip; j++)
						ROTATE(iMatrix, j, ip, j, iq)
					//Case of rotations p < j < q
					for (int j = ip + 1; j + 1 <= iq; j++)
						ROTATE(iMatrix, ip, j, j, iq)
					//Case of rotations q < j <= n
					for (int j = iq + 1; j < 3; j++)
						ROTATE(iMatrix, ip, j, iq, j)
					//Last case
					for (int j = 0; j < 3; j++)
						ROTATE(iVectors, j, ip, j, iq)
				}
			}
		}

		//update b, d and z
		for (int ip = 0; ip < 3; ip++)
		{
			b[ip] += z[ip];
			iValues[ip] = b[ip];
			z[ip] = 0;
		}
	}

	//Too many iterations!
	return false;
}
