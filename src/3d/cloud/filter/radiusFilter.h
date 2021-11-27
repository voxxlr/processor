#pragma once

#include "../kdFileTree.h"

class RadiusFilter : public KdFileTree::InorderOperation
{
	public:

		static float R;
		static float R3;

		RadiusFilter(float iResolution, float iDensity);

	protected:

		float mRadius;
		int mMinPoints;

		void processNode(KdFileTreeNode& iNode, PointCloud& iCloud);
};
