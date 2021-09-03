#pragma once

#include "../../pointCloud.h"

class CloudImporter
{
	public:

		CloudImporter(uint8_t iCoords, float iScalar, float iResolution);
	
		virtual json_spirit::mObject import (std::string iName, glm::dvec3* iCenter) = 0;

		static const uint8_t RIGHT_Y_UP = 0;
		static const uint8_t RIGHT_Z_UP = 1;
		static const uint8_t LEFT_Y_UP = 2;
		static const uint8_t LEFT_Z_UP = 3;

		static uint8_t toCoords(std::string iString)
		{
			if (!iString.compare("right-y"))
			{
				return RIGHT_Y_UP;
			} 
			else if (!iString.compare("right-z"))
			{
				return RIGHT_Z_UP;
			}
			else if (!iString.compare("left-y"))
			{
				return LEFT_Y_UP;
			}
			else if (!iString.compare("left-z"))
			{
				return LEFT_Z_UP;
			}
			return RIGHT_Y_UP;
		};

		//
		// Double
		//
		
		double mMinD[3];
		double mMaxD[3];
		double mCenter[3];

		template <class T> inline void convertCoords(T* iPoint)
		{
			switch (mCoords)
			{
			case RIGHT_Y_UP:
				break;
			case RIGHT_Z_UP:
				{
					T lY = iPoint[1];
					iPoint[1] = iPoint[2];
					iPoint[2] = -lY;
				}
				break;
			case LEFT_Y_UP:
				iPoint[2] = -iPoint[2];
				break;
			case LEFT_Z_UP:
				{
					T lY = iPoint[1];
					iPoint[1] = iPoint[2];
					iPoint[2] = lY;
				}
				break;
			}

			iPoint[0] *= mScalarD;
			iPoint[1] *= mScalarD;
			iPoint[2] *= mScalarD;
		}
		/*
		template <class T> inline void centerD(T* iPosition, T* c)
		{
			iPosition[0] -= c[0];
			iPosition[1] -= c[1];
			iPosition[2] -= c[2];
		}
		*/
		template <class T> inline void center(T* iPosition)
		{
			iPosition[0] -= mCenter[0];
			iPosition[1] -= mCenter[1];
			iPosition[2] -= mCenter[2];
		}
		
		template <class T> inline void growMinMax(T* iPosition)
		{
			mMinD[0] = std::min<double>(iPosition[0], mMinD[0]);
			mMinD[1] = std::min<double>(iPosition[1], mMinD[1]);
			mMinD[2] = std::min<double>(iPosition[2], mMinD[2]);
			mMaxD[0] = std::max<double>(iPosition[0], mMaxD[0]);
			mMaxD[1] = std::max<double>(iPosition[1], mMaxD[1]);
			mMaxD[2] = std::max<double>(iPosition[2], mMaxD[2]);
		}
		/*
		inline void growMinMaxD(glm::dvec4& iPosition)
		{
			mMinD[0] = std::min<double>(iPosition[0], mMinD[0]);
			mMinD[1] = std::min<double>(iPosition[1], mMinD[1]);
			mMinD[2] = std::min<double>(iPosition[2], mMinD[2]);
			mMaxD[0] = std::max<double>(iPosition[0], mMaxD[0]);
			mMaxD[1] = std::max<double>(iPosition[1], mMaxD[1]);
			mMaxD[2] = std::max<double>(iPosition[2], mMaxD[2]);
		}
		*/
		void done();
 
		json_spirit::mObject getMeta(uint8_t iMaxClass);

	protected:

		static uint16_t sDefaultClasses[19*3];

		uint8_t mCoords;
		double mScalarD;
		float mResolution;

}; 
