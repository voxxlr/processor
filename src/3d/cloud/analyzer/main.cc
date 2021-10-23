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
	FILE* lFile = PointCloud::readHeader("cloud", 0, lPointCount);
	fclose(lFile);

	uint64_t lThreads = std::thread::hardware_concurrency();
	// 150 bytes per point includes kd tree memory file handles etc. 
	KdFileTree lFileTree;
	lFileTree.construct("cloud", std::min((uint64_t)(availableMemory()/150)/lThreads, lPointCount/lThreads), 0.00);
		
	Analyzer lAnalyzer;
	lFileTree.process2(lAnalyzer, KdFileTree::LEAVES);
	lFileTree.remove();

	// read meta
	/*
	std::ifstream lStream("meta.json");
	json_spirit::mValue lValue;
	json_spirit::read_stream(lStream, lValue);
	lStream.close();
	*/

	lFile = PointCloud::updateHeader("cloud");
	PointCloud::updateResolution(lFile, lAnalyzer.mResolution);
	/*
	json_spirit::mObject& lMeta = lValue.get_obj();
	lMeta["resolution"] = lAnalyzer.mResolution;// + lAnalyzer.mVariance;

	// write meta
	
	std::ofstream lOstream("meta.json");
	json_spirit::write_stream(lValue, lOstream);
	lOstream.close();
	*/
	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("filetree", "{  }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("analyzer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS;
}
