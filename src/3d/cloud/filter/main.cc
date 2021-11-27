#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <iostream>
#include <fstream>

#include "task.h"

#include "../kdFileTree.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "voxelFilter.h"
#include "radiusFilter.h"


bool processFile(json_spirit::mObject& iConfig)
{
	uint64_t lPointCount;
	FILE* lFile = PointCloud::readHeader(iConfig["file"].get_str(), 0, lPointCount);
	fclose(lFile); 

	float lResolution = iConfig["resolution"].get_real();

	uint64_t lThreads = std::thread::hardware_concurrency();
	// 250 bytes per point includes voxel grid memory, file handles etc. 
	KdFileTree lFileTree;

	lFileTree.construct(iConfig["file"].get_str(), std::min((uint64_t)(availableMemory() / 250) / lThreads, lPointCount / lThreads), 1.1*KdFileTree::SIGMA*lResolution);
	
	VoxelFilter lVoxelFilter(lResolution);
	lFileTree.process(lVoxelFilter, KdFileTree::LEAVES);

	BOOST_LOG_TRIVIAL(info) << "AAAAAAAAAAAAAAA";
	if (iConfig.find("density") != iConfig.end() && !iConfig["density"].is_null())
	{
		BOOST_LOG_TRIVIAL(info) << "DDDDDDDDDDDDDDDDDDDD = ";

		RadiusFilter lRadiusFilter(lResolution, iConfig["density"].get_real());
		lFileTree.process(lRadiusFilter, KdFileTree::LEAVES);
	}
	BOOST_LOG_TRIVIAL(info) << "CCCCCCCCCCCC";

	lFileTree.collapse(iConfig["file"].get_str(), lResolution);
	lFileTree.remove();

	BOOST_LOG_TRIVIAL(info) << "DDDDDDDDDDDDDDD";

	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), std::cout);

	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("filetree", "{  }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("filter", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS;
}
