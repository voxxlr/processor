#include "pts.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

PtsImporter::PtsImporter(json_spirit::mObject& iConfig)
: CloudImporter(iConfig)
{
};

json_spirit::mObject PtsImporter::import(std::string iName)
{
	uint64_t lPointCount = 0;
    try
    {
		FILE* lInputFile = fopen(iName.c_str(),"rb");

		PointCloudAttributes lCloud;
		int lIntensityIndex = -1;
		int lColorIndex = -1;

		// determine point attributes by number of elements on each data line
		char lLine[1024];
		fgets(lLine, 256, lInputFile); // just skip first line ..
		fgets(lLine,1024, lInputFile);
		float lCoords[3];
		int lIntensity;
		char lColor[3];
		switch (sscanf(lLine, "%f %f %f %d %hhu %hhu %hhu", lCoords+0, lCoords+1, lCoords+2, &lIntensity, lColor+0, lColor+1, lColor+2))
		{
			case 4: 
				lIntensityIndex = lCloud.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE);
				break;
			case 6:
				lColorIndex = lCloud.createAttribute(Attribute::COLOR, COLOR_TEMPLATE);
				break;
			case 7: 
				lColorIndex = lCloud.createAttribute(Attribute::COLOR, COLOR_TEMPLATE);
				lIntensityIndex = lCloud.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE);
				break;
		};


		// aabb pass
		BOOST_LOG_TRIVIAL(info) << "AABB pass  ";
		// AABB pass
		fseek(lInputFile, 0, SEEK_SET);
		fgets(lLine, 256, lInputFile); // just skip first line ..
		while (fgets(lLine, 256, lInputFile))
		{
			int lIntensity;
			double lCoords[3];
			sscanf(lLine, "%lf %lf %lf", lCoords+0, lCoords+1, lCoords+2);
			convertCoords(lCoords);
			growMinMax(lCoords);
		}

		glm::dvec3 lCenter;
		lCenter[0] = (mMinD[0] + mMaxD[0])/2;
		lCenter[1] = (mMinD[1] + mMaxD[1])/2;
		lCenter[2] = (mMinD[2] + mMaxD[2])/2;

		mMinD[0] -= lCenter[0];
		mMinD[1] -= lCenter[1];
		mMinD[2] -= lCenter[2];
		mMaxD[0] -= lCenter[0];
		mMaxD[1] -= lCenter[1];
		mMaxD[2] -= lCenter[2];

		BOOST_LOG_TRIVIAL(info) << "Main pass";

		// Main pass		
		Point lPoint(lCloud);

		uint64_t lTotal = 0;
		FILE* lOutputFile = PointCloud::writeHeader("cloud", lCloud);
		IntensityType* lIntensityAttribute = (IntensityType*)lPoint.getAttribute(lIntensityIndex);
		ColorType* lColorAttribute = (ColorType*)lPoint.getAttribute(lColorIndex);

		fseek(lInputFile, 0, SEEK_SET);
		fgets(lLine, 256, lInputFile); // just skip first line ..
		while (fgets(lLine, 256, lInputFile))
		{
			int lIntensity;
			char lColor[3];
			double lCoords[3];
			sscanf(lLine, "%lf %lf %lf %d %hhu %hhu %hhu", lCoords+0, lCoords+1, lCoords+2, &lIntensity, lColor+0, lColor+1, lColor+2);

			convertCoords(lCoords);
			lCoords[0] -= lCenter[0];
			lCoords[1] -= lCenter[1];
			lCoords[2] -= lCenter[2];

			// assign to point
			if (lIntensityAttribute)
			{
				lIntensityAttribute->mValue = ((lIntensity + 2048.0)/4096.0)*USHRT_MAX;
			}
			if (lColorAttribute)
			{
				memcpy(lColorAttribute->mValue, lColor, sizeof(lColor));
				//recordColor(lColorAttribute->mValue[0], lColorAttribute->mValue[1], lColorAttribute->mValue[2]);
			}
			lPoint.position[0] = lCoords[0];
			lPoint.position[1] = lCoords[1];
			lPoint.position[2] = lCoords[2];
			lPoint.write(lOutputFile);
	
			if (lPointCount%10000000 == 0)
			{
				BOOST_LOG_TRIVIAL(info) << "Main pass at " << lPointCount << " points";
			}

			lPointCount ++;
		}

		PointCloud::updateSpatialBounds(lOutputFile, mMinD, mMaxD);
		PointCloud::updateSize(lOutputFile, lTotal);
		fclose(lOutputFile);
		fclose(lInputFile);

	} catch (...) {
		BOOST_LOG_TRIVIAL(info) << "Got an unknown exception";
	}

	return getMeta(0);
}





