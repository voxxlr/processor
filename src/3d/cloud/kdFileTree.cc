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

KdFileTreeNode::KdFileTreeNode(json_spirit::mObject& iConfig)
: mChildLow(0)
, mChildHigh(0)
, mCount(0)
, mFile(0)
, mState(3)
, mThreaded(false)
, mPath(iConfig["path"].get_str())
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
		mState[LEAF] = false;
		mSplit = iConfig["split"].get_real();
		mAxis = iConfig["axis"].get_int();
	}
	else
	{
		mState[LEAF] = true;
		mHeight = 0;
	}
}


KdFileTreeNode::KdFileTreeNode(std::string iPath, uint32_t iHeight)
: mAxis(0)
, mChildLow(0)
, mChildHigh(0)
, mCount(0)
, mFile(0)
, mState(3)
, mThreaded(false)
, mHeight(iHeight)
, mPath(iPath)
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

	mState[LEAF] = true;
	mState[ALIVE] = true;
	mState[DEGENERATE] = false;
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

	std::remove(std::string(mPath + ".ply").c_str());
}

bool KdFileTreeNode::grow(std::string iIndent, uint32_t iLeafsize)
{
	BOOST_LOG_TRIVIAL(info) << iIndent << mPath;

	mState[ALIVE] = false;
	if (!mState[LEAF])
	{
		if (mChildLow->mState[ALIVE])
		{
			if (mChildLow->grow(iIndent+"  ", iLeafsize))
			{
				mState[ALIVE] = true;
			}
		}

		if (mChildHigh->mState[ALIVE])
		{
			if (mChildHigh->grow(iIndent+"  ", iLeafsize))
			{
				mState[ALIVE] = true;
			}
		}

		if (mChildHigh->mState[DEGENERATE])
		{
			mChildLow->mState[DEGENERATE] = true;
			mChildLow->mState[ALIVE] = false;
		}

		if (mChildLow->mState[DEGENERATE])
		{
			mChildHigh->mState[DEGENERATE] = true;
			mChildHigh->mState[ALIVE] = false;
		}
	}
	else
	{
		if (mCount > iLeafsize)
		{
			float lVolume = (max[2] - min[2])*(max[1] - min[1])*(max[0] - min[0]);
			// split along axis where median is farthest from bbox
			float lMaxD = 0;
			for (int i=0; i<3; i++)
			{
				float dToBox = std::min(max[i] - median[i], median[i] - min[i]);
				if (dToBox > lMaxD)
				{
					lMaxD = dToBox;
					mAxis = i;
				}
			}
	
			//if (lMaxD > iVoxelSize) 
			//{
				mSplit = median[mAxis];

				mChildLow = new KdFileTreeNode(mPath+"0", mHeight+1); 
				memcpy(mChildLow->min, min, sizeof(min));
				memcpy(mChildLow->max, max, sizeof(max));
				mChildLow->max[mAxis] = mSplit;

				mChildHigh = new KdFileTreeNode(mPath+"1", mHeight+1);
				memcpy(mChildHigh->min, min, sizeof(min));
				memcpy(mChildHigh->max, max, sizeof(max));
				mChildHigh->min[mAxis] = mSplit;

				mState[LEAF] = false;
				mState[ALIVE] = true;
				BOOST_LOG_TRIVIAL(info) << iIndent << "Split -----  " << mSplit;
			//}	
			//else
			//{
			//	BOOST_LOG_TRIVIAL(info) << iIndent <<  "degenerate data detected (lMaxD) " << lMaxD;
			//	mState[DEGENERATE] = true;
			//}
		}
	}
	return mState[ALIVE];
}

void KdFileTreeNode::feed(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, boost::thread_group*& iGroup, int32_t iThreadCount)
{
	if (!mState[LEAF])
	{
		if (mChildLow->mState[ALIVE])
		{
			mChildLow->feed(iBuffer, iStride, iCount, iGroup, iThreadCount);
		}

		if (mChildHigh->mState[ALIVE])
		{
			mChildHigh->feed(iBuffer, iStride, iCount, iGroup, iThreadCount);
		}
	}
	else
	{
		if (mState[ALIVE])
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

void KdFileTreeNode::recordChunk(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount) 
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
	if (!mState[LEAF])
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

void KdFileTreeNode::writeChunk(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, float iOverlap)
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
	for (int i = 0; i < iCount; i++)
	{
		lPointer = iBuffer + i * iStride;
		if (contains((float*)lPointer))
		{
			fwrite(lPointer, 1, iStride, mFile);
			mCount++;
		}
	}
}

void KdFileTreeNode::write(uint8_t* iBuffer, uint32_t iStride, uint32_t iCount, float iOverlap, boost::thread_group*& iGroup, int32_t iThreadCount)
{
	if (!mState[LEAF])
	{
		mChildLow->write(iBuffer, iStride, iCount, iOverlap, iGroup, iThreadCount);
		mChildHigh->write(iBuffer, iStride, iCount, iOverlap, iGroup, iThreadCount);
	}
	else
	{
		if (!mState[DEGENERATE])
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

	if (!mState[LEAF])
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

bool KdFileTreeNode::prune()
{
	bool lLeaf = mState[LEAF];

	if (!mState[LEAF])
	{
		bool lLo = mChildLow->prune();
		bool lHi = mChildHigh->prune();
		mState[LEAF] = lLo && lHi;
		/*
		bool lLo = mChildLow->pruned();
		bool lHi = mChildHigh->pruned();

		if (lLo && lHi)
		{
			delete mChildLow;
			mChildLow = 0;
			delete mChildHigh;
			mChildHigh = 0;

			mState[LEAF] = true;
		}
		return false;
		*/
	}

	return lLeaf;
}

uint64_t KdFileTreeNode::collapse(FILE* iFile, uint32_t iStride, float* iMin, float* iMax)
{
	if (!mState[LEAF])
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
		while (!lPointBuffer.end())
		{
			PointBuffer::Chunk& lChunk = lPointBuffer.next();

			uint8_t* lPointer = lChunk.mData;
			for (int i = 0; i < lChunk.mSize; i++)
			{
				float* lPosition = (float*)(lChunk.mData + i * iStride);

				iMin[0] = std::min<float>(lPosition[0], iMin[0]);
				iMin[1] = std::min<float>(lPosition[1], iMin[1]);
				iMin[2] = std::min<float>(lPosition[2], iMin[2]);
				iMax[0] = std::max<float>(lPosition[0], iMax[0]);
				iMax[1] = std::max<float>(lPosition[1], iMax[1]);
				iMax[2] = std::max<float>(lPosition[2], iMax[2]);
			}

			fwrite(lChunk.mData, iStride, lCount, iFile);
		}

		fclose(lFile);
		return lCount;
	}
}


//const float KdFileTree::SIGMA = 1.479;
const float KdFileTree::SIGMA = 1.25992104989f; // cube root of 3

KdFileTree::KdFileTree(uint64_t iMemory, uint32_t iCPUs)
	: mMemory(iMemory)
	, mCPUs(iCPUs)
	, mRoot(new KdFileTreeNode("n"))
	, mResolution(0)
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
	FILE* lFile = PointCloud::readHeader(iName, &mPointAttributes, lPointCount, mRoot->min, mRoot->max, &mResolution);
	
	// pass one - build tree
	PointBuffer lPointBuffer(lFile, lPointCount, mPointAttributes.bytesPerPoint() + 3 * sizeof(float), mMemory);
	do
	{
		lPointBuffer.begin();
		while (!lPointBuffer.end())
		{
			PointBuffer::Chunk& lChunk = lPointBuffer.next();

			boost::thread_group* lGroup = new boost::thread_group();
			mRoot->feed(lChunk.mData, lPointBuffer.mStride, lChunk.mSize, lGroup, 2 * mCPUs);
			lGroup->join_all();
			delete lGroup;
		}

	} while (mRoot->grow("", iLeafsize));

	BOOST_LOG_TRIVIAL(info) << "Opening Files";
	mRoot->openFiles(mPointAttributes, mResolution);

	// pass two - write points into leaves
	BOOST_LOG_TRIVIAL(info) << "Writing file tree ";
	lPointBuffer.begin();
	while (!lPointBuffer.end())
	{
		PointBuffer::Chunk& lChunk = lPointBuffer.next();

		boost::thread_group* lGroup = new boost::thread_group();
		mRoot->write(lChunk.mData, lPointBuffer.mStride, lChunk.mSize, iOverlap, lGroup, 2 * mCPUs);
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

uint64_t KdFileTree::collapse(std::string iName)
{
	boost::this_thread::sleep_for(boost::chrono::milliseconds(1000)); // give os time to GC ... ???

	float lMin[3];
	float lMax[3];
	memcpy(lMin, PointCloud::MIN, sizeof(lMin));
	memcpy(lMax, PointCloud::MAX, sizeof(lMax));

	FILE* lFile = PointCloud::writeHeader(iName, mPointAttributes, 0, 0, 0, mResolution);
	uint64_t lTotal = mRoot->collapse(lFile, mPointAttributes.bytesPerPoint() + 3 * sizeof(float), lMin, lMax);
	PointCloud::updateSpatialBounds(lFile, lMin, lMax);
	PointCloud::updateSize(lFile, lTotal);
	fclose(lFile);

	return lTotal;
}

void KdFileTree::remove()
{
	delete mRoot;
	mRoot = new KdFileTreeNode("n");
	std::remove("index.json");
}

json_spirit::mArray KdFileTree::fill(float iSigma) 
{
	json_spirit::mArray lLOD;
	int lDocId = 1;
	mResolution *= iSigma;

	// downsample leaf nodes into file "doc...."
	std::string lName = "doc-" + std::to_string((boost::int64_t)lDocId++);
	FILE* lFile = PointCloud::writeHeader(lName, mPointAttributes, 0, mRoot->min, mRoot->max, mResolution);

	Downsampler lSampler(lFile, mResolution);
	process(lSampler, LEAVES); 
	PointCloud::updateSize(lFile, lSampler.mWritten);
	fclose(lFile);

	uint64_t lPointCount;
	while(!mRoot->prune()) // discard leaf layer
	{
		FILE* lFile = PointCloud::readHeader(lName, NULL, lPointCount);
		mRoot->openFiles(mPointAttributes, mResolution);

		PointBuffer lPointBuffer(lFile, lPointCount, mPointAttributes.bytesPerPoint() + 3 * sizeof(float), mMemory);
		lPointBuffer.begin();
		while (!lPointBuffer.end())
		{
			PointBuffer::Chunk& lChunk = lPointBuffer.next();

			boost::thread_group* lGroup = new boost::thread_group();
			mRoot->write(lChunk.mData, lPointBuffer.mStride, lChunk.mSize, mResolution, lGroup, 2*mCPUs);
			lGroup->join_all();
			delete lGroup;
		}
		fclose(lFile);

		mRoot->closeFiles();

		mResolution *= iSigma;

	    // downsample leaf nodes into file "doc...."
		lName = "doc-"+std::to_string((boost::int64_t)lDocId++);
		lFile = PointCloud::writeHeader(lName, mPointAttributes, 0, mRoot->min, mRoot->max, mResolution);
		Downsampler lSampler(lFile, mResolution);
		process(lSampler, LEAVES); 
		PointCloud::updateSize(lFile, lSampler.mWritten);
		fclose(lFile);
	}

	return lLOD;
};


//
// Processor
//

struct HighestNode
{
	bool operator()(const KdFileTreeNode* iL, const KdFileTreeNode* iR) const
	{
		return iL->mHeight > iR->mHeight;
	}
};

//
// inorder traverasl
//
void KdFileTree::process(KdFileTree::InorderOperation& iProcessor, uint8_t iNodes)
{
	iProcessor.initTraveral(mPointAttributes);

	uint32_t lThreadCount = mCPUs-1;  // select n-1 internal nodes --- 2 threads each will result in mCPUs working threads and mCPU-1 internal threads

	// add internal node threads
	std::priority_queue<KdFileTreeNode*, std::vector<KdFileTreeNode*>, HighestNode> lQueue;
	lQueue.push(mRoot);
	while (!lQueue.empty())
	{
		KdFileTreeNode* lNode = lQueue.top();
		lQueue.pop();
	
		if (lThreadCount-- <= 0)
		{
			break;
		}
		lNode->mThreaded = true;
		
		if (lNode->mChildLow)
		{
			lQueue.push(lNode->mChildLow);
		}

		if (lNode->mChildHigh)
		{
			lQueue.push(lNode->mChildHigh);
		}
	}

	inorderVisit(iProcessor, *mRoot, iNodes, mRoot->mThreaded);

	iProcessor.completeTraveral(mPointAttributes);
}

void KdFileTree::inorderVisit(KdFileTree::InorderOperation& iProcessor, KdFileTreeNode& iNode, uint8_t iNodes, bool iThread)
{
	if (iThread)
	{
		std::string lPath = iNode.mPath;
		size_t lLength = lPath.length();
		if (lLength < 10)
		{
			lPath.insert(0, 10 - lLength, ' ');
		}
		boost::log::core::get()->add_thread_attribute("ThreadName", boost::log::attributes::constant< std::string >(lPath));
	}

	if (iNode.mChildLow && iNode.mChildHigh)
	{
		if (iNode.mThreaded)
		{
			boost::this_thread::sleep_for(boost::chrono::milliseconds(100)); // make sure the threads are balanced
			boost::thread lT1(&KdFileTree::inorderVisit, this, boost::ref(iProcessor), boost::ref(*iNode.mChildLow), iNodes, true);
			boost::thread lT2(&KdFileTree::inorderVisit, this, boost::ref(iProcessor), boost::ref(*iNode.mChildHigh), iNodes, true);
			lT1.join();
			lT2.join();
			iNode.mThreaded = false;
		}
		else
		{
			inorderVisit(iProcessor, *iNode.mChildLow, iNodes, false);
			inorderVisit(iProcessor, *iNode.mChildHigh, iNodes, false); 
		}

		try
		{
			if (iNodes & INTERNAL)
			{
				PointCloud lCloud(mPointAttributes);
				lCloud.readFile(iNode.mPath);

				iProcessor.processNode(iNode, lCloud);
				iProcessor.processInternal(iNode, lCloud);
			}
		}
		catch (const std::bad_alloc& e) 
		{
			BOOST_LOG_TRIVIAL(info) << "Allocation failed: " << e.what();
			exit(-1);
		}
		catch (...)
		{
			BOOST_LOG_TRIVIAL(info) << "Caught Exception in processInternal";
		}
	}
	else
	{
		try
		{
			if (iNodes & LEAVES)
			{
				PointCloud lCloud(mPointAttributes);
				lCloud.readFile(iNode.mPath);

				iProcessor.processNode(iNode, lCloud);
				iProcessor.processLeaf(iNode, lCloud);
			}
		}
		catch (const std::bad_alloc& e) 
		{
			BOOST_LOG_TRIVIAL(info) << "Allocation failed: " << e.what() << "   "  << iNode.mPath;
			exit(-1);
		}
		catch (...)
		{
			BOOST_LOG_TRIVIAL(info) << "Undefined Exception"  << iNode.mPath;
			exit(-1);
		}
	}
}


//
// preorder traverasl
//

void KdFileTree::process(KdFileTree::PreorderOperation& iProcessor)
{
	iProcessor.initTraveral(mPointAttributes);

	std::vector<std::vector<KdFileTreeNode*>> lLevels;

	// separate into levels
	std::queue<KdFileTreeNode*> lQueue;
	lQueue.push(mRoot);
	while (!lQueue.empty())
	{
		KdFileTreeNode* lNode = lQueue.front();
		lQueue.pop();

		if (lNode->mHeight == lLevels.size())
		{
			lLevels.push_back(std::vector<KdFileTreeNode*>());
		}

		lLevels[lNode->mHeight].push_back(lNode);

		if (lNode->mChildLow)
		{
			lQueue.push(lNode->mChildLow);
		}

		if (lNode->mChildHigh)
		{
			lQueue.push(lNode->mChildHigh);
		}
	}

	// process levels
	for (std::vector<std::vector<KdFileTreeNode*>>::iterator lLevel = lLevels.begin(); lLevel != lLevels.end(); lLevel++)
	{
		if (lLevel->size() <= mCPUs)
		{
			uint32_t lThreadCount = std::min<uint32_t>(lLevel->size(), mCPUs);

			boost::barrier lBarrier(lThreadCount);

			boost::thread_group* lGroup = new boost::thread_group();
			for (std::vector<KdFileTreeNode*>::iterator lNode = lLevel->begin(); lNode != lLevel->end(); lNode++)
			{
				lGroup->add_thread(new boost::thread(&KdFileTree::preorderVisit, this, boost::ref(iProcessor), boost::ref(*(*lNode)), boost::ref(lBarrier), lThreadCount));
			}
			lGroup->join_all();
		}
	}

	iProcessor.completeTraveral(mPointAttributes);
}

void KdFileTree::preorderVisit(KdFileTree::PreorderOperation& iProcessor, KdFileTreeNode& iNode, boost::barrier& iBarrier, uint8_t iThreadCount)
{
	std::string lPath = iNode.mPath;
	size_t lLength = lPath.length();
	if (lLength < 10)
	{
		lPath.insert(0, 10 - lLength, ' ');
	}
	boost::log::core::get()->add_thread_attribute("ThreadName", boost::log::attributes::constant< std::string >(lPath));

	try
	{
		PointCloud lCloud(mPointAttributes);
		lCloud.readFile(iNode.mPath);
		iProcessor.processNode(iNode, lCloud, iBarrier, iThreadCount);
	}
	catch (const std::bad_alloc& e) 
	{
		BOOST_LOG_TRIVIAL(info) << "Allocation failed: " << e.what() << "   "  << iNode.mPath;
		exit(-1);
	}
	catch (...)
	{
		BOOST_LOG_TRIVIAL(info) << "Undefined Exception"  << iNode.mPath;
		exit(-1);
	}

}


//
// leaf traverasl
//

void KdFileTree::process(KdFileTree::LeafOperation& iProcessor)
{
	iProcessor.initTraveral(mPointAttributes);

	std::vector<KdFileTreeNode*> lLeafs;

	// separate into levels
	std::queue<KdFileTreeNode*> lQueue;
	lQueue.push(mRoot);
	while (!lQueue.empty())
	{
		KdFileTreeNode* lNode = lQueue.front();
		lQueue.pop();

		if (lNode->mChildLow)
		{
			lQueue.push(lNode->mChildLow);
		}

		if (lNode->mChildHigh)
		{
			lQueue.push(lNode->mChildHigh);
		}

		if (!(lNode->mChildHigh && lNode->mChildHigh))
		{
			lLeafs.push_back(lNode);
		}
	}

	// process levels
	try
	{
		uint32_t lThreadCount = mCPUs;
		boost::thread_group* lGroup = new boost::thread_group();
		for (std::vector<KdFileTreeNode*>::iterator lLeaf = lLeafs.begin(); lLeaf != lLeafs.end(); lLeaf++)
		{
			lGroup->add_thread(new boost::thread(&KdFileTree::leafVisit, this, boost::ref(iProcessor), boost::ref(*(*lLeaf))));
			if (lThreadCount-- == 0)
			{
				lGroup->join_all();
				delete lGroup;

				lThreadCount = mCPUs;
				lGroup = new boost::thread_group();
			}
		}
		lGroup->join_all();

		iProcessor.completeTraveral(mPointAttributes);
	}
	catch (...)
	{
		BOOST_LOG_TRIVIAL(info) << "Undefined Exception in KdFileTree::process(KdFileTree::LeafOperation& iProcessor)";
		exit(-1);
	}

}

void KdFileTree::leafVisit(KdFileTree::LeafOperation& iProcessor, KdFileTreeNode& iNode)
{
	std::string lPath = iNode.mPath;
	size_t lLength = lPath.length();
	if (lLength < 10)
	{
		lPath.insert(0, 10 - lLength, ' ');
	}
	boost::log::core::get()->add_thread_attribute("ThreadName", boost::log::attributes::constant< std::string >(lPath));

	try
	{
		PointCloud lCloud(mPointAttributes);
		lCloud.readFile(iNode.mPath);

		iProcessor.processLeaf(iNode, lCloud);
	}
	catch (const std::bad_alloc& e)
	{
		BOOST_LOG_TRIVIAL(info) << "Allocation failed: " << e.what() << "   " << iNode.mPath;
		exit(-1);
	}
	catch (...)
	{
		BOOST_LOG_TRIVIAL(info) << "Undefined Exception" << iNode.mPath;
		exit(-1);
	}
}


//
//
//
/*
uint32_t KdFileTree::bytesPerPoint()
{
	return mSourceCloud.bytesPerPoint();
}
*/

KdFileTree::InorderOperation::InorderOperation(std::string iName)
: mName(iName)
{
};

void KdFileTree::InorderOperation::initTraveral(PointCloudAttributes& iAttributes) 
{
	BOOST_LOG_TRIVIAL(info) << "Starting Inorder traversal " << mName;
};


KdFileTree::PreorderOperation::PreorderOperation(std::string iName)
: mName(iName)
{
};

void KdFileTree::PreorderOperation::initTraveral(PointCloudAttributes& iAttributes)
{
	BOOST_LOG_TRIVIAL(info) << "Starting Preorder traversal " << mName;
};


KdFileTree::LeafOperation::LeafOperation(std::string iName)
: mName(iName)
{
};

void KdFileTree::LeafOperation::initTraveral(PointCloudAttributes& iAttributes)
{
	BOOST_LOG_TRIVIAL(info) << "Starting Leaf operation " << mName;
};
/*
#ifdef TIMING
boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
#endif

#ifdef TIMING
boost::posix_time::ptime t2 = boost::posix_time::second_clock::local_time();
boost::posix_time::time_duration diff = t2 - t1;
//	BOOST_LOG_TRIVIAL(info) << "Recording " << iCount/(1000000.0) << " million points took " << diff.total_milliseconds()/1000 << " seconds";
#endif
*/


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


void Downsampler::processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud)
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

	/*
	VoxelHashIndex lVoxelHash(iCloud.mPoints, mResolution);

	// pass 1 count
	for (uint32_t i=0; i<iCloud.size(); i++)
	{
		bool lNew;
		Point& lPoint = lVoxelHash.getPoint(*iCloud[i], i, lNew);

		if (lNew)
		{
			lPoint.mWeight = 1;
		}
		else
		{
			lPoint.position[0] = (lPoint.position[0]*lPoint.mWeight + iCloud[i]->position[0])/(lPoint.mWeight+1);
			lPoint.position[1] = (lPoint.position[1]*lPoint.mWeight + iCloud[i]->position[1])/(lPoint.mWeight+1);
			lPoint.position[2] = (lPoint.position[2]*lPoint.mWeight + iCloud[i]->position[2])/(lPoint.mWeight+1);

			iCloud.average(lPoint,*iCloud[i]);
			lPoint.mWeight = lPoint.mWeight+1.0;
		}
	}

	mWriteLock.lock();
	std::vector<int32_t>& lGrid = lVoxelHash.getGrid();
	for (std::vector<int32_t>::iterator lIter = lGrid.begin(); lIter != lGrid.end(); lIter++)
	{
		if (*lIter >= 0)
		{
			Point& lPoint = *iCloud[*lIter];
			if (lPoint.inside(iNode.min, iNode.max))
			{
				iCloud[*lIter]->write(mFile);
				mWritten++;
			}
		}
	}
	mWriteLock.unlock();
	*/

}
