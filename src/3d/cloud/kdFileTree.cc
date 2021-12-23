#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <stdint.h>
#include <ctime>
#include <cstdlib>
#include <stdint.h>
#include <algorithm>

#include "kdFileTree.h"
#include "pointBuffer.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/thread_pool.hpp>
#include <boost/asio.hpp>

#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/attribute_value_set.hpp>

#include "voxelHashIndex2.h"

#define TIMING 1

extern unsigned long long availableMemory();

KdFileTreeNode::KdFileTreeNode(json_spirit::mObject& iConfig)
: mChildLow(0)
, mChildHigh(0)
, mCount(0)
, mFile(0)
, mThreaded(false)
, mPath(iConfig["path"].get_str())
, mVolumeLimit(0) 
{
	min[0] = iConfig["minx"].get_real();
	min[1] = iConfig["miny"].get_real();
	min[2] = iConfig["minz"].get_real();

	max[0] = iConfig["maxx"].get_real();
	max[1] = iConfig["maxy"].get_real();	
	max[2] = iConfig["maxz"].get_real();

	if (iConfig.find("low") != iConfig.end() && iConfig.find("high") != iConfig.end())
	{
		mChildLow = new KdFileTreeNode(iConfig["low"].get_obj());
		mChildHigh = new KdFileTreeNode(iConfig["high"].get_obj());
		mHeight = std::max(mChildLow->mHeight, mChildHigh->mHeight) + 1;
		mSplit = iConfig["split"].get_real();
		mAxis = iConfig["axis"].get_int();
	}
	else
	{
		mHeight = 0;
	}
}


KdFileTreeNode::KdFileTreeNode(std::string iPath, uint32_t iHeight)
: mAxis(0)
, mChildLow(0)
, mChildHigh(0)
, mCount(0)
, mFile(0)
, mThreaded(false)
, mHeight(iHeight)
, mPath(iPath)
, mVolumeLimit(0)
, mAlive(true)
{
	min[0] = std::numeric_limits<float>::max();
	min[1] = std::numeric_limits<float>::max();
	min[2] = std::numeric_limits<float>::max();

	max[0] = -std::numeric_limits<float>::max();
	max[1] = -std::numeric_limits<float>::max();
	max[2] = -std::numeric_limits<float>::max();

	median[0] = 0;
	median[1] = 0;
	median[2] = 0;
}

KdFileTreeNode::~KdFileTreeNode()
{
	if (mChildLow)
	{
		delete mChildLow;
	}
	if (mChildHigh)
	{
		delete mChildHigh;
	}
}

#define PLANK_LENGTH  0.0005f

bool KdFileTreeNode::grow(std::string iIndent, uint32_t iLeafsize)
{
	//BOOST_LOG_TRIVIAL(info) << iIndent << mPath;

	mAlive = false;
	if (mChildLow && mChildHigh)
	{
		if (mChildLow->mAlive)
		{
			if (mChildLow->grow(iIndent+"  ", iLeafsize))
			{
				mAlive = true;
			}
		}

		if (mChildHigh->mAlive)
		{
			if (mChildHigh->grow(iIndent+"  ", iLeafsize))
			{
				mAlive = true;
			}
		}
	}
	else
	{
		if (mCount > iLeafsize)
		{
			float lMaxD = std::max(std::max(max[2] - min[2], max[1] - min[1]), max[0] - min[0]) / PLANK_LENGTH; // convert to Voxxlr's Plank length 
			if (mCount < lMaxD * lMaxD * lMaxD)
			{
				// find split axis
				float lT = 0;
				for (int i = 0; i < 3; i++)
				{
					float dToBox = std::min(max[i] - median[i], median[i] - min[i]);
					if (dToBox > lT)
					{
						lT = dToBox;
						mAxis = i;
					}
				}

				mSplit = median[mAxis];

				mChildLow = new KdFileTreeNode(mPath + "0", mHeight + 1);
				memcpy(mChildLow->min, min, sizeof(min));
				memcpy(mChildLow->max, max, sizeof(max));
				mChildLow->max[mAxis] = mSplit;

				mChildHigh = new KdFileTreeNode(mPath + "1", mHeight + 1);
				memcpy(mChildHigh->min, min, sizeof(min));
				memcpy(mChildHigh->max, max, sizeof(max));
				mChildHigh->min[mAxis] = mSplit;

				BOOST_LOG_TRIVIAL(info) << iIndent << mPath << ": points = " << mCount << " split " << (int)mAxis << ":" << mSplit;
				mAlive = true;
			}
			else
			{
				float lVolume = (max[2] - min[2]) * (max[1] - min[1]) * (max[0] - min[0]);
				mVolumeLimit = std::max((uint64_t) 1, (uint64_t)(lVolume / (PLANK_LENGTH * PLANK_LENGTH * PLANK_LENGTH)));
				BOOST_LOG_TRIVIAL(info) << iIndent << "degenerate " << mCount << " points in " << lVolume << " m2. Limiting volume to " << mVolumeLimit;
				mAlive = false;
			}
		}
		else
		{
			mAlive = false;
		}
	}
	return mAlive;
}

void KdFileTreeNode::feed(uint8_t* iBuffer, uint32_t iStride, size_t iCount, boost::thread_group*& iGroup, int32_t iThreadCount)
{
	if (mChildLow && mChildHigh)
	{
		if (mChildLow->mAlive)
		{
			mChildLow->feed(iBuffer, iStride, iCount, iGroup, iThreadCount);
		}

		if (mChildHigh->mAlive)
		{
			mChildHigh->feed(iBuffer, iStride, iCount, iGroup, iThreadCount);
		}
	}
	else
	{
		if (mAlive)
		{
			iGroup->add_thread(new boost::thread (&KdFileTreeNode::recordChunk, this, iBuffer, iStride, iCount));
			if (iGroup->size() >= iThreadCount)
			{
			//	BOOST_LOG_TRIVIAL(info) << "Waiting for " << iGroup->size() << " theads to complete ";
				iGroup->join_all();
				delete iGroup;
				iGroup = new boost::thread_group();
			}
		}
	}
}

void KdFileTreeNode::recordChunk(uint8_t* iBuffer, uint32_t iStride, size_t iCount) 
{
	for (uint32_t i=0; i<iCount; i++)
	{
		float* lPosition = (float*)(iBuffer + i*iStride); 
		if (contains(lPosition))
		{
			mCount++;
			median[0] += (lPosition[0]-median[0])/mCount;
			median[1] += (lPosition[1]-median[1])/mCount;
			median[2] += (lPosition[2]-median[2])/mCount;
		}
	}
}

void KdFileTreeNode::openFiles(PointCloudAttributes& iAttributes, float iResolution)
{
	if (mChildLow && mChildHigh)
	{
		mChildLow->openFiles(iAttributes, iResolution);
		mChildHigh->openFiles(iAttributes, iResolution);
	}
	else
	{
		mFile = PointCloud::writeHeader(mPath, iAttributes, 0, min, max, iResolution);
		mCount = 0;
	}
}

void KdFileTreeNode::writeChunk(uint8_t* iBuffer, uint32_t iStride, size_t iCount, float iOverlap)
{
	float lMin[3];
	memcpy(lMin, min, sizeof(min));
	lMin[0] -= iOverlap;
	lMin[1] -= iOverlap;
	lMin[2] -= iOverlap;

	float lMax[3];
	memcpy(lMax, max, sizeof(max));
	lMax[0] += iOverlap;
	lMax[1] += iOverlap;
	lMax[2] += iOverlap;

	uint8_t* lPointer = iBuffer;
	for (size_t i = 0; i < iCount; i++)
	{
		if (mVolumeLimit && mCount > mVolumeLimit)
		{
			break;
		}
		lPointer = iBuffer + i * iStride;

		float* lP = (float*)lPointer;
		if (lP[0] >= lMin[0] && lP[0] <= lMax[0] && lP[1] >= lMin[1] && lP[1] <= lMax[1] && lP[2] >= lMin[2] && lP[2] <= lMax[2])
		{
			fwrite(lPointer, 1, iStride, mFile);
			mCount++;
		}
	}
}

void KdFileTreeNode::write(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, float iOverlap, boost::thread_group*& iGroup, int32_t iThreadCount)
{
	if (mChildLow && mChildHigh)
	{
		mChildLow->write(iBuffer, iStride, iCount, iOverlap, iGroup, iThreadCount);
		mChildHigh->write(iBuffer, iStride, iCount, iOverlap, iGroup, iThreadCount);
	}
	else
	{
		iGroup->add_thread(new boost::thread (&KdFileTreeNode::writeChunk, this, iBuffer, iStride, iCount, iOverlap));
		if (iGroup->size() >= iThreadCount)
		{
			iGroup->join_all();
			delete iGroup;
			iGroup = new boost::thread_group();
		}
	}
}

json_spirit::mObject KdFileTreeNode::closeFiles()
{
	json_spirit::mObject lNode;
	lNode["minx"] = min[0];
	lNode["miny"] = min[1];
	lNode["minz"] = min[2];
	lNode["maxx"] = max[0];
	lNode["maxy"] = max[1];
	lNode["maxz"] = max[2];
	lNode["count"] = mCount;
	lNode["path"] = mPath;

	if (mChildLow && mChildHigh)
	{
		json_spirit::mObject lLow = mChildLow->closeFiles();
		json_spirit::mObject lHigh = mChildHigh->closeFiles();

		mThreaded = false;

		lNode["low"] = lLow;
		lNode["high"] = lHigh;
		lNode["axis"] = mAxis;
		lNode["split"] = mSplit;
	}
	else
	{
		PointCloud::updateSize(mFile, mCount);
		fclose(mFile);
	}

	return lNode;
}

void KdFileTreeNode::deleteFiles()
{
	if (mChildLow)
	{
		mChildLow->deleteFiles();
	}
	if (mChildHigh)
	{
		mChildHigh->deleteFiles();
	}

	std::remove(std::string(mPath + ".ply").c_str());
}

bool KdFileTreeNode::prune()
{
	bool lLeaf = !(mChildLow && mChildHigh);

	if (!lLeaf)
	{
		bool lLo = mChildLow->prune();
		bool lHi = mChildHigh->prune();
		if (lLo && lHi)
		{
			delete mChildLow;
			mChildLow = 0;
			delete mChildHigh;
			mChildHigh = 0;
		}
		return false;
	}

	return lLeaf;
}

uint64_t KdFileTreeNode::collapse(FILE* iFile, uint32_t iStride, float* iMin, float* iMax)
{
	if (mChildLow && mChildHigh)
	{
		return mChildLow->collapse(iFile, iStride, iMin, iMax) + mChildHigh->collapse(iFile, iStride, iMin, iMax);
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "Collapsing " << mPath;

		
		uint64_t lCount;
		FILE* lFile = PointCloud::readHeader(mPath, 0, lCount);

		PointBuffer lPointBuffer(lFile, lCount, iStride, lCount*iStride);
		lPointBuffer.begin();
		uint64_t lWritten = 0;
		while (!lPointBuffer.end())
		{
			PointBuffer::Chunk& lChunk = lPointBuffer.next();

			uint8_t* lPointer = lChunk.mData;
			for (int i = 0; i < lChunk.mSize; i++)
			{
				float* lPosition = (float*)(lChunk.mData + i * iStride);

				if (contains(lPosition)) // may be outside due to overlap
				{
					iMin[0] = std::min<float>(lPosition[0], iMin[0]);
					iMin[1] = std::min<float>(lPosition[1], iMin[1]);
					iMin[2] = std::min<float>(lPosition[2], iMin[2]);
					iMax[0] = std::max<float>(lPosition[0], iMax[0]);
					iMax[1] = std::max<float>(lPosition[1], iMax[1]);
					iMax[2] = std::max<float>(lPosition[2], iMax[2]);
					fwrite(lChunk.mData + i * iStride, iStride, 1, iFile);
					lWritten++;
				}
			}
		}

		fclose(lFile);
		return lWritten;
	}
}


//const float KdFileTree::SIGMA = 1.479;
//const float KdFileTree::SIGMA = 1.25992104989f*1.06; // cube root of 3 + experimentally deduced factor TODO compute this factor
const float KdFileTree::SIGMA = 1.41421356237f; // cube root of 3 + experimentally deduced factor TODO compute this factor

//std::thread::hardware_concurrency()

KdFileTree::KdFileTree()
: mRoot(new KdFileTreeNode("n"))
{
}

void KdFileTree::load(std::string iName)
{
	std::ifstream lStream("index.json");
	json_spirit::mValue lJson;
	json_spirit::read_stream(lStream, lJson);
	lStream.close();

	json_spirit::mObject& lIndex = lJson.get_obj();
	mPointAttributes.load(lIndex["attributes"].get_array());
	mRoot = new KdFileTreeNode(lIndex);
}

void KdFileTree::construct(std::string iName, uint32_t iLeafsize, float iOverlap)
{
	uint64_t lPointCount;
	float lResolution;
	FILE* lFile = PointCloud::readHeader(iName, &mPointAttributes, lPointCount, mRoot->min, mRoot->max, &lResolution);

	BOOST_LOG_TRIVIAL(info) << "Constructing filetree for " << lPointCount << " points:";
	BOOST_LOG_TRIVIAL(info) << "   Leafsize " << iLeafsize << " points ";
	BOOST_LOG_TRIVIAL(info) << "   Overlap " << iOverlap << " meters ";

	// pass one - build tree
	PointBuffer lPointBuffer(lFile, lPointCount, mPointAttributes.bytesPerPoint() + 3 * sizeof(float), availableMemory());
	do
	{
		lPointBuffer.begin();
		while (!lPointBuffer.end())
		{
			PointBuffer::Chunk& lChunk = lPointBuffer.next();

			boost::thread_group* lGroup = new boost::thread_group();
			mRoot->feed(lChunk.mData, lPointBuffer.mStride, lChunk.mSize, lGroup, std::thread::hardware_concurrency());
			lGroup->join_all();
			delete lGroup;
		}

	} while (mRoot->grow("", iLeafsize));

	BOOST_LOG_TRIVIAL(info) << "Opening Files";
	mRoot->openFiles(mPointAttributes, lResolution);

	// pass two - write points into leaves
	BOOST_LOG_TRIVIAL(info) << "Writing file tree ";
	lPointBuffer.begin();
	while (!lPointBuffer.end())
	{
		PointBuffer::Chunk& lChunk = lPointBuffer.next();

		boost::thread_group* lGroup = new boost::thread_group();
		mRoot->write(lChunk.mData, lPointBuffer.mStride, lChunk.mSize, iOverlap, lGroup, std::thread::hardware_concurrency());
		lGroup->join_all();
		delete lGroup;
	}
	fclose(lFile);

	json_spirit::mObject lRoot = mRoot->closeFiles();

	std::ofstream lStream;
	lStream.open(std::string("index.json").c_str());
	json_spirit::mArray lArray;
	mPointAttributes.toJson(lArray);
	lRoot["attributes"] = lArray;
	json_spirit::write_stream(json_spirit::mValue(lRoot), lStream, json_spirit::pretty_print);
	lStream.close();
}

const float KdFileTree::MIN_RESOLUTION = 0.001f; // 1 mm

bool KdFileTree::hasAttribute(const std::string& iName)
{
	return mPointAttributes.hasAttribute(iName);
};


//
// Collaps the file tree back into a single file
//

uint64_t KdFileTree::collapse(std::string iName, float iResolution)
{
	boost::this_thread::sleep_for(boost::chrono::milliseconds(20)); // give os time to GC ... ???

	float lMin[3];
	float lMax[3];
	memcpy(lMin, PointCloud::MIN, sizeof(lMin));
	memcpy(lMax, PointCloud::MAX, sizeof(lMax));

	FILE* lFile = PointCloud::writeHeader(iName, mPointAttributes, 0, 0, 0, iResolution);
	uint64_t lTotal = mRoot->collapse(lFile, mPointAttributes.bytesPerPoint() + 3 * sizeof(float), lMin, lMax);
	PointCloud::updateSpatialBounds(lFile, lMin, lMax);
	PointCloud::updateSize(lFile, lTotal);
	fclose(lFile);

	return lTotal;
}

void KdFileTree::remove()
{
	mRoot->deleteFiles();
	delete mRoot;
	mRoot = new KdFileTreeNode("n");
	std::remove("index.json");
}

json_spirit::mArray KdFileTree::fill(float iSigma, float iResolution) 
{
	json_spirit::mArray lLOD;
	iResolution *= iSigma;

	// downsample leaf nodes into file "doc...."
	std::string lName = "res-" + std::to_string(iResolution);
	FILE* lFile = PointCloud::writeHeader(lName, mPointAttributes, 0, mRoot->min, mRoot->max, iResolution);
	Downsampler lSampler(lFile, iResolution);
	process(lSampler, LEAVES); 
	PointCloud::updateSize(lFile, lSampler.mWritten);
	fclose(lFile);
	BOOST_LOG_TRIVIAL(info) << "Resolution = " << iResolution << " points " << lSampler.mWritten;

	uint64_t lPointCount;
	while(!mRoot->prune()) // discard leaf layer
	{
		FILE* lFile = PointCloud::readHeader(lName, NULL, lPointCount);
		mRoot->openFiles(mPointAttributes, iResolution);

		PointBuffer lPointBuffer(lFile, lPointCount, mPointAttributes.bytesPerPoint() + 3 * sizeof(float), availableMemory());
		lPointBuffer.begin();
		while (!lPointBuffer.end())
		{
			PointBuffer::Chunk& lChunk = lPointBuffer.next();

			boost::thread_group* lGroup = new boost::thread_group();
			mRoot->write(lChunk.mData, lPointBuffer.mStride, lChunk.mSize, iResolution, lGroup, std::thread::hardware_concurrency() );
			lGroup->join_all();
			delete lGroup;
		}
		fclose(lFile);

		mRoot->closeFiles();

		iResolution *= iSigma;

	    // downsample leaf nodes into file "doc...."
		lName = "res-" + std::to_string(iResolution);
		lFile = PointCloud::writeHeader(lName, mPointAttributes, 0, mRoot->min, mRoot->max, iResolution);
		Downsampler lSampler(lFile, iResolution);
		process(lSampler, LEAVES); 
		PointCloud::updateSize(lFile, lSampler.mWritten);
		fclose(lFile);
		BOOST_LOG_TRIVIAL(info) << "Resolution = " << iResolution << " points " << lSampler.mWritten;
	}

	// reload tree
	delete mRoot;
	std::ifstream lStream("index.json");
	json_spirit::mValue lJson;
	json_spirit::read_stream(lStream, lJson);
	lStream.close();
	mRoot = new KdFileTreeNode(lJson.get_obj());

	return lLOD;
};


//
// inorder traverasl
//


void KdFileTree::getNodes(std::vector<KdFileTreeNode*>& iList, KdFileTreeNode& iNode, uint8_t iNodes)
{
	if (iNode.mChildLow && iNode.mChildHigh)
	{
		if (iNodes & INTERNAL)
		{
			iList.push_back(&iNode);
		}

		getNodes(iList, *iNode.mChildLow, iNodes);
		getNodes(iList, *iNode.mChildHigh, iNodes);
	}
	else
	{
		if (iNodes & LEAVES)
		{
			iList.push_back(&iNode);
		}
	}
}

void KdFileTree::processNode(KdFileTree::InorderOperation& iProcessor, KdFileTreeNode& iNode)
{
	PointCloud lCloud;
	lCloud.readFile(iNode.mPath);
	lCloud.addAttributes(mPointAttributes);

	iProcessor.processNode(iNode, lCloud);
}


void KdFileTree::process(KdFileTree::InorderOperation& iProcessor, uint8_t iNodes)
{
	std::vector<KdFileTreeNode*> lVector;
	getNodes(lVector, *mRoot, iNodes);

	iProcessor.initTraveral(mPointAttributes);

	boost::thread_group* lGroup = new boost::thread_group();
	for (std::vector<KdFileTreeNode*>::iterator lIter = lVector.begin(); lIter != lVector.end(); lIter++)
	{
		if (lGroup->size() == std::thread::hardware_concurrency())
		{
			lGroup->join_all();
			delete lGroup;
			lGroup = new boost::thread_group();
		}
		lGroup->add_thread(new boost::thread(&KdFileTree::processNode, this, boost::ref(iProcessor), boost::ref(*(*lIter))));
	}

	if (lGroup->size())
	{
		lGroup->join_all();
	}
	delete lGroup;

	iProcessor.completeTraveral(mPointAttributes);
}



KdFileTree::InorderOperation::InorderOperation(std::string iName)
: mName(iName)
{
};

void KdFileTree::InorderOperation::initTraveral(PointCloudAttributes& iAttributes) 
{
	mStartTime = boost::posix_time::second_clock::local_time();
	BOOST_LOG_TRIVIAL(info) << "Starting : " << mName;
};

void KdFileTree::InorderOperation::completeTraveral(PointCloudAttributes& iAttributes)
{
	boost::posix_time::time_duration diff = boost::posix_time::second_clock::local_time() - mStartTime;
	BOOST_LOG_TRIVIAL(info) << "Finished : " << mName << ":" << diff.total_milliseconds() / 1000 << " seconds";
};

//
// Fill internal nodes
//

Downsampler::Downsampler(FILE* iFile, float iResolution)
	: InorderOperation("Lod")
	, mFile(iFile)
	, mWritten(0)
	, mResolution(iResolution)
{
}


void Downsampler::processNode(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	VoxelHashIndex2 lVoxelHash(iCloud.mPoints, mResolution);

	uint32_t lCount = 0;
	for (uint32_t i = 0; i < iCloud.size(); i++)
	{
		lVoxelHash.project(iCloud[i]->position, i);
	}

	int lIntensityIndex = iCloud.getAttributeIndex(Attribute::INTENSITY);
	int lColorIndex = iCloud.getAttributeIndex(Attribute::COLOR);
	int lClassIndex = iCloud.getAttributeIndex(Attribute::CLASS);

	mWriteLock.lock();

	std::vector<VoxelHashIndex2::Voxel*>& lGrid = lVoxelHash.getGrid();
	for (std::vector<VoxelHashIndex2::Voxel*>::iterator lIter = lGrid.begin(); lIter != lGrid.end(); lIter++)
	{
		VoxelHashIndex2::Voxel* lVoxel = *lIter;
		if (lVoxel)
		{
			Point lPoint(iCloud);
			memset(lPoint.position, 0, sizeof(lPoint.position));

			// create class histogram
			uint32_t lClassH[20];
			if (lClassIndex != -1)
			{
				memset(lClassH, 0, sizeof(lClassH));
			}

			double lWeight = 1.0 / lVoxel->list.size();
			for (std::vector<uint32_t>::iterator lIter1 = lVoxel->list.begin(); lIter1 != lVoxel->list.end(); lIter1++)
			{
				Point& lSample = *iCloud[*lIter1];

				if (lIntensityIndex != -1)
				{
					IntensityType& lIntensity = *(IntensityType*)lPoint.getAttribute(lIntensityIndex);
					lIntensity.max(*lSample.getAttribute(lIntensityIndex));
				}

				if (lColorIndex != -1)
				{
					ColorType& lColor = *(ColorType*)lPoint.getAttribute(lColorIndex);
					lColor.add(*lSample.getAttribute(lColorIndex), lWeight);
				}

				if (lClassIndex != -1)
				{
					ClassType& lClass = *(ClassType*)lSample.getAttribute(lClassIndex);
					if (lClass.mValue < 20)
					{
						lClassH[lClass.mValue]++;
					}
				}

				lPoint.position[0] += lSample.position[0] * lWeight;
				lPoint.position[1] += lSample.position[1] * lWeight;
				lPoint.position[2] += lSample.position[2] * lWeight;
			}

			if (lClassIndex != -1)
			{
				// find max in class histogram
				int lMax = 0;
				for (int i = 0; i < 20; i++)
				{
					if (lClassH[i] > lClassH[lMax])
					{
						lMax = i;
					}
				}
				ClassType& lClass = *(ClassType*)lPoint.getAttribute(lClassIndex);
				lClass.mValue = lMax;
			}

			lPoint.write(mFile);
			mWritten++;
		}
	}

	mWriteLock.unlock();

}
