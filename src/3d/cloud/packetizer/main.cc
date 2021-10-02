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
	lFileTree.process(NormalProcessor(), KdFileTree::LEAVES | KdFileTree::INTERNAL);

	PacketProcessor lProcessor;
	lFileTree.process(PacketProcessor(), KdFileTree::LEAVES | KdFileTree::INTERNAL);
	lFileTree.remove();

	// write root
	std::ofstream lRstream("root.json");
	json_spirit::write_stream(json_spirit::mValue(lProcessor.mRootInfo), lRstream);
	lRstream.close();

	BOOST_LOG_TRIVIAL(info) << "DONE";
	return true;
};

int main(int argc, char *argv[])
{
	//task::initialize("packetizer", "{\"cpus\": 2, \"memory\": 2383217664  }", boost::function<bool(json_spirit::mObject&)>(processFile));
	task::initialize("packetizer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
  