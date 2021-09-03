#pragma once

#include "importer.h"

class PtsImporter : public CloudImporter
{
	public:

		PtsImporter(uint8_t iCoords, float iScalar, float iResolution);
	
		json_spirit::mObject import (std::string iName, glm::dvec3* iCenter);
}; 
