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

PointBuffer::PointBuffer(FILE* iFile, uint64_t iCount, uint32_t iStride, uint64_t iMemory)
: mStride(iStride)
, mFile(iFile)
, mDatastart(ftell(iFile))
, mCount(iCount)
, mCurrent(-1)
, POINTS_PER_IO(std::min(uint64_t(0.9*iMemory)/iStride, iCount))
{
	mChunk.mData = new uint8_t[POINTS_PER_IO*iStride];
}

PointBuffer::~PointBuffer()
{
	delete [] mChunk.mData;
}

void PointBuffer::begin()
{
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
		mChunk.mSize = (mCount - (mIter* POINTS_PER_IO))/ POINTS_PER_IO ? POINTS_PER_IO : mCount % POINTS_PER_IO;
		fseek(mFile, mDatastart + mIter * mStride * POINTS_PER_IO, SEEK_SET);
		fread(mChunk.mData, mStride, mChunk.mSize, mFile);
		mCurrent = mIter;
	}

	mIter++;
	return mChunk;
}
