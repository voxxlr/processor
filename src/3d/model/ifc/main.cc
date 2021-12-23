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

using boost::asio::ip::tcp;

#include "task.h"

#include "IfcLoader.h"
#include "IfcParser.h"

bool processFile(json_spirit::mObject& iObject)
{
	BOOST_LOG_TRIVIAL(error) << "PATH:"  << boost::filesystem::current_path();

	IfcLoader lLoader;
	lLoader.process(iObject["file"].get_str());

	IfcParser lParser;
	Model* lModel = lParser.processIfcProject(*lLoader.getProject());  
	json_spirit::mObject lRoot = lModel->write(boost::filesystem::path("."));

	// write root
	std::ofstream lOstream("root.json");
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


	/*
	boost::filesystem::path targetDir(".");
	boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator iter(targetDir); iter != end; ++iter)
    {
        const boost::filesystem::path lPath = (*iter);

		BOOST_LOG_TRIVIAL(error) << lPath.string();
		if (boost::filesystem::extension(lPath.string()) == ".ifc")
		{
			IfcLoader lLoader;
			IfcProject* lProject = lLoader.process(lPath.string());

			IfcParser lParser;
			Model* lModel = lParser.process(*lProject, lSource);
			iRequest["root"] = lModel->write();

			json_spirit::mObject lRoot = iRequest["root"].get_obj();
			lMeta["min"] = lRoot["min"].get_obj();
			lMeta["max"] = lRoot["max"].get_obj();
			break;
		}
    }
	*/

	/*
	json_spirit::mArray& lArray = lSource["files"].get_array();
	for (int i=0; i<lArray.size(); i++)
	{
		if (boost::algorithm::ends_with(lArray[i].get_str(), "ifc"))
		{
			IfcLoader lLoader;
			IfcProject* lProject = lLoader.process(lArray[i].get_str());

			IfcParser lParser;
			Model* lModel = lParser.process(*lProject, lSource);
			iRequest["root"] = lModel->write();

			json_spirit::mObject lRoot = iRequest["root"].get_obj();
			lMeta["min"] = lRoot["min"].get_obj();
			lMeta["max"] = lRoot["max"].get_obj();
			break;
		}
	}
	*/
	