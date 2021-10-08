#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include <tuple>

#include "point.h"

class PointCloudAttributes
{
	friend class PointCloud;

	public:

		PointCloudAttributes();
		PointCloudAttributes(PointCloudAttributes& iAttributes);
		~PointCloudAttributes();

		void load(json_spirit::mArray iArray);

		int createAttribute(const std::string& iName, Attribute& iAttribute);
		int getAttributeIndex(const std::string& iName);
		bool hasAttribute(const std::string& iName);
		int attributeCount()
		{
			return mList.size();
		}

		void average(Point& iDest, Point& iSample);

		void readHeader(char* iName);
		void writeHeader(FILE* iFile);

		void fromJson(json_spirit::mArray& iObject);
		void toJson(json_spirit::mArray& iArray);
			
		operator std::vector<Attribute*>& ()
		{
			return mList;
		};

		size_t bytesPerPoint()
		{
			return mByteCount;
		}

	protected:

		void addAttributes(PointCloudAttributes& iAttrbutes, std::vector<std::tuple<std::string, Attribute*>>& iAdded);

		std::map<std::string, int> mMap;
		std::vector<Attribute*> mList;
		std::vector<int> mAverage;
		uint32_t mByteCount;
};

class PointCloud : public PointCloudAttributes
{
	public:

		static float MIN[3];
		static float MAX[3];

		PointCloud();
		PointCloud(PointCloudAttributes& iAttributes);
		~PointCloud();

		uint64_t mPointCount;
		float mMinExtent[3];
		float mMaxExtent[3];
		float mResolution;

		void addAttributes(PointCloudAttributes& iPointCloud);

		Attribute* getAttribute(Point& iPoint, const std::string& iName);


		// 
		// point access
		//
		std::vector<Point*>::iterator begin()
		{
			return mPoints.begin();
		}
		std::vector<Point*>::iterator end()
		{
			return mPoints.end();
		}
		Point*& operator[] (size_t iIndex)
		{
			return mPoints[iIndex];
		};
		size_t size()
		{
			return mPoints.size();
		}
		void resize(size_t iSize)
		{
			mPoints.resize(iSize);
		}

		std::vector<Point*> mPoints;

		static uint64_t maxMemoryUsage(uint32_t iPoints, PointCloudAttributes& iAttributes)
		{
			uint64_t lCount = 0;
			std::vector<Attribute*>& lAttributes = iAttributes;
			for (std::vector<Attribute*>::iterator lIter = lAttributes.begin(); lIter < lAttributes.end(); lIter++)
			{
				lCount += (*lIter)->objectSize() + sizeof(Attribute*);
			};

			return iPoints*(sizeof(Point) + lCount);
		}

		//
		// reading
		//
		void readFile(std::string& iName);
		void fromJson(json_spirit::mObject& iObject);

		//
		// writing
		//
		void writeFile(std::string& iName);
		json_spirit::mObject toJson();


		static void updateSpatialBounds(FILE* iFile, float* iMin, float* iMax);
		static void updateSpatialBounds(FILE* iFile, double* iMin, double* iMax);
		static void updateResolution(FILE* iFile, float iResolution);
		static FILE* writeHeader(std::string iName, PointCloudAttributes& iAttributes, uint64_t iSize = 0, float* iMin = 0, float* iMax = 0, float iResolution = 0);
		static FILE* readHeader(std::string& iName, PointCloudAttributes* iAttributes, uint64_t& iSize, float* iMin = 0, float* iMax = 0, float* iResolution = 0);
		static void updateSize(FILE* iFile, uint64_t iSize);
		static FILE* updateHeader(std::string iName);


};

