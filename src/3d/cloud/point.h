#pragma once

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <stdint.h>
#include <vector>
#include <map>
#include <string>

class Attribute  
{
	public:

		static const std::string POSITION;
		static const std::string NORMAL;
		static const std::string COLOR;
		static const std::string CLASS;
		static const std::string INTENSITY;
		static const std::string FRAME;

		virtual Attribute* clone() = 0;
		virtual size_t bytesPerPoint() = 0;
		virtual size_t objectSize() = 0;
		virtual void write(FILE* iFile) = 0;	
		virtual void read(FILE* iFile) = 0;
		virtual uint8_t map(uint8_t* iPointer) = 0;

		virtual void toJson(json_spirit::mObject& iObject) = 0;
		
		virtual void average(Attribute& iAttribute, float iWeight) {};
		virtual void add(Attribute& iAttribute, float iWeight) {};
		virtual void max(Attribute& iAttribute) {};
};


template <class T> class SingleAttribute;
template <class T> void _average(SingleAttribute<T>& iA0, SingleAttribute<T>& iA1, float iWeight) {};
template <class T> void _add(SingleAttribute<T>& iA0, SingleAttribute<T>& iA1, float iWeight) {};
template <class T> void _max(SingleAttribute<T>& iA0, SingleAttribute<T>& iA1) {};

template <class T> class SingleAttribute : public Attribute
{
	public:

		T mValue;

		SingleAttribute() { memset(&mValue, 0, sizeof (mValue)); }
		Attribute* clone() { return new SingleAttribute();  }
		size_t bytesPerPoint()  { return sizeof(mValue); }
		size_t objectSize()  { return sizeof(SingleAttribute<T>); }

		void write(FILE* iFile) { fwrite(&mValue, sizeof(mValue),1,iFile); };
		void read(FILE* iFile) { fread(&mValue, sizeof(mValue),1,iFile); };
		uint8_t map(uint8_t* iPointer)
		{
			memcpy(&mValue, iPointer, sizeof(mValue)); 
			return sizeof(mValue);
		};
		
 		static const char * cTypeName;
		void toJson(json_spirit::mObject& iObject)
		{
			iObject["size"] = 1;
			iObject["type"] = SingleAttribute<T>::cTypeName;
		};	
		
		void average(Attribute& iAttribute, float iWeight)
		{
			_average(*this, (SingleAttribute<T>&)iAttribute, iWeight);
		};
		void add(Attribute& iAttribute, float iWeight)
		{
			_add(*this, (SingleAttribute<T>&)iAttribute, iWeight);
		};
		void max(Attribute& iAttribute)
		{
			_max(*this, (SingleAttribute<T>&)iAttribute);
		};

		static SingleAttribute<T>* sHead;
		static boost::mutex sLock;
};

typedef SingleAttribute<uint8_t> ClassType;
typedef SingleAttribute<uint16_t> IntensityType;
void _average(IntensityType& iA0, IntensityType& iA1, float iWeight);
void _add(IntensityType& iA0, IntensityType& iA1, float iWeight);
void _max(IntensityType& iA0, IntensityType& iA1);

union Vec3IntPacked {
	int i32;
	struct {
		int x:10;
		int y:10;
		int z:10;
		int a:2;
	} i32f3;
};

typedef SingleAttribute<Vec3IntPacked> PackedNormalType;




template <class T, int N> class ArrayAttribute;
template <class T, int N> void _average(ArrayAttribute<T,N>& iA0, ArrayAttribute<T,N>& iA1, float iWeight) {};
template <class T, int N> void _add(ArrayAttribute<T,N>& iA0, ArrayAttribute<T,N>& iA1, float iWeight) {};
template <class T, int N> void _max(ArrayAttribute<T,N>& iA0, ArrayAttribute<T,N>& iA1) {};

template <class T, int N> class ArrayAttribute : public Attribute
{
	public:

		T mValue[N];

		ArrayAttribute() { memset(mValue, 0, sizeof (mValue)); }
		ArrayAttribute(T iValue[N]) { memcpy(mValue, iValue, sizeof(mValue)); }
		Attribute* clone()  { return new ArrayAttribute(mValue); }
		size_t bytesPerPoint()  { return sizeof(mValue); }
		size_t objectSize()  { return sizeof(ArrayAttribute<T,N>); }

		void write(FILE* iFile) { fwrite(mValue, sizeof(mValue),1,iFile); };
		void read(FILE* iFile) { fread(mValue, sizeof(mValue),1,iFile); };
		uint8_t map(uint8_t* iPointer)
		{
			memcpy(mValue, iPointer, sizeof(mValue)); 
			return sizeof(mValue);
		};
	
 		void toJson(json_spirit::mObject& iObject)
		{
			iObject["size"] = N;
			iObject["type"] = SingleAttribute<T>::cTypeName;
		};	
		
		void average(Attribute& iAttribute, float iWeight)
		{
			_average(*this, (ArrayAttribute<T, N>&)iAttribute, iWeight);
		};
		
		void add(Attribute& iAttribute, float iWeight)
		{
			_add(*this, (ArrayAttribute<T, N>&)iAttribute, iWeight);
		};

		void max(Attribute& iAttribute)
		{
			_max(*this, (ArrayAttribute<T, N>&)iAttribute);
		};

		static ArrayAttribute<T, N>* sHead;
		static boost::mutex sLock;
};

typedef ArrayAttribute<uint8_t, 3> ColorType;
typedef ArrayAttribute<float, 3> NormalType;

void _average(ColorType& iA0, ColorType& iA1, float iWeight);
void _add(ColorType& iA0, ColorType& iA1, float iWeight);
void _max(ColorType& iA0, ColorType& iA1);

extern NormalType NORMAL_TEMPLATE;
extern ColorType COLOR_TEMPLATE;
extern ClassType CLASS_TEMPLATE;
extern IntensityType INTENSITY_TEMPLATE;




class Point
{
	public:

		float position[3];

		Point(std::vector<Attribute*>& iAttributes);
		~Point();

		// 
		// geometric functions
		//

		inline bool inside(float* iMin, float *iMax)
		{
			return  position[0] >= iMin[0] && 
					position[0] <= iMax[0] && 
					position[1] >= iMin[1] && 
					position[1] <= iMax[1] &&  
					position[2] >= iMin[2] && 
	     			position[2] <= iMax[2];
		}



		//
		// traveral
		//
		static const uint8_t SELECTED = 0x01;
		static const uint8_t TAGGED = 0x02;
		static const uint8_t VOTE = 0x04;
		static const uint8_t DELETED = 0x08;

		inline void clr(uint8_t iBit)
		{
			mMask &= ~iBit;
		}
		inline void set(uint8_t iBit)
		{
			mMask |= iBit;
		}
		inline bool isSet(uint8_t iBit)
		{
			return (mMask & iBit) != 0;
		}
		uint8_t mMask;

		//
		// attributes
		//
		Attribute* getAttribute(int iIndex);
		std::vector<Attribute*>& getAttributes();
		void addAttribute(Attribute& iTemplate);

		float mWeight;

		//
		// I/O
		//
		void write(FILE* iFile);
		void read(FILE* iFile);
		uint8_t* map(uint8_t* iMemory);

		static Point* create(std::vector<Attribute*>& iAttributes);
		static void destroy(Point* iPoint);

	private:

		Point();
		Point& operator=(const Point&); 

		std::vector<Attribute*> mAttributes;
};



		/*
		void normalize()  
		{  
			float lLength = 0;
			for (int i=0; i<N; i++)
			{
				lLength += mValue[i]*mValue[i];
			}

			lLength = sqrt(lLength);
			if (lLength)
			{
				mValue[0] /= lLength;
				mValue[1] /= lLength;
				mValue[2] /= lLength;
			}
		};
		

		float dot(ArrayAttribute<T,N>& iVector)
		{
			float lResult = 0;
			for (int i=0; i<N; i++)
			{
				lResult += mValue[i]*iVector.mValue[i];
			}
			return lResult;
		}

		void invert()
		{
			for (int i=0; i<N; i++)
			{
				mValue[i] =- mValue[i];
			}
		}
		*/
