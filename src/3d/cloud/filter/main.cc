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


bool processFile(json_spirit::mObject& iObject)
{
	uint64_t lPointCount;
	FILE* lFile = PointCloud::readHeader("cloud", 0, lPointCount);
	fclose(lFile);

	uint64_t lThreads = std::thread::hardware_concurrency();
	// 250 bytes per point includes voxel grid memory, file handles etc. 
	KdFileTree lFileTree;
	lFileTree.construct("cloud", std::min((uint64_t)(availableMemory() / 250) / lThreads, lPointCount / lThreads), 1.1*KdFileTree::SIGMA);

	VoxelFilter lVoxelFilter(lFileTree.mResolution);
	lFileTree.process2(lVoxelFilter, KdFileTree::LEAVES);

	if (iObject.find("density") != iObject.end())
	{
		if (!iObject["density"].is_null())
		{
			RadiusFilter lRadiusFilter(lFileTree.mResolution, iObject["density"].get_real());
			lFileTree.process2(lRadiusFilter, KdFileTree::LEAVES);
		}
	}

	lFileTree.collapse("cloud");
	lFileTree.remove();

	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("filetree", "{  }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("filter", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS;
}
