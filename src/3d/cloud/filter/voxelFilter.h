#pragma once

#include "../kdFileTree.h"


class VoxelFilter : public KdFileTree::LeafOperation
{
	public:

		VoxelFilter(float iResolution);

	protected:

		float mResolution;

		void processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud);
};
