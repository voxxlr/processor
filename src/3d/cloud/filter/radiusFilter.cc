#include "../kdFileTree.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "radiusFilter.h"

RadiusFilter::RadiusFilter(float iResolution, float iDensity)
: InorderOperation("Radiusfilter")
, mCutoff(4.1887*R3*iDensity)
, mRadius(R*iResolution)
{
	BOOST_LOG_TRIVIAL(info) << "Resolution = " << iResolution;
	BOOST_LOG_TRIVIAL(info) << "Density = " << iDensity;
	BOOST_LOG_TRIVIAL(info) << "Radius = " << mRadius;
	BOOST_LOG_TRIVIAL(info) << "Cutoff = " << mCutoff;
}

RadiusFilter::RadiusFilter(float iResolution, int iRadius, int iCutoff)
: InorderOperation("Radiusfilter")
, mCutoff(iCutoff)
, mRadius(iRadius*iResolution)
{
	BOOST_LOG_TRIVIAL(info) << "Resolution = " << iResolution;
	BOOST_LOG_TRIVIAL(info) << "Radius = " << mRadius;
	BOOST_LOG_TRIVIAL(info) << "Cutoff = " << mCutoff;
}


void RadiusFilter::processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	KdTree<KdSpatialDomain> lTree(iCloud);
	lTree.construct();

	FILE* lFile = PointCloud::writeHeader(iNode.mPath, iCloud);
	uint32_t lSelected = 0;
	for (uint32_t p=0; p<iCloud.size(); p++)
	{
		if (lTree.detect(p, mRadius, mCutoff))
		{
			iCloud[p]->write(lFile);
			lSelected++;
		}
	}
	iCloud.updateSize(lFile, lSelected);
	fclose(lFile);

	BOOST_LOG_TRIVIAL(info) << "Filtered " << iNode.mPath << " to " << (lSelected*100.0)/iCloud.size() << "%";
};

// This is not done... try to do this only for the split axis in the kdTree... might be enough
float RadiusFilter::R = 5;
float RadiusFilter::R3 = RadiusFilter::R*RadiusFilter::R*RadiusFilter::R;
