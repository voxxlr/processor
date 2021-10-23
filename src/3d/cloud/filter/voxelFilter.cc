#include "../kdFileTree.h"
#include "../voxelHashIndex2.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "voxelFilter.h"

VoxelFilter::VoxelFilter(float iResolution)
: InorderOperation("Voxelfilter")
, mResolution(iResolution)
{
}

void VoxelFilter::processNode(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	VoxelHashIndex2 lVoxelHash(iCloud.mPoints, mResolution);

	for (uint32_t i = 0; i < iCloud.size(); i++)
	{
		lVoxelHash.project(iCloud[i]->position, i);
	}

	int lIntensityIndex = iCloud.getAttributeIndex(Attribute::INTENSITY);
	int lColorIndex = iCloud.getAttributeIndex(Attribute::COLOR);
	int lClassIndex = iCloud.getAttributeIndex(Attribute::CLASS);


	FILE* lFile = PointCloud::writeHeader(iNode.mPath, iCloud, 0, iCloud.mMinExtent, iCloud.mMaxExtent, mResolution);
	uint32_t lCount = 0;

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

			lPoint.write(lFile);
			lCount++;
		}
	}

	iCloud.updateSize(lFile, lCount);
	fclose(lFile);
	BOOST_LOG_TRIVIAL(info) << "Filtered " << iNode.mPath << " to " << (lCount*100.0)/iCloud.size() << "%  ";
};

