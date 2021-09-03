#pragma once

#include <vector>
#include <queue>
#include <stack>

#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>


#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include "point.h"
#include "pointCloud.h"
#include "kdTree.h"

class PointBuffer
{	
	
	public:

		PointBuffer(FILE* iFile, uint64_t iCount, uint32_t iStride, uint64_t iMemory);
		~PointBuffer();

		typedef struct
		{
			uint8_t* mData;
			uint32_t mSize;
		} Chunk;

		void begin();
		bool end();
		Chunk& next();

		uint32_t mStride;

	private:

		uint64_t POINTS_PER_IO;

		int32_t mIter;
		int32_t mCurrent;
		Chunk mChunk;

		uint32_t mCount;
		FILE* mFile;
		uint64_t mDatastart;
}; 
  