#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <queue>

#define _USE_MATH_DEFINES
#include <math.h>

#include "IfcLoader.h"



IfcLoader::IfcLoader()
: mInt0(Token::Token_INT)
, mBoolFalse(Token::Token_BOOL)
{
};


void IfcLoader::invert(uint32_t iId, std::vector<Argument*>& iAttributes)
{
	for (std::vector<Argument*>::iterator lIter = iAttributes.begin(); lIter < iAttributes.end(); lIter++)
	{
		if (dynamic_cast<ReferenceArgument*> (*lIter))
		{
			ReferenceArgument* lReference = dynamic_cast<ReferenceArgument*> (*lIter);
			IfcBaseClass::sINVERSE[lReference->mId].push_back(iId);
		}
		else if (dynamic_cast<ArgumentList*> (*lIter))
		{
			ArgumentList* lList = dynamic_cast<ArgumentList*> (*lIter);
			invert(iId, lList->arguments());
		}
	}
}


void IfcLoader::load(std::vector<Argument*>& iAttributes) 
{
	Token lToken = Token::next();
	while(!lToken.isOperator(')'))
	{
		if (lToken.isOperator(',')) 
		{
		} 
		else if (lToken.isOperator('(')) 
		{
			ArgumentList* lList = new ArgumentList();
			load(lList->arguments());
			iAttributes.push_back(lList);
		} 
		else if (lToken.isOperator('$')) 
		{
			iAttributes.push_back(new NullArgument());
		} 
		else 
		{
			if (lToken.mType == Token::Token_IDENTIFIER) 
			{
				iAttributes.push_back(new ReferenceArgument(lToken));
			} 
			else if (lToken.mType == Token::Token_KEYWORD) 
			{
				IfcEntityInstanceData* lData = new IfcEntityInstanceData(Type::FromString(lToken.mString), 0);
				std::vector<Argument*>& lAttributes = lData->attributes();
				lToken = Token::next();
				load(lAttributes);
	
				iAttributes.push_back(new EntityArgument(SchemaEntity(lData)));
			} 
			else 
			{
				iAttributes.push_back(new ValueArgument(lToken));
			}
		}
		lToken = Token::next();
	}
}

void IfcLoader::process(std::string iFile)
{
	BOOST_LOG_TRIVIAL(info)  << "Processing file " << iFile;

	FILE* lFile = fopen(iFile.c_str(), "rb");

	Verison lVersion = IFC4;

	// read header
 	char lBuffer[1024];
    while (fgets(lBuffer, sizeof(lBuffer), lFile)) 
	{
		if (!strncmp(lBuffer, "FILE_SCHEMA", 11))
		{
			if (strstr(lBuffer, "IFC4"))
			{
				lVersion = IFC4;
			}
			else if (strstr(lBuffer, "IFC2x3") || strstr(lBuffer, "IFC2X3"))
			{
				lVersion = IFC2x3;
			}
		}

		if (!strncmp(lBuffer, "DATA", 4))
		{
			break;
		}
    }

	Token::init(lFile);
	while (Token::sNext) 
	{
		Token lToken = Token::next();
		if (lToken.mType == Token::Token_IDENTIFIER)
		{
			uint32_t lId = lToken.mInteger;
			lToken = Token::next();
			if (lToken.mType == Token::Token_OPERATOR)
			{
				lToken = Token::next();
				if (lToken.mType == Token::Token_KEYWORD)
				{
					switch (lVersion)
					{
					case IFC2x3:
						parseIFC2x3(lToken, lId);
						break;
					case IFC4:
						parseIFC4(lToken, lId);
						break;
					}
				}
			}
		}
		else if (lToken.mType == Token::Token_KEYWORD)
		{
			if (lToken.mString == "ENDSEC")
			{
				break;
			}
		}
    }  
}

void IfcLoader::parseIFC4(Token& iToken, uint32_t iId)
{
	std::string lName = iToken.mString;

	Type::Enum lType = Type::FromString(lName);
	if (lType != Type::UNDEFINED)
	{
		IfcEntityInstanceData* lData = new IfcEntityInstanceData(lType, iId);
		std::vector<Argument*>& lAttributes = lData->attributes();
		iToken = Token::next();
		load(lAttributes);
		iToken = Token::next();

		invert(iId, lAttributes);

		IfcBaseClass::sDATA[iId] = SchemaEntity(lData);
	}
	else
	{
		BOOST_LOG_TRIVIAL(error) << "UNKNOWN Step file entry " << lName;
	}
}

ValueArgument* IfcLoader::parseIFC2x4dateAndTime(std::vector<Argument*>& iAttributes)
{
	std::string lIfcCalendarDate = mIfcCalendarDate[((ReferenceArgument*)iAttributes[0])->mId];
	std::string lIfcLocalTime = mIfcLocalTime[((ReferenceArgument*)iAttributes[1])->mId];

	Token lToken(Token::Token_STRING);
	lToken.mString = lIfcCalendarDate+"T"+lIfcLocalTime;
	return new ValueArgument(lToken);
}

std::string IfcLoader::parseIFC2x3duration(int iSeconds)
{
	int lY = iSeconds/31556952;
	int lM = (iSeconds/2592000)%12;
	int lD = (iSeconds/86400)%30;
	int lH = (iSeconds/3600)%60;
	int lm = (iSeconds/60)%60;
	int lS = iSeconds%60;
	char lBuffer[255];
	sprintf(lBuffer, "P%dY%dM%dDT%dH%dM%dS", lY, lM, lD, lH, lm, lS);

	return lBuffer;
}

void IfcLoader::parseIFC2x3(Token& iToken, uint32_t iId)
{
	std::string lName = iToken.mString;

	Type::Enum lType = Type::FromString(lName);
	if (lType != Type::UNDEFINED)
	{
		IfcEntityInstanceData* lData = new IfcEntityInstanceData(lType, iId);
		std::vector<Argument*>& lAttributes = lData->attributes();
		iToken = Token::next();
		load(lAttributes);
		iToken = Token::next();

		if (lName == "IFCTASK")
		{
			lAttributes[8] = new NullArgument();  // WorkMethod
			lAttributes[9] = new ValueArgument(mBoolFalse); // IsMilestone
			lAttributes.push_back(new NullArgument()); // Priority
			lAttributes.push_back(new NullArgument()); // TaskTime
			lAttributes.push_back(new NullArgument()); // PredefinedType
		}
		else if (lName == "IFCRELSEQUENCE")
		{
			if (dynamic_cast<ValueArgument*>(lAttributes[6]) && (int)*(ValueArgument*)lAttributes[6] != 0.0)
			{
				IfcEntityInstanceData* lLagTime = new IfcEntityInstanceData(Type::IfcLagTime, 0);
				std::vector<Argument*>& lLagTimeAttr = lLagTime->attributes();

				lLagTimeAttr.push_back(new NullArgument()); //Name
				lLagTimeAttr.push_back(new NullArgument()); //DataOrigin
				lLagTimeAttr.push_back(new NullArgument()); //UserDefinedDataOrigin

				Token lToken0(Token::Token_STRING);
				lToken0.mString = parseIFC2x3duration(abs((int)*(ValueArgument*)lAttributes[6]));
				IfcEntityInstanceData* lLagValue = new IfcEntityInstanceData(Type::IfcDuration, 0);
				std::vector<Argument*>& lLagValueAttr = lLagValue->attributes();
				lLagValueAttr.push_back(new ValueArgument(lToken0));

				lLagTimeAttr.push_back(new EntityArgument(SchemaEntity(lLagValue)));

				Token lToken(Token::Token_ENUMERATION);
				lToken.mString = "WORKTIME";
				lLagTimeAttr.push_back(new ValueArgument(lToken));

				lAttributes[6] = new EntityArgument(SchemaEntity(lLagTime));
			}
			else
			{
				lAttributes[6] = new NullArgument();
			}

			lAttributes.push_back(new NullArgument()); //    UserDefinedSequenceType
		}
		else if (lName == "IFCRELNESTS")
		{
			ReferenceArgument* lRelating = (ReferenceArgument*)lAttributes[4];
			if (IfcBaseClass::sDATA[lRelating->mId]->is(Type::IfcTask))
			{
				return;
			}
		}
		else  if (lName == "IFCWORKSCHEDULE")
		{
			if (!dynamic_cast<NullArgument*>(lAttributes[6]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[6];
				mCreationDate[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[11]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[11];
				mStartTime[lRef->mId] = iId;
			}
		}

		invert(iId, lAttributes);

		IfcBaseClass::sDATA[iId] = SchemaEntity(lData);
	}
	else
	{
		IfcEntityInstanceData* lData = new IfcEntityInstanceData(iId);
		std::vector<Argument*>& lAttributes = lData->attributes();
		iToken = Token::next();
		load(lAttributes);
		iToken = Token::next();

 		if (lName == "IFCRELASSIGNSTASKS")
		{
			//if (iId == 159834)
			{
				lData->type_ = Type::IfcRelAssignsToControl;
				invert(iId, lAttributes);
				IfcBaseClass::sDATA[iId] = SchemaEntity(lData);
			}


			// this is IFC2x3 -- the link from IfcScheduleTimeControl (converted to IfcTaskTime) to IfcTask is found here. 
			Argument* lTaskTime = lAttributes[7];

			ArgumentList& lList = (ArgumentList&)*lAttributes[4];
			for (unsigned int i=0; i<lList.size(); i++)
			{
				ReferenceArgument* lTask = (ReferenceArgument*)lList[i];
				IfcBaseClass::sDATA[lTask->mId]->entity->setArgument(11, lTaskTime);
			}
		}
		else  if (lName == "IFCSCHEDULETIMECONTROL")
		{
			if (!dynamic_cast<NullArgument*>(lAttributes[5]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[5];
				mActualStart[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[6]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[6];
				mEarlyStart[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[7]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[7];
				mLateStart[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[8]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[8];
				mScheduleStart[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[9]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[9];
				mActualFinish[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[12]))
			{
				ReferenceArgument* lRef = (ReferenceArgument*)lAttributes[12];
				mScheduleFinish[lRef->mId] = iId;
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[13]))
			{
				Token lToken(Token::Token_STRING);
				lToken.mString = parseIFC2x3duration(abs((int)*(ValueArgument*)lAttributes[13]));
				lAttributes[4] = new ValueArgument(lToken); // ScheduleDuration
			}
			else 
			{
				lAttributes[4] = new NullArgument(); // ScheduleDuration
			}

			lAttributes[0] = new NullArgument(); // Name
			lAttributes[1] = new NullArgument(); // DataOrigin
			lAttributes[2] = new NullArgument(); // UserDefinedDataOrigin
			lAttributes[3] = new NullArgument(); // DurationType
			//lAttributes[4] = new NullArgument(); // ScheduleDuration
			lAttributes[5] = new NullArgument(); // ScheduleStart
			lAttributes[6] = new NullArgument(); // ScheduleFinish
			lAttributes[7] = new NullArgument(); // EarlyStart
			lAttributes[8] = new NullArgument(); // EarlyFinish
			lAttributes[9] = new NullArgument(); // LateStart
			lAttributes[10] = new NullArgument(); // LateFinish
			lAttributes[11] = new NullArgument(); // FreeFloat
			lAttributes[12] = new NullArgument(); // TotalFloat
			lAttributes[13] = new NullArgument(); // IsCritical
			lAttributes[14] = new NullArgument(); // StatusTime
			lAttributes[15] = new NullArgument(); // ActualDuration
			lAttributes[16] = new NullArgument(); // ActualStart
			lAttributes[17] = new NullArgument(); // ActualFinish
			lAttributes[18] = new NullArgument(); // RemainingTime
			lAttributes[19] = lAttributes[22]; // Completion

			lData->type_ = Type::IfcTaskTime;
			IfcBaseClass::sDATA[iId] = SchemaEntity(lData);
		}
		else if (lName == "IFCDATEANDTIME")
		{
			if (mScheduleStart.find(iId) != mScheduleStart.end())
			{
				IfcTask* lTask = (IfcTask*)IfcBaseClass::sDATA[mScheduleStart[iId]];
				lTask->entity->attributes()[5] = parseIFC2x4dateAndTime(lAttributes);;
			}
			else if (mScheduleFinish.find(iId) != mScheduleFinish.end())
			{
				IfcTask* lTask = (IfcTask*)IfcBaseClass::sDATA[mScheduleFinish[iId]];
				lTask->entity->attributes()[6] = parseIFC2x4dateAndTime(lAttributes);;
			}
			else if (mEarlyStart.find(iId) != mEarlyStart.end())
			{
				IfcTask* lTask = (IfcTask*)IfcBaseClass::sDATA[mEarlyStart[iId]];
				lTask->entity->attributes()[7] = parseIFC2x4dateAndTime(lAttributes);;
			}
			else if (mLateStart.find(iId) != mLateStart.end())
			{
				IfcTask* lTask = (IfcTask*)IfcBaseClass::sDATA[mLateStart[iId]];
				lTask->entity->attributes()[9] = parseIFC2x4dateAndTime(lAttributes);;
			}
			else if (mActualStart.find(iId) != mActualStart.end())
			{
				IfcTask* lTask = (IfcTask*)IfcBaseClass::sDATA[mActualStart[iId]];
				lTask->entity->attributes()[16] = parseIFC2x4dateAndTime(lAttributes);
			}
			else if (mActualFinish.find(iId) != mActualFinish.end())
			{
				IfcTask* lTask = (IfcTask*)IfcBaseClass::sDATA[mActualFinish[iId]];
				lTask->entity->attributes()[17] = parseIFC2x4dateAndTime(lAttributes);
			}
			else if (mCreationDate.find(iId) != mCreationDate.end())
			{
				IfcWorkSchedule* lTask = (IfcWorkSchedule*)IfcBaseClass::sDATA[mCreationDate[iId]];
				lTask->entity->attributes()[6] = parseIFC2x4dateAndTime(lAttributes);
			}
			else if (mStartTime.find(iId) != mStartTime.end())
			{
				IfcWorkSchedule* lTask = (IfcWorkSchedule*)IfcBaseClass::sDATA[mStartTime[iId]];
				lTask->entity->attributes()[11] = parseIFC2x4dateAndTime(lAttributes);
			}




			else 
			{
				BOOST_LOG_TRIVIAL(error) << "NOT FOUND FOR  " << iId;
			}
		}
		else if (lName == "IFCLOCALTIME")
		{
			int lH = 0;
			int lM = 0;
			int lS = 0;
			if (!dynamic_cast<NullArgument*>(lAttributes[0]))
			{
				lH = *((ValueArgument*)lAttributes[0]);
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[1]))
			{
				lM = *((ValueArgument*)lAttributes[1]);
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[2]))
			{
				lS = *((ValueArgument*)lAttributes[2]);
			}

			char lBuffer[255];
			sprintf(lBuffer, "%02d:%02d:%02d", lH, lM, lS);
			mIfcLocalTime[iId] = lBuffer;
		}
		else if (lName == "IFCCALENDARDATE")
		{

			int lD = 0;
			int lM = 0;
			int lY = 0;
			if (!dynamic_cast<NullArgument*>(lAttributes[0]))
			{
				lD = *((ValueArgument*)lAttributes[0]);
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[1]))
			{
				lM = *((ValueArgument*)lAttributes[1]);
			}
			if (!dynamic_cast<NullArgument*>(lAttributes[2]))
			{
				lY = *((ValueArgument*)lAttributes[2]);
			}

			char lBuffer[255];
			sprintf(lBuffer, "%04d-%02d-%02d", lY, lM, lD);
			mIfcCalendarDate[iId] = lBuffer;
		}
		else
		{
			BOOST_LOG_TRIVIAL(error) << "UNKNOWN Step file entry " << lName;
		}
	}
}
	
IfcProject* IfcLoader::getProject()
{
	for (std::map<uint32_t, IfcBaseClass*>::iterator lIter = IfcBaseClass::sDATA.begin(); lIter != IfcBaseClass::sDATA.end(); lIter++)
	{
		if (lIter->second ->is(Type::IfcProject))
		{
			return lIter->second->as<IfcProject>();
		}
	}
	return 0;
}

IfcWorkPlan* IfcLoader::getWorkplan()
{
	for (std::map<uint32_t, IfcBaseClass*>::iterator lIter = IfcBaseClass::sDATA.begin(); lIter != IfcBaseClass::sDATA.end(); lIter++)
	{
		if (lIter->second ->is(Type::IfcWorkPlan))
		{
			return lIter->second->as<IfcWorkPlan>();
		}
	}
	return 0;
}





char Token::sLine[1024];
size_t Token::sPointer;
size_t Token::sLength;
FILE* Token::sFile;
char Token::sNext;

void Token::init(FILE* iFile)
{
	sFile = iFile;
	sLength = fread(sLine, 1, 1024, sFile);
	sPointer = 0;
	sNext = sLine[sPointer];
}

void Token::nextC()
{
	if (++sPointer == sLength)
	{
		sLength = fread(sLine, 1, 1024, sFile);
		sPointer = 0;
	}
	
	if (sPointer < sLength)
	{
		sNext = sLine[sPointer];
	}
	else
	{
		sNext = 0;
	}
}

Token Token::next() 
{
	// skip whitespace
	while (sNext && (sNext == ' ' || sNext == '\t' || sNext == '\r' || sNext == '\n'))
	{
		nextC();
	}

	// skipp comments
	if (sNext == '/')
	{
		nextC();
		if (sNext == '*')
		{
			while (1)
			{
				nextC();
				if (sNext == '*')
				{
					nextC();
					if (sNext == '/')
					{
						break;
					}
				}
			}
		}
	};

	Token lToken(Token_NONE);
	
	if (sNext == '(' || sNext == ')' || sNext == '=' || sNext == ',' || sNext == ';' || sNext == '$' || sNext == '*')
	{
		lToken.mChar = sNext;
		lToken.mType = Token_OPERATOR;
		nextC();
	}
	else if (sNext == '#')
	{
		lToken.mType = Token_IDENTIFIER;
		nextC();

		std::string lString;
		while (sNext >= '0' && sNext <= '9')
		{
			lString += sNext;
			nextC();
		}

		lToken.mInteger = std::stoi(lString);
	}
	else if (sNext == '.') 
	{
		std::string lString;

		nextC();
		while (sNext != '.') 
		{
			lString += sNext;
			nextC();
		}

		if (lString == "T") 
		{
			lToken.mType = Token_BOOL;
			lToken.mBoolean = true;
		}
		else if (lString == "F") 
		{
			lToken.mType = Token_BOOL;
			lToken.mBoolean = false;
		}
		else
		{
			lToken.mType = Token_ENUMERATION;
			lToken.mString = lString;
		}
		nextC();
	}
	else if (sNext == '\'')
	{
		std::string lString;

		bool lInString = true;
		nextC();
		do
		{
			if (sNext == '\'')
			{
				lInString = !lInString;
			}
			else if (!lInString)
			{
				if (sNext == ',' || sNext == ')')
				{
					break;
				}
			}
			lString += sNext;
			nextC();
		}
		while (sNext);
		
		lString.pop_back();
		lToken.mString = lString;
		lToken.mType = Token_STRING;
	}
	else if (sNext == '"')
	{
		lToken.mType = Token_BINARY;
	}
	else if (sNext >= '0' && sNext <= '9' || sNext == '-')
	{
		lToken.mType = Token_FLOAT;

		std::string lString;
		lString += sNext;

		nextC();
		while (sNext >= '0' && sNext <= '9' || sNext == '.' || sNext == 'E' || sNext == '-')
		{
			lString += sNext;
			nextC();
		}

		lToken.mDouble = std::stod(lString);
	}
	else
	{
		lToken.mType = Token_KEYWORD;

		std::string lString;
		while (sNext >= 'A' && sNext <= 'Z' || sNext >= '0' && sNext <= '9')
		{
			lString += sNext;
			nextC();
		}
		lToken.mString = lString;
	}	
	return lToken;
}
 

void Token::parseString(std::stringstream& iStream) 
{
	while (sLine[++sPointer] != '\'') 
	{
		iStream.put(sLine[sPointer]);
	}
}










EntityArgument::EntityArgument(IfcBaseClass* iEntity)
: mEntity(iEntity)
{
};
unsigned int EntityArgument::size() const { return 1; }
Argument* EntityArgument::operator [] (unsigned int ) const { throw IfcException("Argument is not a list of arguments"); }
std::string EntityArgument::toString(bool upper) const 
{ 
	return "12";
}
bool EntityArgument::isNull() const { return false; }

EntityArgument::operator IfcBaseClass*() const { 
	return mEntity;
}

void EntityArgument::toJson(json_spirit::mArray& iArray) const
{
	json_spirit::mObject lEntity;
	lEntity[Type::ToString(mEntity->type())] = mEntity->toJson();
	iArray.push_back(lEntity);
}


ArgumentType EntityArgument::type() const {
	return Argument_ENTITY_INSTANCE;
}


ReferenceArgument::ReferenceArgument(Token& iToken)
: mId(iToken.mInteger)
{
};
unsigned int ReferenceArgument::size() const { return 1; }
Argument* ReferenceArgument::operator [] (unsigned int ) const { throw IfcException("Argument is not a list of arguments"); }
std::string ReferenceArgument::toString(bool upper) const 
{ 
	return "12";
}
bool ReferenceArgument::isNull() const { return false; }

ArgumentType ReferenceArgument::type() const {
	return Argument_ENTITY_INSTANCE;
}

ReferenceArgument::operator IfcBaseClass*() const {
	return IfcBaseClass::sDATA[mId];
};

void ReferenceArgument::toJson(json_spirit::mArray& iArray) const
{
	iArray.push_back((long)mId);
}



ValueArgument::ValueArgument(Token& iToken)
: mToken(iToken)
{
};
unsigned int ValueArgument::size() const { return 1; }
Argument* ValueArgument::operator [] (unsigned int ) const { throw IfcException("Argument is not a list of arguments"); }
std::string ValueArgument::toString(bool upper) const 
{ 
	return "12";
}
bool ValueArgument::isNull() const { return false; }

ValueArgument::operator std::string() const { return mToken.mString; }
ValueArgument::operator int() const { return (int) mToken.mDouble; }
ValueArgument::operator double() const { return mToken.mDouble; }
ValueArgument::operator bool() const { return mToken.mBoolean; }

ArgumentType ValueArgument::type() const {

	switch (mToken.mType)
	{
		case Token::Token_NONE: return Argument_NULL;
		case Token::Token_STRING: return Argument_STRING;
		case Token::Token_INT: return Argument_INT; 
		case Token::Token_BOOL: return Argument_BOOL;
		case Token::Token_FLOAT: return Argument_DOUBLE;
		case Token::Token_BINARY: return Argument_BINARY;
	}
	return Argument_NULL;
}

void ValueArgument::toJson(json_spirit::mArray& iArray) const
{
	switch (mToken.mType)
	{
		case Token::Token_NONE: 
			iArray.push_back(json_spirit::Value_impl<json_spirit::mConfig>::null); 
			break;
		case Token::Token_STRING:
			iArray.push_back(mToken.mString); 
			break;
		case Token::Token_INT:			
			iArray.push_back(mToken.mInteger); 
			break;
		case Token::Token_BOOL:			
			iArray.push_back(mToken.mBoolean); 
			break;
		case Token::Token_FLOAT:			
			iArray.push_back(mToken.mDouble); 
			break;
		case Token::Token_BINARY:			
			iArray.push_back(mToken.mString); 
			break;
		default:
			iArray.push_back(json_spirit::Value_impl<json_spirit::mConfig>::null); 
			break;
	}
}



NullArgument::NullArgument()
{
};
unsigned int NullArgument::size() const { return 1; }
Argument* NullArgument::operator [] (unsigned int ) const { throw IfcException("Argument is not a list of arguments"); }
std::string NullArgument::toString(bool upper) const 
{ 
	return "NULL";
}
bool NullArgument::isNull() const { return true; }

NullArgument::operator IfcEntityList::ptr() const { 
	IfcEntityList::ptr l ( new IfcEntityList() );
	return l;
}


ArgumentType NullArgument::type() const {
	return Argument_NULL;
}

void NullArgument::toJson(json_spirit::mArray& iArray) const
{
	iArray.push_back(json_spirit::Value_impl<json_spirit::mConfig>::null); 
}



// templated helper function for reading arguments into a list
template<typename T>
std::vector<T> read_aggregate_as_vector(const std::vector<Argument*>& list) {
	std::vector<T> return_value;
	return_value.reserve(list.size());
	std::vector<Argument*>::const_iterator it = list.begin();
	for (; it != list.end(); ++it) {
		return_value.push_back(**it);
	}
	return return_value;
}
template<typename T>
std::vector< std::vector<T> > read_aggregate_of_aggregate_as_vector2(const std::vector<Argument*>& list) {
	std::vector< std::vector<T> > return_value;
	return_value.reserve(list.size());
	std::vector<Argument*>::const_iterator it = list.begin();
	for (; it != list.end(); ++it) {
		return_value.push_back(**it);
	}
	return return_value;
}

//
// Functions for casting the ArgumentList to other types
//
ArgumentList::operator std::vector<double>() const {
	return read_aggregate_as_vector<double>(mList);
}

ArgumentList::operator std::vector<int>() const {
	return read_aggregate_as_vector<int>(mList);
}

ArgumentList::operator std::vector<std::string>() const {
	return read_aggregate_as_vector<std::string>(mList);
}

ArgumentList::operator std::vector<boost::dynamic_bitset<> >() const {
	return read_aggregate_as_vector<boost::dynamic_bitset<> >(mList);
}

ArgumentList::operator IfcEntityList::ptr() const {
	IfcEntityList::ptr l ( new IfcEntityList() );
	std::vector<Argument*>::const_iterator it;
	for ( it = mList.begin(); it != mList.end(); ++ it ) {
		// FIXME: account for $
		IfcBaseClass* entity = **it;
		l->push(entity);
	}
	return l;
}

ArgumentList::operator std::vector< std::vector<int> >() const {
	return read_aggregate_of_aggregate_as_vector2<int>(mList);
}

ArgumentList::operator std::vector< std::vector<double> >() const {
	return read_aggregate_of_aggregate_as_vector2<double>(mList);
}

ArgumentList::operator IfcEntityListList::ptr() const {
	IfcEntityListList::ptr l ( new IfcEntityListList() );
	std::vector<Argument*>::const_iterator it;
	for ( it = mList.begin(); it != mList.end(); ++ it ) {
		const Argument* arg = *it;
		const ArgumentList* arg_list;
		if ((arg_list = dynamic_cast<const ArgumentList*>(arg)) != 0) {
			IfcEntityList::ptr e = *arg_list;
			l->push(e);
		}
	}
	return l;
}

unsigned int ArgumentList::size() const { return (unsigned int) mList.size(); }

Argument* ArgumentList::operator [] (unsigned int i) const {
	if ( i >= mList.size() ) {
		throw IfcAttributeOutOfRangeException("Argument index out of range");
	}
	return mList[i];
}

void ArgumentList::set(unsigned int i, Argument* argument) {
	while (size() < i) {
		push(new NullArgument());
	}
	if (i < size()) {
		delete mList[i];
		mList[i] = argument;
	} else {
		mList.push_back(argument);
	}	
}

std::string ArgumentList::toString(bool upper) const {
	std::stringstream ss;
	ss << "(";
	for( std::vector<Argument*>::const_iterator it = mList.begin(); it != mList.end(); it ++ ) {
		if ( it != mList.begin() ) ss << ",";
		ss << (*it)->toString(upper);
	}
	ss << ")";
	return ss.str();
}

void ArgumentList::toJson(json_spirit::mArray& iArray) const
{
	json_spirit::mArray lArray;
	for( std::vector<Argument*>::const_iterator it = mList.begin(); it != mList.end(); it ++ ) 
	{
		(*it)->toJson(lArray);
	}
	iArray.push_back(lArray); 
}

bool ArgumentList::isNull() const { return false; }

ArgumentList::~ArgumentList() {
	for( std::vector<Argument*>::iterator it = mList.begin(); it != mList.end(); it ++ ) {
		delete (*it);
	}
	mList.clear();
}


ArgumentType ArgumentList::type() const {
	if (mList.empty()) {
		return Argument_EMPTY_AGGREGATE;
	}
	const ArgumentType elem_type = mList[0]->type();
	if (elem_type == Argument_INT) {
		return Argument_AGGREGATE_OF_INT;
	} else if (elem_type == Argument_DOUBLE) {
		return Argument_AGGREGATE_OF_DOUBLE;
	} else if (elem_type == Argument_STRING) {
		return Argument_AGGREGATE_OF_STRING;
	} else if (elem_type == Argument_BINARY) {
		return Argument_AGGREGATE_OF_BINARY;
	} else if (elem_type == Argument_ENTITY_INSTANCE) {
		return Argument_AGGREGATE_OF_ENTITY_INSTANCE;
	} else if (elem_type == Argument_AGGREGATE_OF_INT) {
		return Argument_AGGREGATE_OF_AGGREGATE_OF_INT;
	} else if (elem_type == Argument_AGGREGATE_OF_DOUBLE) {
		return Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE;
	} else if (elem_type == Argument_AGGREGATE_OF_ENTITY_INSTANCE) {
		return Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE;
	} else if (elem_type == Argument_EMPTY_AGGREGATE) {
		return Argument_AGGREGATE_OF_EMPTY_AGGREGATE;
	} else {
		return Argument_UNKNOWN;
	}
}

void ArgumentList::push(Argument* l) {
	mList.push_back(l);
}
