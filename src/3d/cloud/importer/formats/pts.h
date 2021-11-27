#pragma once

#include "importer.h"

class PtsImporter : public CloudImporter
{
	public:

		PtsImporter(json_spirit::mObject& iConfig);
	
		json_spirit::mObject import (std::string iName);
}; 
