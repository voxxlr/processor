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

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "packetProcessor.h"

#include "../kdFileTree.h"

PacketProcessor::PacketProcessor()
: InorderOperation("Lod")
, mTotalStorage(0)
, mTotalWritten(0)
{
}

void PacketProcessor::processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	// filter points in overlap
	std::vector<uint32_t> lIndex;
	lIndex.reserve(iCloud.size());
	for (int i=0; i<iCloud.size(); i++)
	{
		Point& lPoint = *iCloud[i];
		if (lPoint.inside(iNode.min, iNode.max))
		{
			lIndex.push_back(i);
		}
	}
	iNode.mCount = lIndex.size();

	// create header
	json_spirit::mObject lInfo;
	lInfo["path"] = iNode.mPath;
	lInfo["resolution"] = iCloud.mResolution;

	writePacket(lInfo, iCloud, lIndex);

	if (iNode.mPath == "n")
	{
		// save the root info
		mRootInfo = lInfo;
	}
}

void PacketProcessor::processInternal(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	// filter points without overlap
	std::vector<uint32_t> lIndex;
	lIndex.reserve(iCloud.size());
	for (int i=0; i<iCloud.size(); i++)
	{
		Point& lPoint = *iCloud[i];
		if (lPoint.inside(iNode.min, iNode.max))
		{
			lIndex.push_back(i);
		}
	}
	iNode.mCount = lIndex.size();

	// create header
	json_spirit::mObject lInfo;
	lInfo["path"] = iNode.mPath;
	lInfo["resolution"] = iCloud.mResolution;
	json_spirit::mArray lMin;
	lMin.push_back(json_spirit::mValue(iNode.min[0]));
	lMin.push_back(json_spirit::mValue(iNode.min[1]));
	lMin.push_back(json_spirit::mValue(iNode.min[2]));
	lInfo["min"] = lMin;

	json_spirit::mArray lMax;
	lMax.push_back(json_spirit::mValue(iNode.max[0]));
	lMax.push_back(json_spirit::mValue(iNode.max[1]));
	lMax.push_back(json_spirit::mValue(iNode.max[2]));
	lInfo["max"] = lMax;

	// Children
	lInfo["split"] = iNode.mSplit;
	lInfo["axis"] = iNode.mAxis;

	json_spirit::mObject lLowInfo;
	lLowInfo["count"] = iNode.mChildLow->mCount;
	lInfo["low"] = lLowInfo;

	json_spirit::mObject lHighInfo;
	lHighInfo["count"] = iNode.mChildHigh->mCount;
	lInfo["high"] = lHighInfo;

	writePacket(lInfo, iCloud, lIndex);  

	if (iNode.mPath == "n")
	{
		// save the root info
		mRootInfo = lInfo;
	}
}


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

		if (lMax[0] < iTestMin[0] || lMax[0] > iTestMax[0])
		{
			std::cout << "fail 1 \n";
		}
		if (lMax[1] < iTestMin[1] || lMax[1] > iTestMax[1])
		{
			std::cout << "fail 2 \n";
		}
		if (lMax[2] < iTestMin[2] || lMax[2] > iTestMax[2])
		{
			std::cout << "fail 3 \n";
		}
		kdTest(lMin, lMax, iTree, 2*iIndex+1, iTestMin, iTestMax);

		memcpy(lMin, iMin, sizeof(lMin));
		memcpy(lMax, iMax, sizeof(lMax));
		lMin[lAxis] = lSplit;

		if (lMin[0] < iTestMin[0] || lMin[0] > iTestMax[0])
		{
			std::cout << "fail 1 \n";
		}
		if (lMin[1] < iTestMin[1] || lMin[1] > iTestMax[1])
		{
			std::cout << "fail 2 \n";
		}
		if (lMin[2] < iTestMin[2] || lMin[2] > iTestMax[2])
		{
			std::cout << "fail 3 \n";
		}
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

		//std::cout << (int)lAxis << "   " << lSplit << "          " << lWx << "    " << lWy << "    " << lWz << "\n";;

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

void PacketProcessor::writePacket(json_spirit::mObject& iInfo, PointCloud& iCloud, std::vector<uint32_t>& iIndex)
{
	// open file
	std::string lName = "root/" + iInfo["path"].get_str() + ".bin";
	FILE* lFile = fopen(lName.c_str(), "wb+");

	uint32_t lPointCount = iIndex.size();

	BOOST_LOG_TRIVIAL(info) << "Writing " << iInfo["path"].get_str() << ":" << lName.c_str() << "  :  " << lPointCount;

	//
	// Create Header
	//
	iInfo["count"] = (long)lPointCount;
	iInfo["version"] = "2021.08.28";

	// compute tight AABB
	float min[3];
	min[0] = std::numeric_limits<float>::max();
	min[1] = std::numeric_limits<float>::max();
	min[2] = std::numeric_limits<float>::max();

	float max[3];
	max[0] = -std::numeric_limits<float>::max();
	max[1] = -std::numeric_limits<float>::max();
	max[2] = -std::numeric_limits<float>::max();

	for (std::vector<Point*>::iterator lIter = iCloud.begin(); lIter != iCloud.end(); lIter++)
	{
		Point& lPoint = *(*lIter);
		min[0] = std::min<float>(lPoint.position[0], min[0]);
		min[1] = std::min<float>(lPoint.position[1], min[1]);
		min[2] = std::min<float>(lPoint.position[2], min[2]);
		max[0] = std::max<float>(lPoint.position[0], max[0]);
		max[1] = std::max<float>(lPoint.position[1], max[1]);
		max[2] = std::max<float>(lPoint.position[2], max[2]);
	}

	json_spirit::mArray lMin;
	lMin.push_back(json_spirit::mValue(min[0]));
	lMin.push_back(json_spirit::mValue(min[1]));
	lMin.push_back(json_spirit::mValue(min[2]));
	iInfo["min"] = lMin;

	json_spirit::mArray lMax;
	lMax.push_back(json_spirit::mValue(max[0]));
	lMax.push_back(json_spirit::mValue(max[1]));
	lMax.push_back(json_spirit::mValue(max[2]));
	iInfo["max"] = lMax;

	json_spirit::mObject lPositionAttribute;
	lPositionAttribute["name"] = Attribute::POSITION;
	lPositionAttribute["size"] = 3;
	lPositionAttribute["type"] = SingleAttribute<float>::cTypeName;

	json_spirit::mArray lAttributes;
	lAttributes.push_back(lPositionAttribute);
	((PointCloudAttributes&)iCloud).toJson(lAttributes);
	iInfo["attributes"] = lAttributes;

	//
	// Write Header
	//
	std::stringstream lStream;
	json_spirit::write_stream(json_spirit::mValue(iInfo), lStream, json_spirit::pretty_print);
	std::string lHeader = lStream.str();

	uint32_t lPointer = lHeader.length();
	fwrite(&lPointer, sizeof(lPointer), 1, lFile);
	fwrite(lHeader.c_str(), lPointer, 1, lFile);
	lPointer += sizeof(lPointer);
	lPointer = align4(lFile, lPointer);

	//
	// Create Data
	//

	// arrange points in kdtree and create split index
	std::vector<float> lTree;
	int32_t lDepth = log(lPointCount/100.0f)/log(2.0f); 
	if (lDepth > 0)
	{
		json_spirit::mArray lMinJson = iInfo["min"].get_array();
		float lMin[3];
		lMin[0] = lMinJson[0].get_real();
		lMin[1] = lMinJson[1].get_real();
		lMin[2] = lMinJson[2].get_real();

		json_spirit::mArray lMaxJson = iInfo["max"].get_array();
		float lMax[3];
		lMax[0] = lMaxJson[0].get_real();
		lMax[1] = lMaxJson[1].get_real();
		lMax[2] = lMaxJson[2].get_real();

		lTree.resize(2*lDepth-1, 0);
		kdTreeSort(iCloud, 0, lPointCount, iIndex, lMin, lMax, lTree, 0);
	}

	// write points
	fwrite(&lPointCount, sizeof(lPointCount),1,lFile);
	lPointer += sizeof(lPointCount);
	for (int j=0; j<lPointCount; j++)
	{
		Point* lPoint = iCloud[iIndex[j]];
		fwrite(lPoint->position, sizeof(lPoint->position),1,lFile);
	}
	lPointer += lPointCount*sizeof(sizeof(Point::position));
	lPointer = align4(lFile, lPointer);

	for (int i=0; i<iCloud.attributeCount(); i++)
	{
		for (int j=0; j<lPointCount; j++)
		{
			Point* lPoint = iCloud[iIndex[j]];
			Attribute* lAttribute = lPoint->getAttribute(i);
			lAttribute->write(lFile);
			lPointer += lAttribute->bytesPerPoint();
		}
		lPointer = align4(lFile, lPointer);
	}

	// write split index
	fwrite(&lTree[0], 4, lTree.size(), lFile);

	mCountLock.lock();
	mTotalWritten += iIndex.size();
	mTotalStorage += ftell(lFile);
	mCountLock.unlock();

	fclose(lFile);
}

