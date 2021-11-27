#pragma once

#include "importer.h"

class PlyImporter : public CloudImporter
{
	public:

		PlyImporter(json_spirit::mObject& iConfig);
	
		json_spirit::mObject import(json_spirit::mArray iFiles, std::string iOutput);

}; 
