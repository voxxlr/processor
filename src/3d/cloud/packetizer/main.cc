#include <iostream>
#include <fstream>
#include <map>
#include <string>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "task.h"
#include "../kdFileTree.h"

#include "packetProcessor.h"

bool processFile(json_spirit::mObject& iConfig)
{
	float lResolution = PointCloud::readResolution(iConfig["file"].get_str());

	KdFileTree lFileTree;
	lFileTree.construct(iConfig["file"].get_str(), 120000, 1.6*KdFileTree::SIGMA*lResolution);
	lFileTree.fill(KdFileTree::SIGMA, lResolution);

	PacketProcessor lProcessor;
	lFileTree.process(lProcessor, KdFileTree::LEAVES | KdFileTree::INTERNAL);
	lFileTree.remove();

	// write root
	std::ofstream lStream("root.json");
	lProcessor.mRoot["type"] = "cloud";
	json_spirit::write_stream(json_spirit::mValue(lProcessor.mRoot), lStream);
	lStream.close();

	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), std::cout);

	return true;
};

int main(int argc, char *argv[])
{
	task::initialize("packetizer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
  