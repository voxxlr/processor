#pragma once

#include "importer.h"

class E57Importer  : public CloudImporter
{
	public:

		E57Importer(uint8_t iCoords, float iScalar, float iResolution);
	
		json_spirit::mObject import (std::string iName, glm::dvec3* iCenter);

}; 
