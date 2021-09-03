#include "ptx.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

PtxImporter::PtxImporter(uint8_t iCoords, float iScalar, float iResolution)
: CloudImporter(iCoords, iScalar, iResolution)
{
};

uint64_t PtxImporter::readHeader(FILE* iFile, char* iLine, glm::dmat4& iMatrix)
{
	long lColumns;
	sscanf(iLine, "%ld", &lColumns);
	//std::cout << iLine << "\n"; 

	fgets(iLine, 256, iFile);

	long lRows;
	sscanf(iLine, "%ld", &lRows);
	//std::cout << iLine << "\n"; 

	// skip the scanner location matrix
	for (int i=0; i<4; i++)
	{
		fgets(iLine, 256, iFile);
	//	std::cout << iLine << "\n"; 
	}

	/*
	fgets(iLine, 256, iFile);
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[0][0], &iMatrix[1][0], &iMatrix[2][0], &iMatrix[3][0]);
	fgets(iLine, 256, iFile);
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[0][1], &iMatrix[1][1], &iMatrix[2][1], &iMatrix[3][1]);
	fgets(iLine, 256, iFile);
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[0][2], &iMatrix[1][2], &iMatrix[2][2], &iMatrix[3][2]);
	fgets(iLine, 256, iFile);
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[0][3], &iMatrix[1][3], &iMatrix[2][3], &iMatrix[3][3]);
	*/

		//read cloud transformation matrix
	/*
			for (int i=0; i<4; ++i)
			{
				line = inFile.readLine();
				tokens = line.split(" ",QString::SkipEmptyParts);
				if (tokens.size() != 4)
					return CC_FERR_MALFORMED_FILE;

				float* col = cloudTrans.getColumn(i);
				for (int j=0; j<4; ++j)
				{
					col[j] = tokens[j].toFloat(&ok);
					if (!ok)
						return CC_FERR_MALFORMED_FILE;
				}
			}
	*/

	fgets(iLine, 256, iFile);
	//std::cout << iLine << "\n"; 
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[0][0], &iMatrix[0][1], &iMatrix[0][2], &iMatrix[0][3]);
	fgets(iLine, 256, iFile);
	//std::cout << iLine << "\n"; 
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[1][0], &iMatrix[1][1], &iMatrix[1][2], &iMatrix[1][3]);
	fgets(iLine, 256, iFile);
	//std::cout << iLine << "\n"; 
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[2][0], &iMatrix[2][1], &iMatrix[2][2], &iMatrix[2][3]);
	fgets(iLine, 256, iFile);
	//std::cout << iLine << "\n"; 
	sscanf(iLine, "%lf %lf %lf %lf", &iMatrix[3][0], &iMatrix[3][1], &iMatrix[3][2], &iMatrix[3][3]);


	return lColumns*lRows;
}

json_spirit::mObject PtxImporter::import(std::string iName, glm::dvec3* iCenter)
{
	uint64_t lTotalCount = 0;
    try
    {
		PointCloudAttributes lAttributes;

		FILE* lInputFile = fopen(iName.c_str(),"rb");
		glm::dmat4 lMatrix(1.0);
		char lLine[1024];
		// determine attributes
		fgets(lLine,1024, lInputFile);
		uint64_t lPointCount = readHeader(lInputFile, lLine, lMatrix);
		fgets(lLine,1024, lInputFile);
		float lCoords[3];
		float lIntensity;
		char lColor[3];
		switch (sscanf(lLine, "%f %f %f %f %hhu %hhu %hhu", lCoords+0, lCoords+1, lCoords+2, &lIntensity, lColor+0, lColor+1, lColor+2))
		{
			case 4: 
				lAttributes.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE);
				break;
			case 7: 
				lAttributes.createAttribute(Attribute::COLOR, COLOR_TEMPLATE);
				lAttributes.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE);
				break;
		};

		int lIntensityIndex = lAttributes.getAttributeIndex(Attribute::INTENSITY);
		int lColorIndex = lAttributes.getAttributeIndex(Attribute::COLOR);

		// AABB pass
		if (!iCenter)
		{
			BOOST_LOG_TRIVIAL(info) << "AABB pass  ";
			fseek(lInputFile, 0, SEEK_SET);
			while (fgets(lLine, 256, lInputFile))
			{
				glm::dvec4 lCoords;
		
				lCoords[3] = 1;
				uint64_t lPointCount = readHeader(lInputFile, lLine, lMatrix);
				for (uint64_t i=0; i<lPointCount; i++)
				{
					if (fgets(lLine,1024,lInputFile)) 
					{
						double lX, lY, lZ;
						//sscanf(lLine, "%lf %lf %lf", &lCoords[0], &lCoords[1], &lCoords[2]);
						sscanf(lLine, "%lf %lf %lf", &lX, &lY, &lZ);
						if (lX != 0 && lY != 0 && lZ != 0)
						{
							lCoords[0] = lX;
							lCoords[1] = lY;
							lCoords[2] = lZ;
							lCoords= lMatrix*lCoords;
							convertCoords(&lCoords[0]);
							growMinMax(&lCoords[0]);
						}
					}

					if (i%10000000 == 0)
					{
						BOOST_LOG_TRIVIAL(info) << "AABB pass at " << i << " points";
					}
				}
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
		Point lPoint(lAttributes);

		FILE* lOutputFile = PointCloud::writeHeader("cloud", lAttributes, 0, 0, 0, mResolution);
		IntensityType* lIntensityAttribute = (IntensityType*)lPoint.getAttribute(lIntensityIndex);
		ColorType* lColorAttribute = (ColorType*)lPoint.getAttribute(lColorIndex);

		BOOST_LOG_TRIVIAL(info) << "AABB pass";
		fseek(lInputFile, 0, SEEK_SET);
		while (fgets(lLine, 256, lInputFile))
		{
			glm::dvec4 lCoords;
			lCoords[3] = 1;
			uint64_t lPointCount = readHeader(lInputFile, lLine, lMatrix);
			for (uint64_t i=0; i<lPointCount; i++)
			{
				if (fgets(lLine,1024,lInputFile)) 
				{
					float lIntensity;
					char lColor[3];
					double lX, lY, lZ;

					if (lColorIndex != -1)
					{
						sscanf(lLine, "%lf %lf %lf %f %hhu %hhu %hhu", &lX, &lY, &lZ, &lIntensity, lColor+0, lColor+1, lColor+2);
					}
					else
					{
						sscanf(lLine, "%lf %lf %lf %f", &lX, &lY, &lZ, &lIntensity);
					}

					if (lX != 0 && lY != 0 && lZ != 0)
					{
						lCoords[0] = lX;
						lCoords[1] = lY;
						lCoords[2] = lZ;
						lCoords = lMatrix*lCoords;
						convertCoords(&lCoords[0]);

						center(&lCoords[0]);
						if (iCenter)
						{
							growMinMax(lPoint.position);
						}

						// assign to point
						if (lIntensityAttribute)
						{
							lIntensityAttribute->mValue = lIntensity*USHRT_MAX;
						}
						if (lColorAttribute)
						{
							memcpy(lColorAttribute->mValue, lColor, sizeof(lColor));
						}
						lPoint.position[0] = lCoords[0];
						lPoint.position[1] = lCoords[1];
						lPoint.position[2] = lCoords[2];
						lPoint.write(lOutputFile);
						lTotalCount ++;
					}
				}

				if (i%10000000 == 0)
				{
					BOOST_LOG_TRIVIAL(info) << "Main pass at " << i << " points";
				}
			}
		}

		PointCloud::updateSize(lOutputFile, lTotalCount);
		PointCloud::updateSpatialBounds(lOutputFile, mMinD, mMaxD);
		fclose(lOutputFile);
		fclose(lInputFile);

	} catch (...) {
		BOOST_LOG_TRIVIAL(info) << "Got an unknown exception";
	}

	return getMeta(0);
}



