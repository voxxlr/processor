#ifndef _VoxelHash
#define _VoxelHash

#include <vector>
#include <queue>

#include <glm/glm.hpp>

template <class DATA> struct HashGrid
{
	HashGrid(int32_t ix, int32_t iy, int32_t iz)
	: gx(ix)
	, gy(iy)
	, gz(iz)
	{
	}

	DATA mData;

	int32_t gx;
	int32_t gy;
	int32_t gz;
};

template <class DATA> class VoxelHash
{
	public:

		VoxelHash(uint32_t iSize, double iResolution)
		: mSize(iSize)
		, mResolution(iResolution)
		{
			mVector.resize(iSize, 0);
		};

		~VoxelHash()
		{
			for (typename std::vector<HashGrid<DATA>*>::iterator lIter = mVector.begin(); lIter != mVector.end(); lIter++)  // JSTIER Performance !!!!
			{
				if (*lIter)
				{
					delete *lIter;
				}
			}
		}

		DATA& getValue(glm::dvec3& iVertex, bool &iNew)
		{
			iNew = false;

			int32_t gx = floor(iVertex.x/mResolution);
			int32_t gy = floor(iVertex.y/mResolution);
			int32_t gz = floor(iVertex.z/mResolution);

			//iVertex.x = gx*mResolution;
			//iVertex.y = gy*mResolution;
			//iVertex.z = gz*mResolution;

			uint32_t lIndex =  ((gx*73856093)^(gy*19349663)^(gz*83492791))%mSize;
			while (1)
			{
				HashGrid<DATA>* lHashGrid = mVector[lIndex];
				if (lHashGrid)
				{
					if (lHashGrid->gx == gx && lHashGrid->gy == gy && lHashGrid->gz == gz)
					{
						return lHashGrid->mData;
					}
				}
				else
				{
					lHashGrid = new HashGrid<DATA>(gx, gy, gz);
					mVector[lIndex] = lHashGrid;
					iNew = true;
					return lHashGrid->mData;
				}
				lIndex = (lIndex+1)%mSize;
			}
		};
		
		std::vector<HashGrid<DATA>*>& getGrid()
		{
			return mVector;
		}

	private:
			
		std::vector<HashGrid<DATA>*> mVector;

		uint32_t mSize;
		double mResolution;
}; 
  

#endif
