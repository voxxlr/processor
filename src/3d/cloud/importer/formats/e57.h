#pragma once

#include "importer.h"


class E57Importer  : public CloudImporter
{
	public:

		E57Importer(json_spirit::mObject& iConfig);
	
		json_spirit::mObject import (std::string iName);

	protected:

		float mRadius2;

		bool filtered(std::string iName);

}; 
