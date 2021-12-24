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

#include "analyzer.h"


bool processFile(json_spirit::mObject& iObject)
{
	uint64_t lPointCount; 
	FILE* lFile = PointCloud::readHeader(iObject["file"].get_str(), 0, lPointCount);
	fclose(lFile);

	uint64_t lThreads = std::thread::hardware_concurrency();
	// 150 bytes per point includes kd tree memory file handles etc. 
	KdFileTree lFileTree;
	lFileTree.construct(iObject["file"].get_str(), std::min((uint64_t)(availableMemory()/150)/lThreads, lPointCount/lThreads), 0.00);

	Analyzer lAnalyzer;
	lFileTree.process(lAnalyzer, KdFileTree::LEAVES);
	lFileTree.remove();

	// update resolution in ply file
	lFile = PointCloud::updateHeader(iObject["file"].get_str());
	PointCloud::updateResolution(lFile, lAnalyzer.mResolution);

	json_spirit::mObject lResult;
	lResult["resolution"] = lAnalyzer.mResolution;
	lResult["variance"] = lAnalyzer.mVariance;
	std::ofstream lOstream("process.json");
	json_spirit::write_stream(json_spirit::mValue(lResult), lOstream);

	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("filetree", "{  }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("analyzer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS;
}
