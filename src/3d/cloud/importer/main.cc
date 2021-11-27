#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include "float.h" 
 
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp> 
#include <boost/filesystem.hpp>

#include "task.h"
#include "../point.h"
#include "../kdTree.h"
#include "../kdFileTree.h"

#include "formats/importer.h"
#include "formats/ply.h"
#include "formats/e57.h"
#include "formats/las.h"
#include "formats/ptx.h"
#include "formats/pts.h"
  

bool processFile(json_spirit::mObject& iConfig)
{
	json_spirit::mArray lFiles = iConfig["file"].get_array();
	json_spirit::mObject lProperties;

	std::string lType = boost::filesystem::extension(lFiles[0].get_str());
	if (lType == ".e57")
	{
		E57Importer lImporter(iConfig);
		lProperties = lImporter.import(lFiles[0].get_str()); // only one supported
	}
	else if (lType == ".las" || lType == ".laz" || lType == ".LAS" || lType == ".LAZ")
	{
		LasImporter lImporter(iConfig);
		lProperties = lImporter.import(lFiles[0].get_str());
	}
	else if (lType == ".e57")
	{
		E57Importer lImporter(iConfig);
		lProperties = lImporter.import(lFiles[0].get_str());
	}
	else if (lType == ".pts")
	{
		PtsImporter lImporter(iConfig);
		lProperties = lImporter.import(lFiles[0].get_str());
	}
	else if (lType == ".ptx")
	{
		PtxImporter lImporter(iConfig);
		lProperties = lImporter.import(lFiles[0].get_str());
	}
	else if (lType == ".ply")  /* this can only be used internally */
	{
		PlyImporter lImporter(iConfig);
		lProperties = lImporter.import(lFiles, iConfig["output"].get_str());
	}

	std::ifstream lStream("meta.json");
	json_spirit::mValue lValue;
	json_spirit::read_stream(lStream, lValue);
	lStream.close();

	json_spirit::mObject& lMeta = lValue.get_obj();
	for (auto lIter = lProperties.begin(); lIter != lProperties.end(); lIter++)
	{
		lMeta[lIter->first] = lIter->second;
	}

	// write meta
	std::ofstream lOstream("meta.json");
	json_spirit::write_stream(lValue, lOstream);
	lOstream.close();

	json_spirit::write_stream(json_spirit::mValue(lProperties), std::cout);
	return true;
};


int main(int argc, char *argv[])
{
	task::initialize("importer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
