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
	KdFileTree lFileTree(iObject["memory"].get_int64(), iObject["cpus"].get_int());
	lFileTree.construct("cloud", iObject["leafsize"].get_int(), 1.1*KdFileTree::SIGMA);

	VoxelFilter lVoxelFilter(lFileTree.mResolution);
	lFileTree.process(lVoxelFilter);

	if (iObject.find("density") != iObject.end())
	{
		if (!iObject["density"].is_null())
		{
			RadiusFilter lRadiusFilter(lFileTree.mResolution, iObject["density"].get_real());
			lFileTree.process(lRadiusFilter, KdFileTree::LEAVES);
		}
	}

	lFileTree.collapse("cloud");
	lFileTree.remove();

	BOOST_LOG_TRIVIAL(info) << "DONE";
	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("filetree", "{\"cpus\": 8, \"memory\": 16383217664, \"leafsize\": 120000 }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("filetree", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS;
}
