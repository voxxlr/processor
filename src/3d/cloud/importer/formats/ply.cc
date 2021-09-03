#include "ply.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

static const uint8_t ASCII = 1;
static const uint8_t BINARY_LITTLE_ENDIAN = 2;
static const uint8_t BINARY_BIG_ENDIAN = 3;

class TypeReader
{
	public:

		virtual void skipA(char* iString) = 0;
		virtual float getFloatA(char* iString) = 0;
		virtual uint8_t getUint8A(char* iString) = 0;
		virtual uint16_t getUint16A(char* iString) = 0;

		virtual void skipBL(FILE* iFile) = 0;
		virtual float getFloatBL(FILE* iFile, uint8_t iEndian) = 0;
		virtual uint8_t getUint8BL(FILE* iFile, uint8_t iEndian) = 0;
		virtual uint16_t getUint16BL(FILE* iFile, uint8_t iEndian) = 0;

	protected:

		float reverseFloat(const float inFloat)
		{
		   float retVal;
		   char *floatToConvert = ( char* ) & inFloat;
		   char *returnFloat = ( char* ) & retVal;

		   // swap the bytes into a temporary buffer
		   returnFloat[0] = floatToConvert[3];
		   returnFloat[1] = floatToConvert[2];
		   returnFloat[2] = floatToConvert[1];
		   returnFloat[3] = floatToConvert[0];

		   return retVal;
		}
};


#define TYPEREADER(type,vartype,format) \
class type##Reader : public TypeReader \
{\
	public:\
	\
		inline type readBL(FILE* iFile)\
		{\
			type lValue;\
			fread(&lValue, sizeof(type),1,iFile);\
			return lValue;\
		};\
		\
		float getFloatBL(FILE* iFile, uint8_t iEndian)\
		{\
			float lValue = (float)readBL(iFile);\
			if (iEndian == BINARY_BIG_ENDIAN)\
			{\
				return reverseFloat(lValue);\
			}\
			return lValue;\
		};\
		uint8_t getUint8BL(FILE* iFile, uint8_t iEndian)\
		{\
			return (uint8_t)readBL(iFile);\
		};\
		uint16_t getUint16BL(FILE* iFile, uint8_t iEndian)\
		{\
			return (uint16_t)readBL(iFile);\
		};\
		void skipBL(FILE* iFile)\
		{\
			readBL(iFile);\
		};\
		\
		inline type readA(char* iString)\
		{\
			vartype lValue;\
			sscanf(iString, format, &lValue);\
			return lValue;\
		};\
		\
		float getFloatA(char* iValue)\
		{\
			return (float)readA(iValue);\
		};\
		uint8_t getUint8A(char* iValue)\
		{\
			return (uint8_t)readA(iValue);\
		};\
		uint16_t getUint16A(char* iValue)\
		{\
			return (uint16_t)readA(iValue);\
		};\
		void skipA(char* iValue)\
		{\
			readA(iValue);\
		};\
		\
} type##Instance;\



TYPEREADER(uint32_t, uint32_t, "%du");
TYPEREADER(uint16_t, uint16_t,"%hu");
TYPEREADER(uint8_t, uint16_t,"%hu");
TYPEREADER(int32_t, int32_t, "%d");
TYPEREADER(int16_t, int16_t, "%h");
TYPEREADER(int8_t, int16_t, "%h");
TYPEREADER(float,float,"%f");
TYPEREADER(double,double,"%lf");


class PropertyReader
{
	public:

		virtual void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian) = 0;
		virtual void readA(Point& iPoint, char* iBuffer) = 0;
};

class VertexReader : public PropertyReader
{
	public:

		TypeReader& mReader;
		int mIndex;

		VertexReader(int iIndex, TypeReader& iReader)
		: mIndex(iIndex)
		, mReader(iReader)
		{
		}

		void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian)
		{
			iPoint.position[mIndex] = mReader.getFloatBL(iFile, iEndian);
		};
		void readA(Point& iPoint, char* iString)
		{
			iPoint.position[mIndex] = mReader.getFloatA(iString);
		};
};

class NormalReader : public PropertyReader
{
	public:

		TypeReader& mReader;
		int mAttributeIndex;
		int mIndex;

		NormalReader(int iIndex, TypeReader& iReader, int iAttributeIndex)
		: mIndex(iIndex)
		, mReader(iReader)
		, mAttributeIndex(iAttributeIndex)
		{
		}

		void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian)
		{
			NormalType& lAttribute = *(NormalType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue[mIndex] = mReader.getFloatBL(iFile, iEndian);
		};
		void readA(Point& iPoint, char* iString)
		{
			NormalType& lAttribute = *(NormalType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue[mIndex] = mReader.getFloatA(iString);
		};
};

class ColorReader : public PropertyReader
{
	public:

		TypeReader& mReader;
		int mAttributeIndex;
		int mIndex;

		ColorReader(int iIndex, TypeReader& iReader, int iAttributeIndex)
		: mIndex(iIndex)
		, mReader(iReader)
		, mAttributeIndex(iAttributeIndex)
		{
		}

		void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian)
		{
			ColorType& lAttribute = *(ColorType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue[mIndex] = mReader.getUint8BL(iFile, iEndian);
		};
		void readA(Point& iPoint, char* iString)
		{
			ColorType& lAttribute = *(ColorType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue[mIndex] = mReader.getUint8A(iString);
		};
};

class IntensityReader : public PropertyReader
{
	public:

		TypeReader& mReader;
		int mAttributeIndex;
	
		IntensityReader(TypeReader& iReader, int iAttributeIndex)
		: mReader(iReader)
		, mAttributeIndex(iAttributeIndex)
		{
		}

		void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian)
		{
			ClassType& lAttribute = *(ClassType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue = mReader.getUint16BL(iFile, iEndian);
		};
		void readA(Point& iPoint, char* iString)
		{
			ClassType& lAttribute = *(ClassType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue = mReader.getUint16A(iString);
		};
};

class ClassReader : public PropertyReader
{
	public:

		TypeReader& mReader;
		int mAttributeIndex;
	
		ClassReader(TypeReader& iReader, int iAttributeIndex)
		: mReader(iReader)
		, mAttributeIndex(iAttributeIndex)
		{
		}

		void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian)
		{
			ClassType& lAttribute = *(ClassType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue = mReader.getUint8BL(iFile, iEndian);
		};
		void readA(Point& iPoint, char* iString)
		{
			ClassType& lAttribute = *(ClassType*)iPoint.getAttribute(mAttributeIndex);
			lAttribute.mValue = mReader.getUint8A(iString);
		};
};

class NullReader : public PropertyReader
{
	public:

		TypeReader& mReader;

		NullReader(TypeReader& iReader)
		: mReader(iReader)
		{
		}

		void readBL(Point& iPoint, FILE* iFile, uint8_t iEndian)
		{
			mReader.getFloatBL(iFile, iEndian);
		};

		void readA(Point& iPoint, char* iString)
		{
			mReader.getFloatA(iString);
		};
};


TypeReader& getReader(char* iType)
{
	if (!strcmp(iType, "char") || !strcmp(iType, "int8"))
	{
		return int16_tInstance;
	}
	else if (!strcmp(iType, "short") || !strcmp(iType, "int16"))
	{
		return int16_tInstance;
	}
	else if (!strcmp(iType, "int") || !strcmp(iType, "int32"))
	{
		return int32_tInstance;
	}
	else if (!strcmp(iType, "uchar") || !strcmp(iType, "uint8"))
	{
		return uint8_tInstance;
	}
	else if (!strcmp(iType, "ushort") || !strcmp(iType, "uint16"))
	{
		return uint16_tInstance;
	}
	else if (!strcmp(iType, "uint") || !strcmp(iType, "uint32"))
	{
		return uint32_tInstance;
	}
	else if (!strcmp(iType, "float") || !strcmp(iType, "float32"))
	{
		return floatInstance;
	}
	else if (!strcmp(iType, "double") || !strcmp(iType, "float64"))
	{
		return doubleInstance;
	}
}


void readPlyPoint(Point& iPoint, FILE* iFile, uint32_t iFormat, PropertyReader** iFields, uint32_t iIndex)
{
	switch (iFormat)
	{
		case BINARY_LITTLE_ENDIAN:
		case BINARY_BIG_ENDIAN:
			{
				for (int i=0; i<iIndex; i++)
				{
					iFields[i]->readBL(iPoint, iFile, iFormat);
				}
			}
			break;
		case ASCII:
			{
				char lLine[256];
				fgets(lLine, 256, iFile); 
				uint8_t lLength = strlen(lLine);
				lLine[lLength-1] = 0x00;

				int lPtr = 0;
				for (int i=0; i<iIndex; i++)
				{
					char lValue[255];
					while (lLine[lPtr] == ' ')
					{
						lPtr++;
					}
					sscanf(&lLine[lPtr], "%s", lValue);
					iFields[i]->readA(iPoint, lValue);
					lPtr+= strlen(lValue)+1;
				}
			}
			break;
	}		
};


PlyImporter::PlyImporter(uint8_t iCoords, float iScalar, float iResolution)
: CloudImporter(iCoords, iScalar, iResolution)
{
};


json_spirit::mObject PlyImporter::import (std::string iName, glm::dvec3* iCenter)
{
	FILE* lInputFile = fopen(iName.c_str(),"rb");
	
	//setvbuf (InputFile, NULL, _IOFBF, BUFFER_SIZE);

	char lLine[1024];
    size_t lLength = 0;
	fgets(lLine, 256, lInputFile); 
	lLine[strlen(lLine)-1] = 0x00;

	PointCloudAttributes lAttributes;
	PropertyReader* lFields[200];
	memset(lFields,0,sizeof(lFields));

	uint32_t lFormat = 0;
	uint32_t lIndex = 0;
	uint32_t lDatastart = 0;
	unsigned long lPointCount = 0;
	bool lVertexElement = false;
	while (fgets(lLine,1024,lInputFile)) 
	{
		char lBuffer[4][64];
		//std::cout << lLine << "\n";
		sscanf(lLine, "%s %s %s", (char*)&lBuffer[0], (char*)&lBuffer[1], (char*)&lBuffer[2]);
		if (!strcmp(lBuffer[0], "format"))
		{
			if (!strcmp(lBuffer[1],"ascii"))
			{
				lFormat = ASCII;
			}
			else if (!strcmp(lBuffer[1],"binary_little_endian"))
			{
				lFormat = BINARY_LITTLE_ENDIAN;
			}
			else if (!strcmp(lBuffer[1],"binary_big_endian"))
			{
				lFormat = BINARY_BIG_ENDIAN;
			}
		}
		if (!strcmp(lBuffer[0], "property") && lVertexElement)
		{
			if (!strcmp(lBuffer[2], "x"))
			{
				lFields[lIndex] = new VertexReader(0, getReader(lBuffer[1]));
			}
			else if (!strcmp(lBuffer[2], "y"))
			{
				lFields[lIndex] = new VertexReader(1, getReader(lBuffer[1]));
			}
			else if (!strcmp(lBuffer[2], "z"))
			{
				lFields[lIndex] = new VertexReader(2, getReader(lBuffer[1]));
			}
			else if (!strcmp(lBuffer[2], "nx"))
			{
				lFields[lIndex] = new NormalReader(0, getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::NORMAL, NORMAL_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "ny"))
			{
				lFields[lIndex] = new NormalReader(1, getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::NORMAL, NORMAL_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "nz"))
			{
				lFields[lIndex] = new NormalReader(2, getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::NORMAL, NORMAL_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "r") || !strcmp(lBuffer[2], "red") || !strcmp(lBuffer[2], "diffuse_red"))
			{
				lFields[lIndex] = new ColorReader(0, getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::COLOR, COLOR_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "g") || !strcmp(lBuffer[2], "green") || !strcmp(lBuffer[2], "diffuse_green"))
			{
				lFields[lIndex] = new ColorReader(1, getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::COLOR, COLOR_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "b") || !strcmp(lBuffer[2], "blue") || !strcmp(lBuffer[2], "diffuse_blue"))
			{
				lFields[lIndex] = new ColorReader(2, getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::COLOR, COLOR_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "intensity"))
			{
				lFields[lIndex] = new IntensityReader(getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::INTENSITY, INTENSITY_TEMPLATE));
			}
			else if (!strcmp(lBuffer[2], "class"))
			{
				lFields[lIndex] = new ClassReader(getReader(lBuffer[1]), lAttributes.createAttribute(Attribute::CLASS, CLASS_TEMPLATE));
			}
			else
			{
				lFields[lIndex] = new NullReader(getReader(lBuffer[1]));
			}
			lIndex++; 
		}
		else if (!strcmp(lBuffer[0], "element"))
		{
			if (!strcmp(lBuffer[1], "vertex"))
			{
				lVertexElement = true;
				sscanf(lBuffer[2], "%lu", &lPointCount);
			}
			else
			{
				lVertexElement = false;
			}
		}
		else if (!strncmp(lLine, "end_header", 10))
		{
			break;
		}
    }
	lDatastart = ftell(lInputFile);

	BOOST_LOG_TRIVIAL(info) << iName << " contains " << lPointCount << " points ";
	Point lPoint(lAttributes);

	// aabb pass
	if (!iCenter)
	{
		double lCoord[3];
		fseek(lInputFile, lDatastart, SEEK_SET);
		for(unsigned long p=0; p < lPointCount; p++)
		{
			readPlyPoint(lPoint, lInputFile, lFormat, lFields, lIndex);
			convertCoords(lPoint.position);
			growMinMax(lPoint.position);
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



	ColorType* lColor = 0;
	int lColorIndex = lAttributes.getAttributeIndex(Attribute::COLOR);
	if (lColorIndex != -1)
	{
		lColor = (ColorType*)lPoint.getAttribute(lColorIndex);
	}

	// write pass
	FILE* lOutputFile = PointCloud::writeHeader("cloud", lAttributes, 0, 0, 0, mResolution);
	fseek(lInputFile, lDatastart, SEEK_SET);
	for(unsigned long p=0; p < lPointCount; p++)
	{
		readPlyPoint(lPoint, lInputFile, lFormat, lFields, lIndex);
		convertCoords(lPoint.position);
		
		center(lPoint.position);
		if (iCenter)
		{
			growMinMax(lPoint.position);
		}

		lPoint.write(lOutputFile);
	}

	PointCloud::updateSize(lOutputFile, lPointCount);
	PointCloud::updateSpatialBounds(lOutputFile, mMinD, mMaxD);
	fclose(lOutputFile); 

	fclose(lInputFile);

	return getMeta(0);
}
