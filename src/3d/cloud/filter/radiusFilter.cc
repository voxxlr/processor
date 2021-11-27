#include "../kdFileTree.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "radiusFilter.h"

#define _USE_MATH_DEFINES
#include <math.h>

float RadiusFilter::R = 5; // mutliple of resolution
float RadiusFilter::R3 = RadiusFilter::R*RadiusFilter::R*RadiusFilter::R;

RadiusFilter::RadiusFilter(float iResolution, float iDensity)
: InorderOperation("Radiusfilter")
, mMinPoints(4.0/3.0*M_PI*R3*iDensity)
, mRadius(R*iResolution)
{
	BOOST_LOG_TRIVIAL(info) << "Resolution = " << iResolution;
	BOOST_LOG_TRIVIAL(info) << "Density = " << iDensity;
	BOOST_LOG_TRIVIAL(info) << "Radius = " << mRadius;
	BOOST_LOG_TRIVIAL(info) << "Min # of Points = " << mMinPoints;
}

void RadiusFilter::processNode(KdFileTreeNode& iNode, PointCloud& iCloud)
{
	KdTree<KdSpatialDomain> lTree(iCloud);
	lTree.construct();

	FILE* lFile = PointCloud::writeHeader(iNode.mPath, iCloud);
	uint32_t lSelected = 0;
	for (uint32_t p=0; p<iCloud.size(); p++)
	{
		if (lTree.detect(p, mRadius, mMinPoints))
		{
			iCloud[p]->write(lFile);
			lSelected++;
		}
	}
	iCloud.updateSize(lFile, lSelected);
	fclose(lFile);

	BOOST_LOG_TRIVIAL(info) << "Filtered " << iNode.mPath << " to " << (lSelected*100.0)/iCloud.size() << "%";
};
