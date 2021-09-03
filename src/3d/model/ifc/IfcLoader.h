#pragma once

#include "Ifc4.h"
#include "IfcBindings.h"


class Token 
{
	public:

		typedef enum TokenType {
			Token_NONE,
			Token_STRING,
			Token_IDENTIFIER,
			Token_OPERATOR,
			Token_ENUMERATION,
			Token_KEYWORD,
			Token_INT,
			Token_BOOL,
			Token_FLOAT,
			Token_BINARY
		} TokenType;

		Token(TokenType iType)
		: mType(iType)
		, mBoolean(false)
		, mChar('\0')
		, mInteger(0)
		, mDouble(0.0)
		, mString("")
		{
		};

		Token(const Token& iToken)
		: mType(iToken.mType)
		, mBoolean(iToken.mBoolean)
		, mChar(iToken.mChar)
		, mInteger(iToken.mInteger)
		, mDouble(iToken.mDouble)
		, mString(iToken.mString)
		{
		};

		Token& operator=(const  Token& iToken) 
		{
			mType = iToken.mType;
			mBoolean = iToken.mBoolean;
			mChar = iToken.mChar;
			mInteger = iToken.mInteger;
			mDouble = iToken.mDouble;
			mString = iToken.mString;
			return *this;
		};

		TokenType mType;

		bool isOperator(char iOperator)
		{
			return mType == Token_OPERATOR && mChar == iOperator;
		}

		bool mBoolean;
		char mChar; 
		int mInteger; 
		double mDouble; 
		std::string mString;

		static void init(FILE* iFile);
		static Token next();
		static char sNext;

	private:

		static void parseString(std::stringstream& iValue);
		static void nextC();

		static char sLine[1024];
		static size_t sLength;
		static size_t sPointer;
		static FILE* sFile;
};

class EntityArgument : public Argument 
{
	public:

		EntityArgument(IfcBaseClass* iEntity);

		bool isNull() const;
		unsigned int size() const;

		Argument* operator [] (unsigned int i) const;
		std::string toString(bool upper=false) const;
		ArgumentType type() const;
		operator IfcBaseClass*() const;
		void toJson(json_spirit::mArray& iArray) const;

	private:		

		IfcBaseClass* mEntity;
};

class ReferenceArgument : public Argument 
{
	public:

		ReferenceArgument(Token& iToken);

		bool isNull() const;
		unsigned int size() const;

		Argument* operator [] (unsigned int i) const;
		std::string toString(bool upper=false) const;
		ArgumentType type() const;
		operator IfcBaseClass*() const;
		void toJson(json_spirit::mArray& iArray) const;

		uint32_t mId;
};

class ValueArgument : public Argument 
{
	public:

		ValueArgument(Token& iToken);

		bool isNull() const;
		unsigned int size() const;
		ArgumentType type() const;

		Argument* operator [] (unsigned int i) const;
		std::string toString(bool upper=false) const;
		void toJson(json_spirit::mArray& iArray) const;

		operator std::string() const;
		operator int() const;
		operator double() const;
		operator bool() const;

		Token mToken;
};

class NullArgument : public Argument 
{
	public:

		NullArgument();

		bool isNull() const;
		unsigned int size() const;
		ArgumentType type() const;

		Argument* operator [] (unsigned int i) const;
		std::string toString(bool upper=false) const;
		operator IfcEntityList::ptr() const;
		void toJson(json_spirit::mArray& iArray) const;

	private:		
};

class ArgumentList: public Argument 
{
	public:
		~ArgumentList();

		
		ArgumentType type() const;
		
		operator std::vector<int>() const;
		operator std::vector<double>() const;
		operator std::vector<std::string>() const;
		operator std::vector<boost::dynamic_bitset<> >() const;
		operator IfcEntityList::ptr() const;

		operator std::vector< std::vector<int> >() const;
		operator std::vector< std::vector<double> >() const;
		operator IfcEntityListList::ptr() const;
		
		bool isNull() const;
		unsigned int size() const;

		Argument* operator [] (unsigned int i) const;
		void set(unsigned int i, Argument*);
		std::string toString(bool upper=false) const;
		std::vector<Argument*>& arguments() { return mList; }
		void toJson(json_spirit::mArray& iArray) const;

	private:

		std::vector<Argument*> mList;
		void push(Argument* l);
};





class IfcLoader
{
	public:

		typedef enum 
		{
			IFC2x3,
			IFC4
		} Verison;

		IfcLoader();

		void process(std::string iFile);

		IfcProject* getProject();
		IfcWorkPlan* getWorkplan();
	
	protected:

		void parseIFC4(Token& iToken, uint32_t iId);	
		void parseIFC2x3(Token& iToken, uint32_t iId);
		std::string parseIFC2x3duration(int iSeconds);
		ValueArgument* parseIFC2x4dateAndTime(std::vector<Argument*>& iAttributes);

		void invert(uint32_t iId, std::vector<Argument*>& iAttributes);
		void load(std::vector<Argument*>& iAttributes);

		Token mInt0;
		Token mBoolFalse;



		// TaskTime
		std::map<uint32_t, uint32_t> mScheduleFinish;
		std::map<uint32_t, uint32_t> mScheduleStart;
		std::map<uint32_t, uint32_t> mActualStart;
		std::map<uint32_t, uint32_t> mActualFinish;
		std::map<uint32_t, uint32_t> mEarlyStart;
		std::map<uint32_t, uint32_t> mLateStart;

		// WorkSchedule
		std::map<uint32_t, uint32_t> mCreationDate;
		std::map<uint32_t, uint32_t> mStartTime;


		std::map<uint32_t, std::string> mIfcCalendarDate;
		std::map<uint32_t, std::string> mIfcLocalTime;
	
};

			