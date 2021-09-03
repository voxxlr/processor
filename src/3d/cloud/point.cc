#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

#include "point.h"

const std::string Attribute::POSITION("position");
const std::string Attribute::NORMAL("normal");
const std::string Attribute::COLOR("color");
const std::string Attribute::CLASS("class");
const std::string Attribute::INTENSITY("intensity");

template <typename T> const char* SingleAttribute<T>::cTypeName = "undefined";

template <> const char* SingleAttribute<uint8_t>::cTypeName = "uint8";
template <> const char* SingleAttribute<uint16_t>::cTypeName = "uint16";
template <> const char* SingleAttribute<uint32_t>::cTypeName = "uint32";
template <> const char* SingleAttribute<float>::cTypeName = "float32";
template <> const char* SingleAttribute<Vec3IntPacked>::cTypeName = "INT_2_10_10_10_REV";


template<> ArrayAttribute<float, 3>* ArrayAttribute<float, 3>::sHead = 0;
template<> ArrayAttribute<uint8_t, 3>* ArrayAttribute<uint8_t, 3>::sHead = 0;
template<> SingleAttribute<uint32_t>* SingleAttribute<uint32_t>::sHead = 0;
template<> SingleAttribute<uint16_t>* SingleAttribute<uint16_t>::sHead = 0;
template<> SingleAttribute<uint8_t>* SingleAttribute<uint8_t>::sHead = 0;
template<> SingleAttribute<Vec3IntPacked>* SingleAttribute<Vec3IntPacked>::sHead = 0;

void _average(ColorType& iA0, ColorType& iA1, float iWeight)
{
	for (int i=0; i<3; i++)
	{
		iA0.mValue[i] = (uint8_t)((iA0.mValue[i]*iWeight + iA1.mValue[i])/(iWeight+1.0));
	}
}

void _average(IntensityType& iA0, IntensityType& iA1, float iWeight)
{
	iA0.mValue= (uint8_t)((iA0.mValue*iWeight + iA1.mValue)/(iWeight+1.0));
}

void _add(ColorType& iA0, ColorType& iA1, float iWeight)
{
	for (int i=0; i<3; i++)
	{
		iA0.mValue[i] += (uint8_t)(iA1.mValue[i]*iWeight);
	}
}

void _add(IntensityType& iA0, IntensityType& iA1, float iWeight)
{
	iA0.mValue= (uint8_t)(iA1.mValue*iWeight);
}


void _max(ColorType& iA0, ColorType& iA1)
{
	for (int i=0; i<3; i++)
	{
		iA0.mValue[i] = std::max(iA0.mValue[i], iA1.mValue[i]);
	}
}

void _max(IntensityType& iA0, IntensityType& iA1)
{
	iA0.mValue = std::max(iA1.mValue, iA0.mValue);
}

IntensityType INTENSITY_TEMPLATE;
ColorType COLOR_TEMPLATE;
ClassType CLASS_TEMPLATE;
NormalType NORMAL_TEMPLATE;

Point::Point()
: mWeight(0)
, mMask(0)
{
}

Point::Point(std::vector<Attribute*>& iAttributes)
: mWeight(0)
, mMask(0)
{
	mAttributes.reserve(iAttributes.size());
	for (std::vector<Attribute*>::iterator lIter = iAttributes.begin() ; lIter != iAttributes.end(); ++lIter)
	{
		mAttributes.push_back((*lIter)->clone());
	}
}

Point::~Point()
{
	for (std::vector<Attribute*>::iterator lIter = mAttributes.begin() ; lIter != mAttributes.end(); ++lIter)
	{
		delete *lIter;
	}
}

//
// I/O
//

void Point::write(FILE* iFile)
{
	fwrite(position, sizeof(position),1,iFile);
	for (std::vector<Attribute*>::iterator lIter = mAttributes.begin() ; lIter != mAttributes.end(); ++lIter)
	{
		(*lIter)->write(iFile);
	}
};

void Point::read(FILE* iFile)
{
	fread(position, sizeof(position), 1, iFile);
	for (std::vector<Attribute*>::iterator lIter = mAttributes.begin() ; lIter != mAttributes.end(); ++lIter)
	{
		(*lIter)->read(iFile);
	}
}

uint8_t* Point::map(uint8_t* iMemory)
{
	for (std::vector<Attribute*>::iterator lIter = mAttributes.begin() ; lIter != mAttributes.end(); ++lIter)
	{
		iMemory += (*lIter)->map(iMemory);
	}
	return iMemory;
};

//
// Attribute API
// 

Attribute* Point::getAttribute(int iIndex)
{
	std::vector<Attribute*>& lAttributes = getAttributes();
	if (iIndex >= 0 && iIndex < lAttributes.size())
	{
		return lAttributes[iIndex];
	}
	return 0;
}

std::vector<Attribute*>& Point::getAttributes()
{
	return mAttributes;
}

void Point::addAttribute(Attribute& iTemplate)
{
	mAttributes.push_back(iTemplate.clone());
};

Point* Point::create(std::vector<Attribute*>& iAttributes)
{
	Point* lPoint = 0;
	if (!lPoint)
	{
		lPoint = new Point(iAttributes);
	}
	
	return lPoint;

};

void Point::destroy(Point* iPoint)
{
	delete iPoint;
};

