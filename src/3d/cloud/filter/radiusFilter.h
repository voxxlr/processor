#pragma once

#include "../kdFileTree.h"

class RadiusFilter : public KdFileTree::InorderOperation
{
	public:

		static float R;
		static float R3;

		RadiusFilter(float iResolution, float iDensity);
		RadiusFilter(float iResolution, int iRadius, int iCutoff);

	protected:

		float mRadius;
		int mCutoff;

		void processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud);
};
