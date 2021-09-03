#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include "e57.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#if defined(WIN32)
#include <Windows.h>
#endif

#include "e57/E57Foundation.h"
#include "e57/E57Simple.h"

struct E57
{
	int64_t nColumn;    
	int64_t nSize;
	int64_t nPointsSize;        //Number of points
	int64_t nGroupsSize;        //Number of groups
	int64_t nCountSize;         //Number of points per group
	bool bColumnIndex; //indicates that idElementName is "columnIndex"

	int8_t* isInvalidData;
	double* xData;
	double* yData;
	double* zData;
	double* intData;
	bool bIntensity;
	double intRange;
	double intOffset;
	uint16_t* redData;
	uint16_t* greenData;
	uint16_t* blueData;
	bool bColor;
	int32_t colorRedRange;
	int32_t colorRedOffset;
	int32_t colorGreenRange;
	int32_t colorGreenOffset;
	int32_t colorBlueRange;
	int32_t colorBlueOffset;
	int64_t* idElementValue;
	int64_t* startPointIndex;
	int64_t* pointCount;
	int8_t* rowIndex;
	int32_t* columnIndex;

	int mIndex;

	int mIntensityAttribute;
	int mColorAttribute;

	glm::dmat4 mWorldPose;

	E57(e57::Reader& iReader, int iIndex)
	: nColumn(0)
	, nSize(0)
	, nPointsSize(0)
	, nGroupsSize(0)
	, nCountSize(0)
	, bColumnIndex(false)
	, isInvalidData(NULL)
	, xData(NULL)
	, yData(NULL)
	, zData(NULL)
	, intData(NULL)
	, bIntensity(false)
	, intRange(0)
	, intOffset(0)
	, redData(NULL)
	, greenData(NULL)
	, blueData(NULL)
	, bColor(false)
	, colorRedRange(1)
	, colorRedOffset(0)
	, colorGreenRange(1)
	, colorGreenOffset(0)
	, colorBlueRange(1)
	, colorBlueOffset(0)
	, idElementValue(NULL)
	, startPointIndex(NULL)
	, pointCount(NULL)
	, rowIndex(NULL)
	, columnIndex(NULL)
	, mIndex(iIndex)
	{
		iReader.ReadData3D(iIndex, mHeader);
		iReader.GetData3DSizes(iIndex, nSize, nColumn, nPointsSize, nGroupsSize, nCountSize, bColumnIndex);

		/*
		glm::quat lQuat = glm::quat(mHeader.pose.rotation.w, mHeader.pose.rotation.x, mHeader.pose.rotation.y, mHeader.pose.rotation.z);
		glm::vec3 lAxis = glm::axis(lQuat);
		std::cout << "AXI: " << lAxis.x << " " <<lAxis.y << " " << lAxis.z << "   " << glm::angle(lQuat) << "\n";
		std::cout << "TRS: " << mHeader.pose.translation.x << " " <<mHeader.pose.translation.y << " " << mHeader.pose.translation.z <<"\n";
		*/
		glm::dmat4 lRotationMatrix = glm::mat4_cast(glm::dquat(mHeader.pose.rotation.w, mHeader.pose.rotation.x, mHeader.pose.rotation.y, mHeader.pose.rotation.z));
		glm::dmat4 lTranslationMatrix = glm::translate(glm::dmat4(1.0f), glm::dvec3(mHeader.pose.translation.x, mHeader.pose.translation.y, mHeader.pose.translation.z));
		mWorldPose = lTranslationMatrix * lRotationMatrix;

		if (nSize == 0) nSize = 1024;

		xData = new double[nSize];
		yData = new double[nSize];
		zData = new double[nSize];
		if (mHeader.pointFields.cartesianInvalidStateField)
		{
			isInvalidData = new int8_t[nSize];
		}
		if (mHeader.pointFields.intensityField)
		{
			bIntensity = true;
			intData = new double[nSize];
			intRange = mHeader.intensityLimits.intensityMaximum - mHeader.intensityLimits.intensityMinimum;
			intOffset = mHeader.intensityLimits.intensityMinimum;
		}

		if (mHeader.pointFields.colorRedField)
		{
			bColor = true;
			redData = new uint16_t[nSize];
			greenData = new uint16_t[nSize];
			blueData = new uint16_t[nSize];
			colorRedRange = mHeader.colorLimits.colorRedMaximum - mHeader.colorLimits.colorRedMinimum;
			colorRedOffset = mHeader.colorLimits.colorRedMinimum;
			colorGreenRange = mHeader.colorLimits.colorGreenMaximum - mHeader.colorLimits.colorGreenMinimum;
			colorGreenOffset = mHeader.colorLimits.colorGreenMinimum;
			colorBlueRange = mHeader.colorLimits.colorBlueMaximum - mHeader.colorLimits.colorBlueMinimum;
			colorBlueOffset = mHeader.colorLimits.colorBlueMinimum;
		}

		if(nGroupsSize > 0)
		{
			idElementValue = new int64_t[nGroupsSize];
			startPointIndex = new int64_t[nGroupsSize];
			pointCount = new int64_t[nGroupsSize];

			if (!iReader.ReadData3DGroupsData(iIndex, nGroupsSize, idElementValue, startPointIndex, pointCount))
			{
				nGroupsSize = 0;
			}
		}

		if(mHeader.pointFields.rowIndexField)
		{
			rowIndex = new int8_t[nSize];
		}

		if(mHeader.pointFields.columnIndexField)
		{
			columnIndex = new int32_t[nSize];
		}
	}

	
	~E57()
	{
		if(isInvalidData) delete isInvalidData;
		if(xData) delete xData;
		if(yData) delete yData;
		if(zData) delete zData;
		if(intData) delete intData;
		if(redData) delete redData;
		if(greenData) delete greenData;
		if(blueData) delete blueData;
	}

	unsigned long read(Point& iPoint, e57::Reader& iReader, FILE* iOutputFile, CloudImporter& iImporter)
	{
		e57::CompressedVectorReader lReader = start(iReader);

		unsigned long lPointCount = 0;

		unsigned size = 0;
		while (size = lReader.read())
		{
			for (unsigned long i = 0; i < size; i++)
			{
				if (!isInvalidData || !isInvalidData[i])
				{
					glm::dvec4 lScanPosition = glm::dvec4(xData[i], yData[i], zData[i], 1.0);
					//if (lScanPosition.x*lScanPosition.x + lScanPosition.y*lScanPosition.y + lScanPosition.z*lScanPosition.z < iCutoff2)
					{
						glm::dvec4 lWorldPostion = mWorldPose*lScanPosition;
						iImporter.convertCoords(&lWorldPostion[0]);
						iImporter.growMinMax(&lWorldPostion[0]);

						iPoint.position[0] = lWorldPostion[0];
						iPoint.position[1] = lWorldPostion[1];
						iPoint.position[2] = lWorldPostion[2];

						if (bColor)
						{             
							ColorType& lColor = *(ColorType*)iPoint.getAttribute(mColorAttribute);

							//Normalize color to 0 - 255
							lColor.mValue[0] = ((redData[i] - colorRedOffset) * 255)/colorRedRange;
							lColor.mValue[1] = ((greenData[i] - colorGreenOffset) * 255)/colorBlueRange;
							lColor.mValue[2] = ((blueData[i] - colorBlueOffset) * 255)/colorBlueRange;

							//iImporter.recordColor(lColor.mValue[0],lColor.mValue[1],lColor.mValue[2]);
						}

						if (bIntensity)
						{         
							IntensityType& lIntensityAttribute = *(IntensityType*)iPoint.getAttribute(mIntensityAttribute);

							double lIntensity = (intData[i] - intOffset)/intRange; //Normalize intensity to 0 - 1.
							lIntensityAttribute.mValue = lIntensity*USHRT_MAX;
						}

						iPoint.write(iOutputFile);
						lPointCount++;
					}
				}
			}
		}

		lReader.close();

		return lPointCount;
	}

	e57::CompressedVectorReader start(e57::Reader& iReader)
	{
		return iReader.SetUpData3DPointsData(
								mIndex,                      //!< data block index given by the NewData3D
								nSize,                           //!< size of each of the buffers given
								xData,                          //!< pointer to a buffer with the x data
								yData,                          //!< pointer to a buffer with the y data
								zData,                          //!< pointer to a buffer with the z data
								isInvalidData,          //!< pointer to a buffer with the valid indication
								intData,                        //!< pointer to a buffer with the lidar return intesity
								NULL,
								redData,                        //!< pointer to a buffer with the color red data
								greenData,                      //!< pointer to a buffer with the color green data
								blueData,                       //!< pointer to a buffer with the color blue data
								NULL,
								NULL,
								NULL,
								NULL,
								rowIndex,                       //!< pointer to a buffer with the rowIndex
								columnIndex                     //!< pointer to a buffer with the columnIndex
								);
	}

	e57::Data3D mHeader;
};







E57Importer::E57Importer(uint8_t iCoords, float iScalar, float iResolution)
: CloudImporter(iCoords, iScalar, iResolution)
{
};


json_spirit::mObject E57Importer::import (std::string iName, glm::dvec3* iCenter)
{
	uint64_t lPointCount = 0;
	FILE* lOutputFile = 0;
	PointCloudAttributes lAttributes;

    try
    {
		e57::Reader eReader(iName);
		e57::E57Root rootHeader;
		eReader.GetE57Root(rootHeader);

		std::vector<E57*> lList;
		int32_t lScanCount = eReader.GetData3DCount();
		for (int32_t i=0; i<lScanCount; i++)
		{
			E57* lScan = new E57(eReader, i);
			lList.push_back(lScan);
			if (lScan->bColor)
			{
				lScan->mColorAttribute = lAttributes.createAttribute(Attribute::COLOR, COLOR_TEMPLATE);
			}
			if (lScan->bIntensity)
			{
				lScan->mIntensityAttribute = lAttributes.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE);
			}

			if (!iCenter)
			{
				mCenter[0] += lScan->mWorldPose[3][0];
				mCenter[1] += lScan->mWorldPose[3][1];
				mCenter[2] += lScan->mWorldPose[3][2];
			}
			lPointCount += lScan->nPointsSize;
		}

		if (!iCenter)
		{
			mCenter[0] /= lScanCount;
			mCenter[1] /= lScanCount;
			mCenter[2] /= lScanCount;
		}
		else
		{
			mCenter[0] = (*iCenter)[0];
			mCenter[1] = (*iCenter)[1];
			mCenter[2] = (*iCenter)[2];
		}

		for (std::vector<E57*>::iterator lIter = lList.begin(); lIter != lList.end(); lIter++)
		{
			 (*lIter)->mWorldPose[3][0] -= mCenter[0];
			 (*lIter)->mWorldPose[3][1] -= mCenter[1];
			 (*lIter)->mWorldPose[3][2] -= mCenter[2];
		}
		convertCoords(mCenter);
	
		// write ply
		//std::cout << " BEFORE " << lPointCount << "\n";
		Point lPoint(lAttributes);
		lOutputFile = PointCloud::writeHeader("cloud", lAttributes,0,0,0,mResolution);
		lPointCount = 0;

		for (std::vector<E57*>::iterator lIter = lList.begin(); lIter != lList.end(); lIter++)
		{
			BOOST_LOG_TRIVIAL(info) << "File - " << (*lIter)->mHeader.name << "  :   " << (*lIter)->nPointsSize;
			lPointCount += (*lIter)->read(lPoint, eReader, lOutputFile, *this);
		}

	} catch (e57::E57Exception& ex) {
		BOOST_LOG_TRIVIAL(info) << "Got an e57::E57Exception, what=" << ex.what() << " file " << ex.sourceFileName() << " line " << ex.sourceLineNumber() <<  " context " << ex.context() << endl;
	} catch (std::exception& ex) {
		BOOST_LOG_TRIVIAL(info) << "Got an std::exception, what=" << ex.what() << endl;
	} catch (...) {
		BOOST_LOG_TRIVIAL(info) << "Got an unknown exception" << endl;
	}

	//std::cout << " AFTER " << lPointCount << "\n";
	if (lOutputFile)
	{
		PointCloud::updateSize(lOutputFile, lPointCount);
		PointCloud::updateSpatialBounds(lOutputFile, mMinD, mMaxD);
		fclose(lOutputFile);
	}

	return getMeta(0);
}
