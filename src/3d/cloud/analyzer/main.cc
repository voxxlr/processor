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
	KdFileTree lFileTree(iObject["memory"].get_int64(), iObject["cpus"].get_int());
	lFileTree.construct("cloud", iObject["leafsize"].get_int(), 0.00);
		
	Analyzer lAnalyzer;
	lFileTree.process(lAnalyzer);
	lFileTree.remove();

	// read meta
	/*
	std::ifstream lStream("meta.json");
	json_spirit::mValue lValue;
	json_spirit::read_stream(lStream, lValue);
	lStream.close();
	*/

	FILE* lFile = PointCloud::updateHeader("cloud");
	PointCloud::updateResolution(lFile, lAnalyzer.mResolution);
	/*
	json_spirit::mObject& lMeta = lValue.get_obj();
	lMeta["resolution"] = lAnalyzer.mResolution;// + lAnalyzer.mVariance;

	// write meta
	
	std::ofstream lOstream("meta.json");
	json_spirit::write_stream(lValue, lOstream);
	lOstream.close();
	*/

	BOOST_LOG_TRIVIAL(info) << "DONE";
	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("filetree", "{\"cpus\": 8, \"memory\": 16383217664, \"input\": \"cloud\", \"leafsize\": 120000 }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("filetree", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS;
}
