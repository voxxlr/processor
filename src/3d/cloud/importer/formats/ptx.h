#pragma once

#include "importer.h"

class PtxImporter : public CloudImporter
{
	public:

		PtxImporter(json_spirit::mObject& iConfig);
	
		json_spirit::mObject import (std::string iName);

	private:
	
		uint64_t readHeader(FILE* iFile, char* lLine, glm::dmat4& iMatrx);

}; 
