#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "task.h"

#include "tiler.h"

bool processFile(json_spirit::mObject& iObject)
{
	json_spirit::mObject lRoot;

//	std::stringstream ll;
//	json_spirit::write_stream(json_spirit::mValue(lSource), ll);
//	BOOST_LOG_TRIVIAL(error) << ll.str();

	// color
	if (iObject.find("color") != iObject.end())
	{
		TmsTiler lTiler;

		lTiler.process(iObject["color"].get_str());
		lRoot["color"] = lTiler.mRoot;

		// add projection to meta data
		/*
		std::ifstream lStream("meta.json");
		json_spirit::mValue lValue;
		json_spirit::read_stream(lStream, lValue);
		lStream.close();

		json_spirit::mObject& lMeta = lValue.get_obj();
		lMeta["proj"] = lTiler.mRoot["proj"];

		// write meta
		std::ofstream lOstream("meta.json");
		json_spirit::write_stream(lValue, lOstream);
		lOstream.close();
		*/
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "NO COLOR";
	}

	// elevation if exists
	if (iObject.find("elevation") != iObject.end())
	{
		TmsTiler lTiler;
		lTiler.process(iObject["elevation"].get_str());
		lRoot["elevation"] = lTiler.mRoot;
	}
	else
	{
		BOOST_LOG_TRIVIAL(info) << "NO ELEVATION";
	}

	// write root
	std::ofstream lOstream("root.json");
	json_spirit::write_stream(json_spirit::mValue(lRoot), lOstream);
	lOstream.close();

	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), std::cout);

	return true;
};

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
    
#ifdef _WIN32

#define TEST 1   

#endif


int main(int argc, char *argv[])
{

#if defined TEST

	TmsTiler lTiler;

	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/map/output"));
	chdir("D:/home/voxxlr/data/map/output");
	//lTiler.process("D:/tiff/MMC_Bolar_Ortho_clip_v1.tif");
	//lTiler.process("D:/tiff/1546630597157.tif");

	//lTiler.process("D:/tiff/DroneMapper_AdobeButtes3D-Flight1_2/Ortho-DroneMapper.tif");

	//lTiler.process("D:/tiff/Ortho-DrnMppr.tif");
	//lTiler.process("D:/tiff/depth.tif");
	//lTiler.process("D:/tiff/depth.tif");

	//lTiler.process("D:/tiff/doc-0-upload.tif");
	/*
	FILE* lFile = fopen("D:/home/voxxlr/data/map/1585783781601_NAD83_UTM_11N_for_Voxxlr.tif", "rb");
	if (lFile)
	{
		char lBuffer[1024];
		fread(lBuffer, 1024, 1, lFile);
	}
	*/

	lTiler.process("D:/home/voxxlr/data/map/color.tif");
	//lTiler.process("D:/STRABAG/181102/DSM/Teil_1_dsm.tif");
	
	//lTiler.process("D:/home/voxxlr/data/map/color.tif");
	chdir("..");
	
#endif

	task::initialize("tiler", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS; 
}
