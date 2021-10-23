#pragma once

#include "../kdFileTree.h"


class VoxelFilter : public KdFileTree::InorderOperation
{
	public:

		VoxelFilter(float iResolution);

	protected:

		float mResolution;

		void processNode(KdFileTreeNode& iNode, PointCloud& iCloud);
};
