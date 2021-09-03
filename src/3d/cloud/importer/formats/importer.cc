#include <limits>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "importer.h"


CloudImporter::CloudImporter(uint8_t iCoords, float iScalar, float iResolution)
: mCoords(iCoords)
, mScalarD(iScalar)
, mResolution(iResolution)
{
	mMinD[0] = std::numeric_limits<double>::max();
	mMinD[1] = std::numeric_limits<double>::max();
	mMinD[2] = std::numeric_limits<double>::max();

	mMaxD[0] = -std::numeric_limits<double>::max();
	mMaxD[1] = -std::numeric_limits<double>::max();
	mMaxD[2] = -std::numeric_limits<double>::max();

	mCenter[0] = 0;
	mCenter[1] = 0;
	mCenter[2] = 0;
};

uint16_t CloudImporter::sDefaultClasses[19*3] = 
{
	0,0,255,
	0,255,0,
	158,127,109,		// 2 Ground
	79,119,63,			// 3 Low Vegetation
	33,137,33,			// 4 Medium Vegetation
	206,239,191,		// 5 High Vegetation
	137,0,25,			// 6 Building
	137,137,137,		// 7 Low Point
	137,137,137,		// 8 Reserved
	63,163,221,			// 9 Water
	97,97,97,		    // 10 Rail
	137,137,137,		// 11 Road Surface
	137,137,137,		// 12 Reserved
	137,137,137,		// 13 Wire - Guard (Shield)
	137,137,137,		// 14 Wire - Conductor (Phase)
	137,137,137,		// 15 Transmission Tower
	137,137,137,		// 16 Wire-Structure Connector (Insulator)
	137,137,137,		// 17 Bridge Deck
	255,255,255			// 18 High Noise  
};

json_spirit::mObject CloudImporter::getMeta(uint8_t iMaxClass)
{
    json_spirit::mObject lMeta;

	json_spirit::mObject lTransform;
	json_spirit::mObject lTranslate;
	lTranslate["x"] = mCenter[0];
	lTranslate["y"] = mCenter[1];
	lTranslate["z"] = mCenter[2];
	lTransform["position"] = lTranslate;
    lMeta["origin"] = lTransform;

    if (iMaxClass > 0)
    {
	    json_spirit::mArray lClasses;

        for (int i=0; i<std::max(iMaxClass, (uint8_t)19); i++)
        {
	        json_spirit::mObject lColor;
	        lColor["r"] = sDefaultClasses[i*3+0];
	        lColor["g"] = sDefaultClasses[i*3+1];
	        lColor["b"] = sDefaultClasses[i*3+2];
            lClasses.push_back(lColor);
        }
        for (int i=19; i<iMaxClass; i++)
        {
 	        json_spirit::mObject lColor;
	        lColor["r"] = 90;
	        lColor["g"] = 90;
	        lColor["b"] = 90;
            lClasses.push_back(lColor);
       }

        lMeta["classes"] = lClasses;
    }

	return lMeta;
}
        		

void CloudImporter::done() 
{
	BOOST_LOG_TRIVIAL(info) << "importer done   " ;
	BOOST_LOG_TRIVIAL(info) << "importer done - min " << mMinD[0] << ", " <<  mMinD[1] << ", " << mMinD[2];
	BOOST_LOG_TRIVIAL(info) << "importer done - max " << mMaxD[0] << ", " <<  mMaxD[1] << ", " << mMaxD[2];
	BOOST_LOG_TRIVIAL(info) << "importer done - pos " << mCenter[0] << ", " <<  mCenter[1] << ", " << mCenter[2];
}


/*
	
		json_spirit::mObject getTransform()
		{
			json_spirit::mObject lTransform;
			json_spirit::mObject lTranslate;
			lTranslate["x"] = mCenter[0];
			lTranslate["y"] = mCenter[1];
			lTranslate["z"] = mCenter[2];
			lTransform["position"] = lTranslate;
			return lTransform;
		}

*/