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
	
	json_spirit::mObject lFilter = iConfig["filter"].get_obj();

	if (lFilter.find("voxel") != iConfig.end() && !lFilter["voxel"].is_null())
	{
		VoxelFilter lVoxelFilter(lResolution);
		lFileTree.process(lVoxelFilter, KdFileTree::LEAVES);
	}

	if (lFilter.find("density") != iConfig.end() && !lFilter["density"].is_null())
	{
		RadiusFilter lRadiusFilter(lResolution, lFilter["density"].get_real());
		lFileTree.process(lRadiusFilter, KdFileTree::LEAVES);
	}

	lFileTree.collapse(iConfig["file"].get_str(), lResolution);
	lFileTree.remove();

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
