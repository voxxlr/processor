#pragma once

#include "importer.h"


class LasImporter : public CloudImporter
{
	public:

		LasImporter(uint8_t iCoords, float iScalar, float iResolution);
	
		json_spirit::mObject import (std::string iName, glm::dvec3* iCenter);

	//	uint64_t append(FILE* iFIle, std::string iName, PointCloud& iCloud);
}; 
