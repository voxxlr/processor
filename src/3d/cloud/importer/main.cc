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
#include "formats/pts.h"
#include "formats/ptx.h"



bool processFile(json_spirit::mObject& iObject)
{
	uint8_t lCoords = CloudImporter::RIGHT_Z_UP;
	if (iObject.find("coords") != iObject.end())
	{
		lCoords = CloudImporter::toCoords(iObject["coords"].get_str());
	}

	float lScalar = 1.0;
	if (iObject.find("scalar") != iObject.end())
	{
		lScalar = iObject["scalar"].get_real();
	}

	glm::dvec3* lCenter = 0;
	if (iObject.find("center") != iObject.end())
	{
		lCenter = new glm::dvec3(0.0);
		json_spirit::mArray lArray = iObject["center"].get_array();
		(*lCenter)[0] = lArray[0].get_real();
		(*lCenter)[1] = lArray[1].get_real();
		(*lCenter)[2] = lArray[2].get_real();
	};

	float lResolution = 0;
	if (iObject.find("resolution") != iObject.end())
	{
		if (!iObject["resolution"].is_null())
		{
			lResolution = iObject["resolution"].get_real();
		}
	}

	std::string lFile = iObject["file"].get_str();
	std::string lType = boost::filesystem::extension(lFile);

	json_spirit::mObject lProperties;
	if (lType == ".las" || lType == ".laz" || lType == ".LAS" || lType == ".LAZ")
	{
		LasImporter lImporter(lCoords, lScalar, lResolution);
		lProperties = lImporter.import(lFile, lCenter);
	}
	else if (lType == ".e57")
	{
		E57Importer lImporter(lCoords, lScalar, lResolution);
		lProperties = lImporter.import(lFile, lCenter);
	}
	else if (lType == ".pts")
	{
		PtsImporter lImporter(lCoords, lScalar, lResolution);
		lProperties = lImporter.import(lFile, lCenter);
	}
	else if (lType == ".ptx")
	{
		PtxImporter lImporter(lCoords, lScalar, lResolution);
		lProperties = lImporter.import(lFile, lCenter);
	}
	else if (lType == ".ply")
	{
		PlyImporter lImporter(lCoords, lScalar, lResolution);
		lProperties = lImporter.import(lFile, lCenter);
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

	BOOST_LOG_TRIVIAL(info) << "DONE";
	return true;
};


int main(int argc, char *argv[])
{
	task::initialize("importer", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
