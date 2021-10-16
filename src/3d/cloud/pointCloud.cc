#include <inttypes.h> 

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include "pointCloud.h"

//
// Attribute API
// 
PointCloudAttributes::PointCloudAttributes()
: mByteCount(0)
{
}

PointCloudAttributes::PointCloudAttributes(PointCloudAttributes& iAttributes)
: mByteCount(0)
{
	for (size_t i=0; i< iAttributes.mList.size(); i++)
	{
		for (std::map<std::string, int>::iterator lIter = iAttributes.mMap.begin() ; lIter != iAttributes.mMap.end(); ++lIter)
		{
			if (lIter->second == i)
			{
				createAttribute(lIter->first, *iAttributes.mList[i]);
			}
		}
	}
}

PointCloudAttributes::~PointCloudAttributes()
{
	for (std::vector<Attribute*>::iterator lIter = mList.begin() ; lIter != mList.end(); ++lIter)
	{
		delete *lIter;
	}
}

void PointCloudAttributes::load(json_spirit::mArray iArray)
{
	for (std::vector<json_spirit::mValue>::iterator lIter = iArray.begin(); lIter != iArray.end(); ++lIter)
	{
		json_spirit::mObject lObject = (*lIter).get_obj();

		if (lObject["name"].get_str() == Attribute::NORMAL)
		{
			if (lObject["type"].get_str() == SingleAttribute<float>::cTypeName)
			{
				NormalType lAttribute;
				createAttribute(Attribute::NORMAL, lAttribute);
			}
			else if (lObject["type"].get_str() == SingleAttribute<Vec3IntPacked>::cTypeName)
			{
				PackedNormalType lAttribute;
				createAttribute(Attribute::NORMAL, lAttribute);
			}
		}
		else if (lObject["name"].get_str() == Attribute::COLOR)
		{
			ArrayAttribute<uint8_t, 3> lAttribute;
			createAttribute(Attribute::COLOR, lAttribute);
		}
		else if (lObject["name"].get_str() == Attribute::CLASS)
		{
			SingleAttribute<uint8_t> lAttribute;
			createAttribute(Attribute::CLASS, lAttribute);
		}
		else if (lObject["name"].get_str() == Attribute::INTENSITY)
		{
			SingleAttribute<uint16_t> lAttribute;
			createAttribute(Attribute::INTENSITY, lAttribute);
		}
	}
}


int PointCloudAttributes::createAttribute(const std::string& iName, Attribute& iAttribute)
{
	std::map<std::string, int>::iterator lIter = mMap.find(iName);
	if (lIter == mMap.end())
	{
		Attribute* lAttribute = iAttribute.clone();
		mList.push_back(lAttribute);
		int lIndex = mList.size() - 1;
		mMap[iName] = lIndex;

		if (iName == Attribute::COLOR)
		{
			mAverage.push_back(lIndex);
		}
		else if (iName == Attribute::INTENSITY)
		{
			mAverage.push_back(lIndex);
		}

		mByteCount += lAttribute->bytesPerPoint();
		return lIndex;
	}
	else
	{
		return lIter->second;
	}
}


void PointCloudAttributes::readHeader(char* iName)
{
	if (!strcmp(iName, "nx") || !strcmp(iName, "ny")|| !strcmp(iName, "nz"))
	{
		NormalType lAttribute;
		createAttribute(Attribute::NORMAL, lAttribute);
	}
	else if (!strcmp(iName, "np"))
	{
		PackedNormalType lAttribute;
		createAttribute(Attribute::NORMAL, lAttribute);
	}
	else if (!strcmp(iName, "red") || !strcmp(iName, "green") || !strcmp(iName, "blue"))
	{
		ColorType lAttribute;
		createAttribute(Attribute::COLOR, lAttribute);
	}
	else if (!strcmp(iName, "r") || !strcmp(iName, "g") || !strcmp(iName, "b"))
	{
		ColorType lAttribute;
		createAttribute(Attribute::COLOR, lAttribute);
	}
	else if (!strcmp(iName, "class"))	
	{
		ClassType lAttribute;
		createAttribute(Attribute::CLASS, lAttribute);
	}
	else if (!strcmp(iName, "intensity"))
	{
		IntensityType lAttribute;
		createAttribute(Attribute::INTENSITY, lAttribute);
	}
}


void PointCloudAttributes::writeHeader(FILE* iFile)
{
	for (size_t i=0; i<mList.size(); i++)
	{
		for (std::map<std::string, int>::iterator lIter1 = mMap.begin() ; lIter1 != mMap.end(); ++lIter1)
		{
			if (lIter1->second == i)
			{
				if (lIter1->first == Attribute::COLOR)
				{
					fprintf(iFile, "property uchar red\n"
								   "property uchar green\n"
								   "property uchar blue\n");
				}
				else if (lIter1->first == Attribute::NORMAL)
				{
					Attribute* lAttribute = mList[lIter1->second];
					if (dynamic_cast<NormalType*>(lAttribute))
					{
						fprintf(iFile,   "property float nx\n"
										 "property float ny\n"
										 "property float nz\n");
					}
					else if (dynamic_cast<PackedNormalType*>(lAttribute))
					{
						fprintf(iFile,   "property int np\n");
					}
				}
				else if (lIter1->first == Attribute::CLASS)
				{
					fprintf(iFile, "property uchar class\n");
				}
				else if (lIter1->first == Attribute::INTENSITY)
				{
					fprintf(iFile, "property ushort intensity\n");
				}
			} 
		}
	}
}



void PointCloudAttributes::toJson(json_spirit::mArray& iArray)
{
	// make sure to push in the same order they were created
	for (size_t i=0; i<mList.size(); i++)
	{
		for (std::map<std::string, int>::iterator lIter = mMap.begin() ; lIter != mMap.end(); ++lIter)
		{
			if (lIter->second == i)
			{
				Attribute* lAttribute = mList[lIter->second];
				json_spirit::mObject lObject;
				lObject["name"] = lIter->first;
				lAttribute->toJson(lObject);
				iArray.push_back(lObject);
			}
		}
	}
}

void PointCloudAttributes::fromJson(json_spirit::mArray& iArray)
{
	for (std::vector<json_spirit::mValue>::iterator lIter = iArray.begin() ; lIter != iArray.end(); ++lIter)
	{
		json_spirit::mObject lObject = (*lIter).get_obj();

		if (lObject["name"].get_str() == Attribute::NORMAL)
		{
			if (lObject["type"].get_str() == SingleAttribute<float>::cTypeName)
			{
				NormalType lAttribute;
				createAttribute(Attribute::NORMAL, lAttribute);
			}
			else if (lObject["type"].get_str() == SingleAttribute<Vec3IntPacked>::cTypeName)
			{
				PackedNormalType lAttribute;
				createAttribute(Attribute::NORMAL, lAttribute);
			}
		}
		else if (lObject["name"].get_str() == Attribute::COLOR)
		{
			ArrayAttribute<uint8_t, 3> lAttribute;
			createAttribute(Attribute::COLOR, lAttribute);
		}
		else if (lObject["name"].get_str() == Attribute::CLASS)
		{
			SingleAttribute<uint8_t> lAttribute;
			createAttribute(Attribute::CLASS, lAttribute);
		}
		else if (lObject["name"].get_str() == Attribute::INTENSITY)
		{
			SingleAttribute<uint16_t> lAttribute;
			createAttribute(Attribute::INTENSITY, lAttribute);
		}
	}
}

void PointCloudAttributes::addAttributes(PointCloudAttributes& iAttrbutes, std::vector<std::tuple<std::string, Attribute*>>& iAdded)
{
	for (std::map<std::string, int>::iterator lIter = iAttrbutes.mMap.begin() ; lIter != iAttrbutes.mMap.end(); ++lIter)
	{
		if (mMap.find(lIter->first) == mMap.end())
		{
			iAdded.push_back(std::make_tuple(lIter->first, iAttrbutes.mList[lIter->second]));
		} 
	}

	std::vector<std::tuple<std::string, Attribute*>> lMissing;
	for (std::vector<std::tuple<std::string, Attribute*>>::iterator lIter = iAdded.begin(); lIter != iAdded.end(); lIter++)
	{
		createAttribute(std::get<0>(*lIter), *std::get<1>(*lIter));
	}

};


bool PointCloudAttributes::hasAttribute(const std::string& iName)
{
	return mMap.find(iName) != mMap.end();
};

void PointCloudAttributes::average(Point& iDest, Point& iSrce)
{
	for (std::vector<int>::iterator lIter = mAverage.begin(); lIter != mAverage.end(); lIter++)
	{
		Attribute* lDest = iDest.getAttribute(*lIter);
		Attribute* lSrce = iSrce.getAttribute(*lIter);
		lDest->average(*lSrce, iDest.mWeight);
	}
};

int PointCloudAttributes::getAttributeIndex(const std::string& iName)
{
	std::map<std::string, int>::iterator lIter = mMap.find(iName);
	if (lIter != mMap.end())
	{
		return lIter->second;
	}
	return -1;
};


float PointCloud::MIN[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
float PointCloud::MAX[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

PointCloud::PointCloud()
: mPointCount(0)
, mResolution(0) // undefined
{
	memcpy(mMinExtent, MIN, sizeof(MIN));
	memcpy(mMaxExtent, MAX, sizeof(MAX));
};

PointCloud::PointCloud(PointCloudAttributes& iAttributes)
: PointCloudAttributes(iAttributes)
{
};

PointCloud::~PointCloud()
{
	for (std::vector<Point*>::iterator lIter = mPoints.begin() ; lIter != mPoints.end(); ++lIter)
	{
		Point::destroy(*lIter);
	}
};



//
// Attributes
//

void PointCloud::addAttributes(PointCloudAttributes& iPointCloud)
{
	std::vector<std::tuple<std::string, Attribute*>> lAdded;

	PointCloudAttributes::addAttributes(iPointCloud, lAdded);
	for (std::vector<Point*>::iterator lIter0 = mPoints.begin() ; lIter0 != mPoints.end(); ++lIter0)
	{
		for (std::vector<std::tuple<std::string, Attribute*>>::iterator lIter1 = lAdded.begin(); lIter1 != lAdded.end(); lIter1++)
		{
			(*lIter0)->addAttribute(*std::get<1>(*lIter1));
		}
	}
};

Attribute* PointCloud::getAttribute(Point& iPoint, const std::string& iName)
{
	int lIndex = getAttributeIndex(iName);
	if (lIndex != -1)
	{
		return iPoint.getAttribute(lIndex);
	}
	return 0;
};

//
// Reading
//

FILE* PointCloud::readHeader(std::string iName, PointCloudAttributes* iAttributes, uint64_t& iSize, float* iMin, float* iMax, float* iResolution)
{
	std::string lName = iName + ".ply";
	FILE* lFile = fopen(lName.c_str(), "rb");
	char lLine[1024];

	while (fgets(lLine, 1024, lFile))
	{
		char lBuffer[5][64];
		sscanf(lLine, "%s %s %s %s %s", (char*)&lBuffer[0], (char*)&lBuffer[1], (char*)&lBuffer[2], (char*)&lBuffer[3], (char*)&lBuffer[4]);
		if (!strcmp(lBuffer[0], "property"))
		{
			if (iAttributes)
			{
				iAttributes->readHeader(lBuffer[2]);
			}
		}
		else if (!strcmp(lBuffer[0], "comment"))
		{
			if (!strcmp(lBuffer[1], "minx"))
			{
				if (iMin)
				{
					sscanf(lBuffer[2], "%f", &iMin[0]);
				}
			}
			else if (!strcmp(lBuffer[1], "miny"))
			{
				if (iMin)
				{
					sscanf(lBuffer[2], "%f", &iMin[1]);
				}
			}
			else if (!strcmp(lBuffer[1], "minz"))
			{
				if (iMin)
				{
					sscanf(lBuffer[2], "%f", &iMin[2]);
				}
			}
			else if (!strcmp(lBuffer[1], "maxx"))
			{
				if (iMax)
				{
					sscanf(lBuffer[2], "%f", &iMax[0]);
				}
			}
			else if (!strcmp(lBuffer[1], "maxy"))
			{
				if (iMax)
				{
					sscanf(lBuffer[2], "%f", &iMax[1]);
				}
			}
			else if (!strcmp(lBuffer[1], "maxz"))
			{
				if (iMax)
				{
					sscanf(lBuffer[2], "%f", &iMax[2]);
				}
			}
			else if (!strcmp(lBuffer[1], "resolution"))
			{
				if (iResolution)
				{
					sscanf(lBuffer[2], "%f", iResolution);
				}
			}
		}
		else if (!strcmp(lBuffer[0], "element"))
		{
			std::sscanf(lBuffer[2], "%" SCNu64, &iSize);
		}
		else if (!strncmp(lLine, "end_header", 10))
		{
			break;
		}
	}
	return lFile;
}


void PointCloud::readFile(std::string& iName)
{
	boost::posix_time::ptime t1 = boost::posix_time::second_clock::local_time();

	FILE* lFile = readHeader(iName, this, mPointCount, mMinExtent, mMaxExtent, &mResolution);

	mPoints.reserve(mPointCount);
	for (uint32_t i=0; i<mPointCount; i++)
	{
		Point* lPoint = Point::create(*this);
		fread(lPoint->position, sizeof(lPoint->position), 1, lFile);

		std::vector<Attribute*>& lAttributes = lPoint->getAttributes();
		for (std::vector<Attribute*>::iterator lIter = lAttributes.begin() ; lIter != lAttributes.end(); ++lIter)
		{
			(*lIter)->read(lFile);
		}

		mPoints.push_back(lPoint);
	}

	fclose(lFile);

	boost::posix_time::ptime t2 = boost::posix_time::second_clock::local_time();
	boost::posix_time::time_duration diff = t2 - t1;
	BOOST_LOG_TRIVIAL(info) << "Read " << iName << " with " << mPointCount << " points at " << bytesPerPoint() << " bytes/point in " << diff.total_milliseconds() / 1000 << " seconds";
}


//
// Writing
//

void PointCloud::writeFile(std::string& iName)
{
	// write points back to file
	FILE* lFile = writeHeader(iName, *this, mPoints.size(), mMinExtent, mMaxExtent, mResolution);
	for (std::vector<Point*>::iterator lIter = mPoints.begin(); lIter !=  mPoints.end(); lIter++)
	{
		 (*lIter)->write(lFile);
	}
	fclose(lFile);
}

#define UPDATE_FIXED_LINE(format, value)\
	{\
		char lBuffer[100]; \
		memset(lBuffer, ' ', sizeof(lBuffer));\
		lBuffer[99] = '\n';\
		sprintf(lBuffer, format, value);\
		lBuffer[strlen(lBuffer)] = ' ';\
		fseek(iFile, lLineStart, SEEK_SET);\
		fwrite(lBuffer, sizeof(lBuffer), 1, iFile);\
		fseek(iFile, lLineStart+100, SEEK_SET);\
	}\

#define CREATE_FIXED_LINE(format, value)\
	{\
		char lBuffer[100];\
		memset(lBuffer, ' ', sizeof(lBuffer));\
		lBuffer[99] = '\n';\
		sprintf(lBuffer, format, value);\
		lBuffer[strlen(lBuffer)] = ' ';\
		fwrite(lBuffer, sizeof(lBuffer), 1, lFile);\
	}\


FILE* PointCloud::writeHeader(std::string iName, PointCloudAttributes& iAttributes, uint64_t iSize, float* iMin, float* iMax, float iResolution)
{
	std::string lName = iName + ".ply";
	FILE* lFile = fopen(lName.c_str(), "wb+");

	fprintf(lFile, "ply\nformat binary_little_endian 1.0\n");
	if (iMin)
	{
		CREATE_FIXED_LINE("comment minx %f", iMin[0]);
		CREATE_FIXED_LINE("comment miny %f", iMin[1]);
		CREATE_FIXED_LINE("comment minz %f", iMin[2]);
	}
	else
	{
		CREATE_FIXED_LINE("comment minx %f", MIN[0]);
		CREATE_FIXED_LINE("comment miny %f", MIN[1]);
		CREATE_FIXED_LINE("comment minz %f", MIN[2]);
	}
	if (iMax)
	{
		CREATE_FIXED_LINE("comment maxx %f", iMax[0]);
		CREATE_FIXED_LINE("comment maxy %f", iMax[1]);
		CREATE_FIXED_LINE("comment maxz %f", iMax[2]);
	}
	else
	{
		CREATE_FIXED_LINE("comment maxx %f", MAX[0]);
		CREATE_FIXED_LINE("comment maxy %f", MAX[1]);
		CREATE_FIXED_LINE("comment maxz %f", MAX[2]);
	}

	CREATE_FIXED_LINE("comment resolution %f", iResolution);

	CREATE_FIXED_LINE("element vertex %lu", iSize);

	fprintf(lFile, "property float x\n"
				   "property float y\n"
				   "property float z\n");

	iAttributes.writeHeader(lFile);

	fprintf(lFile, "end_header\n");

	return lFile;
}



FILE* PointCloud::updateHeader(std::string iName)
{
	std::string lName = iName + ".ply";
	FILE* lFile = fopen(lName.c_str(), "rb+");

	return lFile;
}



void PointCloud::updateSize(FILE* iFile, uint64_t iSize)
{
	fseek(iFile, 0, SEEK_SET);
	char lLine[1024];
    size_t lLength = 0;
	uint32_t lLineStart = ftell(iFile);
	while (fgets(lLine,1024,iFile)) 
	{
		if (!strncmp(lLine, "element vertex", 14))
		{
			UPDATE_FIXED_LINE("element vertex %lu", iSize);
			break;
		}
		lLineStart = ftell(iFile);
	}
}

void PointCloud::updateSpatialBounds(FILE* iFile, double* iMin, double* iMax)
{
	float lMin[3];
	lMin[0] = iMin[0];
	lMin[1] = iMin[1];
	lMin[2] = iMin[2];

	float lMax[3];
	lMax[0] = iMax[0];
	lMax[1] = iMax[1];
	lMax[2] = iMax[2];

	updateSpatialBounds(iFile, lMin, lMax);
}

void PointCloud::updateSpatialBounds(FILE* iFile, float* iMin, float* iMax)
{
	fseek(iFile, 0, SEEK_SET);
	char lLine[1024];
    size_t lLength = 0;
	uint32_t lLineStart = ftell(iFile);
	while (fgets(lLine,1024,iFile)) 
	{
		//std::cout << strlen(lLine) << ":" << ftell(iFile) << "--" << lLine << "\n";
		
		if (!strncmp(lLine, "comment minx", 12))
		{
			UPDATE_FIXED_LINE("comment minx %f", iMin[0]);
		}
		else if (!strncmp(lLine, "comment miny", 12))
		{
			UPDATE_FIXED_LINE("comment miny %f", iMin[1]);
		}
		else if (!strncmp(lLine, "comment minz", 12))
		{
			UPDATE_FIXED_LINE("comment minz %f", iMin[2]);
		}
		else if (!strncmp(lLine, "comment maxx", 12))
		{
			UPDATE_FIXED_LINE("comment maxx %f", iMax[0]);
		}
		else if (!strncmp(lLine, "comment maxy", 12))
		{
			UPDATE_FIXED_LINE("comment maxy %f", iMax[1]);
		}
		else if (!strncmp(lLine, "comment maxz", 12))
		{
			UPDATE_FIXED_LINE("comment maxz %f", iMax[2]);
			break;
		}
		
		lLineStart = ftell(iFile);
	}
};

void PointCloud::updateResolution(FILE* iFile, float iResolution)
{
	fseek(iFile, 0, SEEK_SET);
	char lLine[1024];
	size_t lLength = 0;
	uint32_t lLineStart = ftell(iFile);
	while (fgets(lLine, 1024, iFile))
	{
		if (!strncmp(lLine, "comment resolution", 18))
		{
			UPDATE_FIXED_LINE("comment resolution %f", iResolution);
			break;
		}

		lLineStart = ftell(iFile);
	}
}

//
//
//

json_spirit::mObject PointCloud::toJson()
{
	json_spirit::mObject lObject;
	lObject["pointcount"] = mPointCount;
	json_spirit::mArray lAttributes;
	PointCloudAttributes::toJson(lAttributes);
	lObject["attributes"] = lAttributes;
 
	return lObject;
}

void PointCloud::fromJson(json_spirit::mObject& iObject)
{
	mPointCount = iObject["pointcount"].get_int64();
	PointCloudAttributes::fromJson(iObject["attributes"].get_array());
}


