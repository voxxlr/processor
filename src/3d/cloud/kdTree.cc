#include <vector>
#include <algorithm>

#include "point.h"		
#include "kdTree.h"

//
// Position attribute
//

KdSpatialDomain::KdSpatialDomain(PointCloud& iCloud)
: mPointCloud(iCloud)
, mAxis(0)
{
};

void KdSpatialDomain::axisSort(std::vector<uint32_t>& iIndex, uint32_t iLower, uint32_t iUpper, uint8_t iAxis)
{
	mAxis = iAxis;
	std::sort (iIndex.begin()+iLower, iIndex.begin()+iUpper, *this);     
};

KdSpatialNormalDomain::KdSpatialNormalDomain(PointCloud& iCloud)
: KdSpatialDomain(iCloud)
, mNormalIndex(iCloud.getAttributeIndex(Attribute::NORMAL))
{
};







KdColorDomain::KdColorDomain(PointCloud& iCloud)
: mPointCloud(iCloud)
, mAxis(0)
, mColorIndex(iCloud.getAttributeIndex(Attribute::COLOR))
{
};

void KdColorDomain::axisSort(std::vector<uint32_t>& iIndex, uint32_t iLower, uint32_t iUpper, uint8_t iAxis)
{
	mAxis = iAxis;
	std::sort (iIndex.begin()+iLower, iIndex.begin()+iUpper, *this);     
};


KdColorNormalDomain::KdColorNormalDomain(PointCloud& iCloud)
: KdColorDomain(iCloud)
, mNormalIndex(iCloud.getAttributeIndex(Attribute::NORMAL))
{
};





KdSpatialColorDomain::KdSpatialColorDomain(PointCloud& iCloud)
: KdSpatialDomain(iCloud)
, mColorIndex(iCloud.getAttributeIndex(Attribute::COLOR))
{
};