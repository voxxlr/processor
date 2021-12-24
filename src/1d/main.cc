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
	json_spirit::write_stream(json_spirit::mValue(lCuber.mRoot), lOstream);
	lOstream.close();

	std::ofstream lOstream("process.json");
	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), lOstream);

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

	boost::filesystem::create_directories(boost::filesystem::path("D:/home/voxxlr/data/360/output"));
	_chdir("D:/home/voxxlr/data/360/output");

	Cuber lCuber;
	lCuber.process(std::string("../1615757969154_R0010011.JPG"));

	_chdir("..");
	
#endif

	task::initialize("cuber", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));

    return EXIT_SUCCESS;
}
