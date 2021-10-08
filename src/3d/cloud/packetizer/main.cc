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
#include "normalProcessor.h"

bool processFile(json_spirit::mObject& iObject)
{
	KdFileTree lFileTree(iObject["memory"].get_int64(), iObject["cpus"].get_int());
	lFileTree.construct("cloud", 120000, 1.1*KdFileTree::SIGMA);
	lFileTree.fill(KdFileTree::SIGMA);

	NormalProcessor lProcessor0;
	lFileTree.process(lProcessor0, KdFileTree::LEAVES | KdFileTree::INTERNAL);

	PacketProcessor lProcessor1;
	lFileTree.process(lProcessor1, KdFileTree::LEAVES | KdFileTree::INTERNAL);
	lFileTree.remove();

	// write root
	std::ofstream lRstream("root.json");
	json_spirit::write_stream(json_spirit::mValue(lProcessor1.mRootInfo), lRstream);
	lRstream.close();

	BOOST_LOG_TRIVIAL(info) << "DONE";
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
	//task::initialize("packetizer", "{\"cpus\": 4, \"memory\": 2383217664  }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("packetizer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
  