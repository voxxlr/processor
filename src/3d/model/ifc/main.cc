#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <iostream>
#include <fstream>
#include <map>
#include <string>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include "task.h"

#include "IfcLoader.h"
#include "IfcParser.h"

bool processFile(json_spirit::mObject& iObject)
{
	IfcLoader lLoader;
	lLoader.process(iObject["file"].get_str());

	IfcParser lParser;
	Model* lModel = lParser.processIfcProject(*lLoader.getProject());  
	json_spirit::mObject lRoot = lModel->write(boost::filesystem::path("."));

	// write root
	std::ofstream lOstream("root.json");
	lRoot["type"] = "model";
	json_spirit::write_stream(json_spirit::mValue(lRoot), lOstream);
	lOstream.close();
	
	// return json object
	json_spirit::mObject lResult;
	json_spirit::write_stream(json_spirit::mValue(lResult), std::cout);

	return true;  
};
     
int main(int argc, char *argv[])  
{  
	task::initialize("ifc", argv[1], boost::function<bool(json_spirit::mObject&)>(processFile));
	
    return EXIT_SUCCESS; 
}

