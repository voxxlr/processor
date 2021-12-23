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

		PointBuffer(FILE* iFile, size_t iCount, uint32_t iStride, size_t iMemory);
		~PointBuffer();

		typedef struct
		{
			uint8_t* mData;
			size_t mSize;
		} Chunk;

		void begin();
		bool end();
		Chunk& next();

		uint32_t mStride;

	private:

		size_t POINTS_PER_IO;

		size_t mIter;
		size_t mCurrent;
		Chunk mChunk;

		size_t mCount;
		FILE* mFile;
		long mDatastart;
}; 
  