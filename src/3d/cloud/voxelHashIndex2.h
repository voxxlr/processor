#ifndef _VoxelHash
#define _VoxelHash

#include <vector>
#include <queue>

#include <glm/glm.hpp>

#include "point.h"

class VoxelHashIndex2
{
	public:

		static const int SCALAR = 5;

		typedef struct
		{
			std::vector<uint32_t> list;
			int32_t gx;
			int32_t gy;
			int32_t gz;

		} Voxel;

		VoxelHashIndex2(std::vector<Point*>& iCloud, float iResolution)
		: mSize(iCloud.size()* SCALAR)
		, mResolution(iResolution)
		, mCloud(iCloud)
		{
			mIndex.resize(mSize, 0);
		};

		~VoxelHashIndex2()
		{
			for (std::vector<Voxel*>::iterator lIter = mIndex.begin(); lIter != mIndex.end(); lIter++)
			{
				if (*lIter)
				{
					delete *lIter;
				}
			}
		};

		void project(float* iPosition, int32_t iIndex)
		{
			int32_t gx = (int32_t)floor(iPosition[0]/mResolution);
			int32_t gy = (int32_t)floor(iPosition[1]/mResolution);
			int32_t gz = (int32_t)floor(iPosition[2]/mResolution);
			
			uint32_t lIndex =  ((gx*73856093)^(gy*19349663)^(gz*83492791))%mSize;
			while (1)
			{
				Voxel* lVoxel = mIndex[lIndex];
				if (lVoxel)
				{
					if (lVoxel->gx == gx && lVoxel->gy == gy && lVoxel->gz == gz)
					{
						lVoxel->list.push_back(iIndex);
						break;
					}
				}
				else
				{
					lVoxel = new Voxel();
					lVoxel->gx = gx;
					lVoxel->gy = gy;
					lVoxel->gz = gz;
					lVoxel->list.push_back(iIndex);
					mIndex[lIndex] = lVoxel;
					break;
				}
				lIndex = (lIndex+1)%mSize;
			}
		};

		std::vector<Voxel*>& getGrid()
		{
			return mIndex;
		}
		
		static uint64_t maxMemoryUsage(uint64_t iPoints)
		{
			return SCALAR*iPoints*sizeof(Voxel);
		}

	private:

		std::vector<Voxel*> mIndex;
		std::vector<Point*>& mCloud;

		uint32_t mSize;
		float mResolution;
}; 
  

#endif
