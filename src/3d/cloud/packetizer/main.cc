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

bool processFile(json_spirit::mObject& iObject)
{
	KdFileTree lFileTree;
	lFileTree.construct("cloud", 120000, 1.6*KdFileTree::SIGMA);
	lFileTree.fill(KdFileTree::SIGMA);

	PacketProcessor lProcessor;
	lFileTree.process2(lProcessor, KdFileTree::LEAVES | KdFileTree::INTERNAL);
	lFileTree.remove();

	// write root
	std::ofstream lStream("root.json");
	json_spirit::write_stream(json_spirit::mValue(lProcessor.mRootInfo), lStream);
	lStream.close();

	return true;
};

int main(int argc, char *argv[])
{
	/*
	PointCloudAttributes lAttributes;

	NormalProcessor lProcessor0;

	PointCloud lCloud;
	lCloud.readFile(std::string("n0000"));
	lProcessor0.initTraveral(lAttributes);
	lCloud.addAttributes(lAttributes);
	lProcessor0.computeNormals(lCloud, lCloud.getAttributeIndex(Attribute::NORMAL));
	lCloud.writeFile(std::string("n0000_N"));
	*/
	task::initialize("packetizer", "{ }", boost::function<bool(json_spirit::mObject&)>(processFile));
	//task::initialize("packetizer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
  