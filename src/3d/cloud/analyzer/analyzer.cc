#include "../kdFileTree.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "analyzer.h"

#define TIMING 1

class Random
{
public:

	Random(uint32_t iSeed, uint32_t iMax)
		: mSeed(iSeed)
		, mMax(iMax)
	{
	};

	inline uint32_t next()
	{
		uint32_t const a = 1103515245;
		uint32_t const c = 12345;
		mSeed = a * mSeed + c;
		return ((mSeed >> 16) & 0x7FFF) % mMax;
	}

	void seed(uint32_t iSeed)
	{
		mSeed = iSeed;
	}

private:

	uint32_t mMax;
	uint32_t mSeed;
};

Analyzer::Analyzer()
: LeafOperation("Analyzer")
, mResolution(0)
, mVariance(0)
{
}

void Analyzer::processLeaf(KdFileTreeNode& iNode, PointCloud& iCloud)
{
#ifdef TIMING
	boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();
#endif

	KdTree<KdSpatialDomain> lTree(iCloud);
	lTree.construct();

#ifdef TIMING
	boost::posix_time::ptime t2 = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_duration diff = t2 - t1;
	t1 = t2;
#endif

	// compute mean distance to neightbors for 1% of points
	size_t lMax = iCloud.size()/100;

	// select points outside overlap
	std::vector<uint32_t> lSamples;
	lSamples.reserve(lMax);
	Random lRandom(1, iCloud.size());
	uint32_t lAttempts = 2*lMax;
	while (lSamples.size() < lMax && lAttempts)
	{
		uint32_t lIndex = lRandom.next();
		Point& lPoint = *iCloud[lIndex];
		if (lPoint.inside(iNode.min, iNode.max))
		{
			lSamples.push_back(lIndex);
		}
		lAttempts--;
	}

	// compute average distances
	std::vector<std::pair<uint32_t, float>> lRadiusSearch;
	#define N 3
	for (uint32_t i=0; i<lSamples.size(); i++) 
	{
		lRadiusSearch.clear();
		lTree.knn<N>(lSamples[i], lRadiusSearch);

		// compute average distance to neightbors for this point
		Point& lPoint = *iCloud[lSamples[i]];
		lPoint.mWeight = 0;
		for (int n=0; n<N; n++)
		{
			lPoint.mWeight += lRadiusSearch[n].second;
		}
		lPoint.mWeight /= N;
	}
	#undef N

#ifdef TIMING
	t2 = boost::posix_time::second_clock::local_time();
	diff = t2 - t1;
	BOOST_LOG_TRIVIAL(info) << "sampled kdTree in " << diff.total_milliseconds()/1000 << " seconds";
#endif

	if (lSamples.size())
	{
		double lMean = 0;
		for (uint32_t i=0; i<lSamples.size(); i++) 
		{
			Point& lPoint = *iCloud[lSamples[i]];
			lMean += lPoint.mWeight;
		}
		lMean/=lSamples.size();

		double lVariance = 0;
		for (uint32_t i=0; i<lSamples.size(); i++) 
		{
			Point& lPoint = *iCloud[lSamples[i]];
			double lDifference = lPoint.mWeight - lMean;
			lVariance += lDifference*lDifference;
		}
		lVariance/=lSamples.size();

		mVectorLock.lock();
		mNodes.push_back(std::tuple<double, double, uint32_t>(lMean, lVariance, lSamples.size()));
		mVectorLock.unlock();
	}
}

void Analyzer::completeTraveral(PointCloudAttributes& iAttributes)
{
	uint64_t lCount = 0;
	for (std::vector<std::tuple<double, double, uint32_t>>::iterator lIter = mNodes.begin(); lIter != mNodes.end(); lIter++)
	{
		BOOST_LOG_TRIVIAL(info) << "Mean = " << std::get<0>(*lIter) << " variance = " << std::get<1>(*lIter) << " weight = " << std::get<2>(*lIter);

		mResolution = mResolution*lCount + std::get<0>(*lIter)*std::get<2>(*lIter);
		mVariance = mVariance*lCount + std::get<1>(*lIter)*std::get<2>(*lIter);

		lCount += std::get<2>(*lIter);

		mResolution /= lCount;
		mVariance /= lCount;
	};

	BOOST_LOG_TRIVIAL(info) << "Resolution = " << mResolution << " varriance = " << mVariance;	
}










	/*
	size_t lSamples = std::max<size_t>(iCloud.size()/100, 1);
	
	Random lRandom(1, iCloud.size());
	std::vector<std::pair<uint32_t, float>> lRadiusSearch;
	#define N 4
	for (size_t i=0; i<lSamples; i++) 
	{
		size_t lIndex = lRandom.next();
		lRadiusSearch.clear();
		lTree.knn<N>(lIndex, lRadiusSearch);

		// compute average distance for this point
		Point& lPoint = *iCloud[lIndex];
		lPoint.mWeight = 0;
		for (int n=0; n<N; n++)
		{
			lPoint.mWeight += lRadiusSearch[n].second;
		}
		lPoint.mWeight /= N;
	}
	#undef N

#ifdef TIMING
	t2 = boost::posix_time::second_clock::local_time();
	diff = t2 - t1;
	BOOST_LOG_TRIVIAL(info) << "sampled kdTree in " << diff.total_milliseconds()/1000 << " seconds";
#endif

	double lMean = 0;
	lRandom.seed(1);
	for (size_t i=0; i<lSamples; i++) 
	{
		Point& lPoint = *iCloud[lRandom.next()];
		lMean += lPoint.mWeight;
	}
	lMean/=lSamples;

	double lVariance = 0;
	lRandom.seed(1);
	for (size_t i=0; i<lSamples; i++) 
	{
		Point& lPoint = *iCloud[lRandom.next()];
		double lDifference = lPoint.mWeight - lMean;
		lVariance += lDifference*lDifference;
	}
	lVariance/=lSamples;
	*/