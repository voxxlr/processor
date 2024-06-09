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
#include "cuber.h"

bool processFile(json_spirit::mObject& iObject)
{
	Cuber lCuber;
	lCuber.process(iObject["file"].get_str());

	// write root
	std::ofstream lOstream("root.json");
	lCuber.mRoot["type"] = "panorama";
	json_spirit::write_stream(json_spirit::mValue(lCuber.mRoot), lOstream);
	lOstream.close();

	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), std::cout);

	return true;
};


#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
    

int main(int argc, char *argv[])
{
	task::initialize("cuber", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
