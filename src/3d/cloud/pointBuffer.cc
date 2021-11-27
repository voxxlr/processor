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

#include "pointBuffer.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

PointBuffer::PointBuffer(FILE* iFile, size_t iCount, uint32_t iStride, size_t iMemory)
: mStride(iStride)
, mFile(iFile)
, mDatastart(ftell(iFile))
, mCount(iCount)
, mCurrent(-1)
, POINTS_PER_IO(std::min(uint64_t(0.9*iMemory)/iStride, iCount))
{
	POINTS_PER_IO = std::min((size_t)200000000, POINTS_PER_IO);
	//BOOST_LOG_TRIVIAL(info) << "POINTS_PER_IO =" << POINTS_PER_IO;
	mChunk.mData = new uint8_t[POINTS_PER_IO*iStride];
}

PointBuffer::~PointBuffer()
{
	delete [] mChunk.mData;
}

void PointBuffer::begin()
{
	fseek(mFile, mDatastart, SEEK_SET);
	mIter = 0;
}

bool PointBuffer::end()
{
	return mIter*POINTS_PER_IO >= mCount;
}

PointBuffer::Chunk& PointBuffer::next()
{
	if (mCurrent != mIter) 
	{
		mChunk.mSize = fread(mChunk.mData, mStride, POINTS_PER_IO, mFile);
		BOOST_LOG_TRIVIAL(info) << "fread " << mChunk.mSize;
		mCurrent = mIter;
	}

	mIter++;
	return mChunk;
}
