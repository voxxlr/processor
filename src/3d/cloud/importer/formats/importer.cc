#include <limits>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "importer.h"


CloudImporter::CloudImporter(json_spirit::mObject& iConfig)
: mCoords(CloudImporter::RIGHT_Z_UP)
, mScalarD(1.0)
, mTransform(1.0)
, mConfig(iConfig)
{
	if (iConfig.find("coords") != iConfig.end())
	{
		if (!iConfig["coords"].is_null())
		{
			mCoords = CloudImporter::toCoords(iConfig["coords"].get_str());
		}
	}

	if (iConfig.find("scalar") != iConfig.end())
	{
		if (!iConfig["scalar"].is_null())
		{
			mScalarD = iConfig["scalar"].get_real();
		}
	}

	if (iConfig.find("transform") != iConfig.end())
	{
		if (!iConfig["transform"].is_null())
		{
			json_spirit::mArray lMatrix = iConfig["transform"].get_array();
			for (int r=0; r<4; r++)
			{
				mTransform[r][0] = lMatrix[r*4+0].get_real();
				mTransform[r][1] = lMatrix[r*4+1].get_real();
				mTransform[r][2] = lMatrix[r*4+2].get_real();
				mTransform[r][3] = lMatrix[r*4+3].get_real();
			}
		}
	};

	BOOST_LOG_TRIVIAL(info) << "CT: " << mTransform[0][0] << " " << mTransform[0][1] << " " << mTransform[0][2] << " " << mTransform[0][3];
	BOOST_LOG_TRIVIAL(info) << "    " << mTransform[1][0] << " " << mTransform[1][1] << " " << mTransform[1][2] << " " << mTransform[1][3];
	BOOST_LOG_TRIVIAL(info) << "    " << mTransform[2][0] << " " << mTransform[2][1] << " " << mTransform[2][2] << " " << mTransform[2][3];
	BOOST_LOG_TRIVIAL(info) << "    " << mTransform[3][0] << " " << mTransform[3][1] << " " << mTransform[3][2] << " " << mTransform[3][3];

	mMinD[0] = std::numeric_limits<double>::max();
	mMinD[1] = std::numeric_limits<double>::max();
	mMinD[2] = std::numeric_limits<double>::max();

	mMaxD[0] = -std::numeric_limits<double>::max();
	mMaxD[1] = -std::numeric_limits<double>::max();
	mMaxD[2] = -std::numeric_limits<double>::max();
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

json_spirit::mObject CloudImporter::getMeta()
{
    json_spirit::mObject lMeta;

	json_spirit::mArray lTransform;
	for (int i=0; i<4; i++)
	{
		json_spirit::mArray lColumn;
		lColumn.push_back(json_spirit::mValue(mTransform[i][0]));
		lColumn.push_back(json_spirit::mValue(mTransform[i][1]));
		lColumn.push_back(json_spirit::mValue(mTransform[i][2]));
		lColumn.push_back(json_spirit::mValue(mTransform[i][3]));
		lTransform.push_back(lColumn);
	}
	lMeta["transform"] = lTransform;

	return lMeta;
}
        		

void CloudImporter::done() 
{
	BOOST_LOG_TRIVIAL(info) << "importer done   " ;
	BOOST_LOG_TRIVIAL(info) << "importer done - min " << mMinD[0] << ", " <<  mMinD[1] << ", " << mMinD[2];
	BOOST_LOG_TRIVIAL(info) << "importer done - max " << mMaxD[0] << ", " <<  mMaxD[1] << ", " << mMaxD[2];
	BOOST_LOG_TRIVIAL(info) << "importer done - transform " << mTransform[0][0] << ", " <<  mTransform[0][0] << ", " << mTransform[0][0];
	BOOST_LOG_TRIVIAL(info) << "                          " << mTransform[0][0] << ", " <<  mTransform[0][0] << ", " << mTransform[0][0];
	BOOST_LOG_TRIVIAL(info) << "                          " << mTransform[0][0] << ", " <<  mTransform[0][0] << ", " << mTransform[0][0];
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