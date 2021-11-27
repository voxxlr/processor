#pragma once

#include "importer.h"


class LasImporter : public CloudImporter
{
	public:

		LasImporter(json_spirit::mObject& iConfig);
	
		json_spirit::mObject import (std::string iName);

	//	uint64_t append(FILE* iFIle, std::string iName, PointCloud& iCloud);
}; 
