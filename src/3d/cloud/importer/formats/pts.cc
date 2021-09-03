#include "pts.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

PtsImporter::PtsImporter(uint8_t iCoords, float iScalar, float iResolution)
: CloudImporter(iCoords, iScalar, iResolution)
{
};

json_spirit::mObject PtsImporter::import(std::string iName, glm::dvec3* iCenter)
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
		if (!iCenter)
		{
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

			mCenter[0] = (mMinD[0] + mMaxD[0])/2;
			mCenter[1] = (mMinD[1] + mMaxD[1])/2;
			mCenter[2] = (mMinD[2] + mMaxD[2])/2;

			center(mMinD);
			center(mMaxD);
		}
		else
		{
			mCenter[0] = (*iCenter)[0];
			mCenter[1] = (*iCenter)[1];
			mCenter[2] = (*iCenter)[2];
			convertCoords(mCenter);
		}

		BOOST_LOG_TRIVIAL(info) << "Main pass";

		// Main pass		
		Point lPoint(lCloud);

		uint64_t lTotal = 0;
		FILE* lOutputFile = PointCloud::writeHeader("cloud", lCloud, 0, 0, 0, mResolution);
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

			center(lCoords);
			if (iCenter)
			{
				growMinMax(lPoint.position);
			}

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





