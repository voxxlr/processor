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

	// color
	if (iObject.find("color") != iObject.end())
	{
		TmsTiler lTiler;

		lTiler.process(iObject["color"].get_str());
		lRoot["color"] = lTiler.mRoot;
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

	std::ofstream lProcess("process.json");
	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), lProcess);

	return true;
};

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
    

int main(int argc, char *argv[])
{
	task::initialize("tiler", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS; 
}
