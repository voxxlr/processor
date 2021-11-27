///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2016 Geist Software Labs

///////////////////////////////////////////////////////////////////////////

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <math.h>

#if defined (__linux__)
	#include <sys/stat.h>
	#include <sys/types.h>
#endif

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem.hpp>

#include "packetProcessor.h"

#include "../kdFileTree.h"

#define PACKED_NORMAL 1

PacketProcessor::PacketProcessor()
: InorderOperation("Packetizer")
, mTotalStorage(0)
, mTotalWritten(0)
{
}

void PacketProcessor::initTraveral(PointCloudAttributes& iAttributes)
{
	InorderOperation::initTraveral(iAttributes);

	if (!boost::filesystem::exists("./root"))
	{
		boost::filesystem::create_directory("./root");
	}

#if defined PACKED_NORMAL
	PackedNormalType lAttribute;
#else
	NormalType lAttribute;
#endif	

	iAttributes.createAttribute(Attribute::NORMAL, lAttribute);
}

void PacketProcessor::completeTraveral(PointCloudAttributes& iAttributes)
{
	// write root
	std::ofstream lStream("root.json");
	json_spirit::write_stream(json_spirit::mValue(mRootInfo), lStream);
	lStream.close();
	/*
	#if defined (__linux__)
		chmod("./root.json", 0777);
	#endif	
	*/
}

void PacketProcessor::computeNormals(PointCloud& iPoints, uint32_t iNormalIndex)
{
	KdTree<KdSpatialDomain> lTree(iPoints, 100);
	lTree.construct();

	//
	// compute normals
	//
	std::vector<std::pair<uint32_t, float>> lRadiusSearch;
	for (size_t i = 0; i < iPoints.size(); i++)
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
			for (int m = 1; m < 3; m++)
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
		lPacked.i32f3.x = floor(lNormal[0] * 511);
		lPacked.i32f3.y = floor(lNormal[1] * 511);
		lPacked.i32f3.z = floor(lNormal[2] * 511);
		lPacked.i32f3.a = 0;

#else

		NormalType& lNormalAttribute = *(NormalType*)lPoint.getAttribute(iNormalIndex);
		lNormalAttribute.mValue[0] = lNormal[0];
		lNormalAttribute.mValue[1] = lNormal[1];
		lNormalAttribute.mValue[2] = lNormal[2];

#endif
	}
}

void PacketProcessor::computeCovariance(PointCloud& iCloud, std::vector<std::pair<uint32_t, float>>& iIndex, glm::dmat3& iMatrix)
{
	double lAccum[9];
	memset(lAccum, 0, sizeof(lAccum));

	for (std::vector<std::pair<uint32_t, float>>::iterator lIter = iIndex.begin(); lIter != iIndex.end(); lIter++)
	{
		Point* lPoint = iCloud[lIter->first];
		double px = lPoint->position[0];
		double py = lPoint->position[1];
		double pz = lPoint->position[2];

		lAccum[0] += px * px;
		lAccum[1] += px * py;
		lAccum[2] += px * pz;
		lAccum[3] += py * py; // 4
		lAccum[4] += py * pz; // 5
		lAccum[5] += pz * pz; // 8
		lAccum[6] += px;
		lAccum[7] += py;
		lAccum[8] += pz;
	}

	uint32_t lSize = iIndex.size();
	for (int i = 0; i < 9; i++)
	{
		lAccum[i] /= lSize;
	}

	iMatrix[0][0] = lAccum[0] - lAccum[6] * lAccum[6];
	iMatrix[1][1] = lAccum[3] - lAccum[7] * lAccum[7];
	iMatrix[2][2] = lAccum[5] - lAccum[8] * lAccum[8];
	iMatrix[1][0] = iMatrix[0][1] = lAccum[1] - lAccum[6] * lAccum[7];
	iMatrix[2][0] = iMatrix[0][2] = lAccum[2] - lAccum[6] * lAccum[8];
	iMatrix[2][1] = iMatrix[1][2] = lAccum[4] - lAccum[7] * lAccum[8];
}

bool PacketProcessor::computeEigen(glm::dmat3& iMatrix, glm::dmat3& iVectors, glm::dvec3& iValues, unsigned maxIterationCount)
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
			tresh = sm / static_cast<double>(5 * 3 * 3); //...on the first three sweeps.
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
						t = 1 / (fabs(theta) + sqrt(1 + theta * theta));
						if (theta < 0)
							t = -t;
					}

					double c = 1 / sqrt(t * t + 1);
					double s = t * c;
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


void PacketProcessor::processNode(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	computeNormals(iCloud, iCloud.getAttributeIndex(Attribute::NORMAL));

	//
	// compute tight AABB
	//
	float min[3];
	min[0] = std::numeric_limits<float>::max();
	min[1] = std::numeric_limits<float>::max();
	min[2] = std::numeric_limits<float>::max();

	float max[3];
	max[0] = -std::numeric_limits<float>::max();
	max[1] = -std::numeric_limits<float>::max();
	max[2] = -std::numeric_limits<float>::max();


	// filter points in overlap
	std::vector<uint32_t> lIndex;
	lIndex.reserve(iCloud.size());
	for (int i = 0; i < iCloud.size(); i++)
	{
		Point& lPoint = *iCloud[i];
		if (lPoint.inside(iNode.min, iNode.max))
		{
			lIndex.push_back(i);
			min[0] = std::min<float>(lPoint.position[0], min[0]);
			min[1] = std::min<float>(lPoint.position[1], min[1]);
			min[2] = std::min<float>(lPoint.position[2], min[2]);
			max[0] = std::max<float>(lPoint.position[0], max[0]);
			max[1] = std::max<float>(lPoint.position[1], max[1]);
			max[2] = std::max<float>(lPoint.position[2], max[2]);
		}
	}
	
	iNode.mCount = lIndex.size();

	uint32_t lPointCount = lIndex.size(); // should be uint32_t

	BOOST_LOG_TRIVIAL(info) << "Writing " << iNode.mPath << "  :  " << lPointCount;

	//
	// Create Header
	//
	json_spirit::mObject lInfo;
	lInfo["path"] = iNode.mPath;
	lInfo["resolution"] = iCloud.mResolution;
	lInfo["count"] = (long)lPointCount;
	lInfo["version"] = "2021.08.28";

	if (iNode.mChildHigh && iNode.mChildLow)
	{
		// Children
		lInfo["split"] = iNode.mSplit;
		lInfo["axis"] = iNode.mAxis;
		/*
		json_spirit::mObject lLowInfo;
		lLowInfo["count"] = iNode.mChildLow->mCount;
		lInfo["low"] = lLowInfo;

		json_spirit::mObject lHighInfo;
		lHighInfo["count"] = iNode.mChildHigh->mCount;
		lInfo["high"] = lHighInfo;
		*/
	}

	json_spirit::mArray lMin;
	lMin.push_back(json_spirit::mValue(min[0]));
	lMin.push_back(json_spirit::mValue(min[1]));
	lMin.push_back(json_spirit::mValue(min[2]));
	lInfo["min"] = lMin;

	json_spirit::mArray lMax;
	lMax.push_back(json_spirit::mValue(max[0]));
	lMax.push_back(json_spirit::mValue(max[1]));
	lMax.push_back(json_spirit::mValue(max[2]));
	lInfo["max"] = lMax;


	json_spirit::mObject lPositionAttribute;
	lPositionAttribute["name"] = Attribute::POSITION;
	lPositionAttribute["size"] = 3;
	lPositionAttribute["type"] = SingleAttribute<float>::cTypeName;

	json_spirit::mArray lAttributes;
	lAttributes.push_back(lPositionAttribute);
	((PointCloudAttributes&)iCloud).toJson(lAttributes);

	lInfo["attributes"] = lAttributes;

	//
	// Write Header
	//
	std::string lName = "root/" + iNode.mPath + ".bin";
	FILE* lFile = fopen(lName.c_str(), "wb+");

	std::stringstream lStream;
	json_spirit::write_stream(json_spirit::mValue(lInfo), lStream, json_spirit::pretty_print);
	std::string lHeader = lStream.str();

	uint32_t lPointer = lHeader.length();
	fwrite(&lPointer, sizeof(lPointer), 1, lFile);
	fwrite(lHeader.c_str(), lPointer, 1, lFile);
	lPointer += sizeof(lPointer);
	lPointer = align4(lFile, lPointer);

	//
	// Sort points as in kdtree and create split index
	//
	std::vector<float> lTree;
	int32_t lDepth = log(lPointCount / 100.0f) / log(2.0f);
	if (lDepth > 0)
	{
		lTree.resize(2 * lDepth - 1, 0);
		kdTreeSort(iCloud, 0, lPointCount, lIndex, min, max, lTree, 0);
	}

	//
	// Write points
	//
	fwrite(&lPointCount, sizeof(lPointCount), 1, lFile);
	lPointer += sizeof(lPointCount);
	for (int j = 0; j < lPointCount; j++)
	{
		Point* lPoint = iCloud[lIndex[j]];
		fwrite(lPoint->position, sizeof(lPoint->position), 1, lFile);
	}
	lPointer += lPointCount * sizeof(sizeof(Point::position));
	lPointer = align4(lFile, lPointer);

	for (int i = 0; i < iCloud.attributeCount(); i++)
	{
		for (int j = 0; j < lPointCount; j++)
		{
			Point* lPoint = iCloud[lIndex[j]];
			Attribute* lAttribute = lPoint->getAttribute(i);
			lAttribute->write(lFile);
			lPointer += lAttribute->bytesPerPoint();
		}
		lPointer = align4(lFile, lPointer);
	}

	// write split index
	fwrite(&lTree[0], 4, lTree.size(), lFile);

	mCountLock.lock();
	mTotalWritten += lPointCount;
	mTotalStorage += ftell(lFile);
	mCountLock.unlock();

	fclose(lFile);

	if (iNode.mPath == "n")
	{
		// save the root info
		mRootInfo = lInfo;
	}
}

//
//
//



//
//
//

void PacketProcessor::kdTest(float* iMin, float* iMax, std::vector<float>& iTree, int iIndex, float* iTestMin, float* iTestMax)
{
	if (iIndex < iTree.size())
	{
		uint8_t lAxis;
		float lWx = iMax[0] - iMin[0];
		float lWy = iMax[1] - iMin[1];
		float lWz = iMax[2] - iMin[2];
		if (lWx >= lWy && lWx >= lWz) lAxis = Block::X;
		if (lWy >= lWx && lWy >= lWz) lAxis = Block::Y;
		if (lWz >= lWx && lWz >= lWy) lAxis = Block::Z;

		// split distance is in index
		float lSplit = iTree[iIndex];
		// split index is half the points

		float lMax[3];
		float lMin[3];

		memcpy(lMin, iMin, sizeof(lMin));
		memcpy(lMax, iMax, sizeof(lMax));
		lMax[lAxis] = lSplit;
		kdTest(lMin, lMax, iTree, 2*iIndex+1, iTestMin, iTestMax);

		memcpy(lMin, iMin, sizeof(lMin));
		memcpy(lMax, iMax, sizeof(lMax));
		lMin[lAxis] = lSplit;
		kdTest(lMin, lMax, iTree, 2*iIndex+2, iTestMin, iTestMax);
	}
}

void PacketProcessor::kdTreeSort(PointCloud& iCloud, uint32_t iLower, uint32_t iUpper, std::vector<uint32_t>& iIndex, float* iMin, float* iMax, std::vector<float>& iTree, int iNodeIndex)
{
 	if (iNodeIndex < iTree.size())
	{
		uint8_t lAxis;
		float lWx = iMax[0] - iMin[0];
		float lWy = iMax[1] - iMin[1];
		float lWz = iMax[2] - iMin[2];
		if (lWx >= lWy && lWx >= lWz) lAxis = Block::X;
		if (lWy >= lWx && lWy >= lWz) lAxis = Block::Y;
		if (lWz >= lWx && lWz >= lWy) lAxis = Block::Z;

		KdSpatialDomain lPosition(iCloud);
		lPosition.axisSort(iIndex, iLower, iUpper, lAxis);

		uint32_t lMiddle = (iUpper+iLower)/2;
		Point& lPoint = *iCloud[iIndex[lMiddle]];
		float lSplit = lPoint.position[lAxis];
		iTree[iNodeIndex] = lSplit;

		float lMax[3];
		float lMin[3];

		memcpy(lMin, iMin, sizeof(lMin));
		memcpy(lMax, iMax, sizeof(lMax));
		lMax[lAxis] = lSplit;
		kdTreeSort(iCloud, iLower, lMiddle, iIndex, lMin, lMax, iTree, 2*iNodeIndex+1);

		memcpy(lMin, iMin, sizeof(lMin));
		memcpy(lMax, iMax, sizeof(lMax));
		lMin[lAxis] = lSplit;
		kdTreeSort(iCloud, lMiddle, iUpper, iIndex, lMin, lMax, iTree, 2*iNodeIndex+2);
	}
}

uint32_t PacketProcessor::align4(FILE* iFile, uint32_t iPointer)
{
	int lAlignment = iPointer%4;
	if (lAlignment)
	{
		for (int i=0; i<4-lAlignment; i++)
		{
			char lChar = 0;
			fwrite(&lChar, 1, 1, iFile);
			iPointer++;
		}
	}
	return iPointer;
}

void PacketProcessor::writePacket(KdFileTreeNode& iNode, PointCloud& iCloud)
{

}

