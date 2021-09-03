#pragma once

#include "../kdFileTree.h"

class Analyzer : public KdFileTree::LeafOperation
{
	public:

		Analyzer();
		
		double mResolution;
		double mVariance;

	protected:

		void processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud);

		void completeTraveral(PointCloudAttributes& iAttributes);

		boost::mutex mVectorLock;
		std::vector<std::tuple<double, double, uint32_t>> mNodes;  // average, stddev, count

};
