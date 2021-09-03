#include <set>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <ctime>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>

#include "IfcBindings.h"


std::map<uint32_t, IfcBaseClass*> IfcBaseClass::sDATA;
std::map<uint32_t, std::vector<int>> IfcBaseClass::sINVERSE;

std::string IfcBaseClass::toString() const
{
	return entity->toString();
};

json_spirit::mArray IfcBaseClass::toJson()
{
	return entity->toJson();
}



json_spirit::mArray IfcEntityInstanceData::toJson() const 
{
	json_spirit::mArray lArray;

	std::vector<Argument*>::const_iterator it = attributes_.begin();
	lArray.push_back(json_spirit::Value_impl<json_spirit::mConfig>::null);  // omit GlobalId its too long
	for (it++; it != attributes_.end(); it++) 
	{
		(*it)->toJson(lArray);
	}

	return lArray;
}




std::string IfcEntityInstanceData::toString(bool upper) const 
{
	std::stringstream ss;
	ss.imbue(std::locale::classic());
	
	std::string dt = Type::ToString(type());

	if (!Type::IsSimple(type()) || id_ != 0) {
		ss << "#" << id_ << "=";
	}

	ss << dt << "(";
	std::vector<Argument*>::const_iterator it = attributes_.begin();
	for (; it != attributes_.end(); ++it) {
		if (it != attributes_.begin()) {
			ss << ",";
		}
		ss << (*it)->toString(upper);
	}
	ss << ")";

	return ss.str();
}

IfcEntityInstanceData::~IfcEntityInstanceData() 
{
	for (std::vector<Argument*>::const_iterator it = attributes_.begin(); it != attributes_.end(); ++it) 
	{
		delete *it;
	}
}


#include "IfcLoader.h"

Argument* IfcEntityInstanceData::getArgument(unsigned int i) const 
{
	if (i < attributes_.size()) 
	{
		return attributes_[i];
	} 
	else 
	{
		static NullArgument sNull;
		return &sNull;
		//throw IfcException("Attribute index out of range");
	}
}

void IfcEntityInstanceData::setArgument(unsigned int i, Argument* a, ArgumentType attr_type) 
{ 
	attributes_[i] = a; 
};

#include "IfcLoader.h" // TODO fix this ..

IfcEntityList::ptr IfcEntityInstanceData::getInverse(Type::Enum type, int attribute_index) 
{
	IfcEntityList::ptr l = IfcEntityList::ptr(new IfcEntityList);
	for (std::vector<int>::iterator lIter = IfcBaseClass::sINVERSE[id_].begin(); lIter < IfcBaseClass::sINVERSE[id_].end(); lIter++)
	{
		IfcBaseClass* lInstance = IfcBaseClass::sDATA[*lIter];
		if (lInstance->is(type))
		{
			Argument* lArgument = lInstance->entity->getArgument(attribute_index);

			if (lArgument->type() == Argument_ENTITY_INSTANCE) 
			{
				ReferenceArgument* lReference = (ReferenceArgument*)lArgument;
				if (lReference->mId == id_)
				{
					l->push(lInstance);
				}
			} 
			else if (lArgument->type() == Argument_AGGREGATE_OF_ENTITY_INSTANCE) 
			{
				IfcEntityList::ptr li = *lArgument;
				if (li->contains(IfcBaseClass::sDATA[id_]))
				{
					l->push(lInstance);
				}
			} 
			else if (lArgument->type() == Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE) 
			{
				IfcEntityListList::ptr li = *lArgument;
				if (li->contains(IfcBaseClass::sDATA[id_]))
				{
					l->push(lInstance);
				}
			}
		}
	}

	return l;
}












void IfcEntityList::push(IfcBaseClass* l) {
	if (l) {
		ls.push_back(l);
	}
}
void IfcEntityList::push(const IfcEntityList::ptr& l) {
	if (l) {
		for( it i = l->begin(); i != l->end(); ++i  ) {
			if ( *i ) ls.push_back(*i);
		}
	}
}
unsigned int IfcEntityList::size() const { return (unsigned int) ls.size(); }
void IfcEntityList::reserve(unsigned capacity) { ls.reserve((size_t)capacity); }
IfcEntityList::it IfcEntityList::begin() { return ls.begin(); }
IfcEntityList::it IfcEntityList::end() { return ls.end(); }
IfcBaseClass* IfcEntityList::operator[] (int i) {
	return ls[i];
}
bool IfcEntityList::contains(IfcBaseClass* instance) const {
	return std::find(ls.begin(), ls.end(), instance) != ls.end();
}
void IfcEntityList::remove(IfcBaseClass* instance) {
	std::vector<IfcBaseClass*>::iterator it;
	while ((it = std::find(ls.begin(), ls.end(), instance)) != ls.end()) {
		ls.erase(it);
	}
}

IfcEntityList::ptr IfcEntityList::filtered(const std::set<Type::Enum>& entities) {
	IfcEntityList::ptr return_value(new IfcEntityList);
	for (it it = begin(); it != end(); ++it) {
		bool contained = false;
		for (std::set<Type::Enum>::const_iterator jt = entities.begin(); jt != entities.end(); ++jt) {
			if ((*it)->is(*jt)) {
				contained = true;
				break;
			}
		}
		if (!contained) {
			return_value->push(*it);
		}
	}	
	return return_value;
}

IfcEntityList::ptr IfcEntityList::unique() {
	std::set<IfcBaseClass*> encountered;
	IfcEntityList::ptr return_value(new IfcEntityList);
	for (it it = begin(); it != end(); ++it) {
		if (encountered.find(*it) == encountered.end()) {
			return_value->push(*it);
			encountered.insert(*it);
		}
	}
	return return_value;
} 


unsigned int IfcBaseType::getArgumentCount() const { return 1; }
Argument* IfcBaseType::getArgument(unsigned int i) const { return entity->getArgument(i); }
const char* IfcBaseType::getArgumentName(unsigned int i) const { if (i == 0) { return "wrappedValue"; } else { throw IfcAttributeOutOfRangeException("Argument index out of range"); } }


//Note: some of these methods are overloaded in derived classes
Argument::operator int() const { throw IfcException("Argument is not an integer"); }
Argument::operator bool() const { throw IfcException("Argument is not a boolean"); }
Argument::operator double() const { throw IfcException("Argument is not a number"); }
Argument::operator std::string() const { throw IfcException("Argument is not a string"); }
Argument::operator boost::dynamic_bitset<>() const { throw IfcException("Argument is not a binary"); }
Argument::operator IfcBaseClass*() const { 

	throw IfcException("Argument is not an entity instance"); 
}
Argument::operator std::vector<double>() const { throw IfcException("Argument is not a list of floats"); }
Argument::operator std::vector<int>() const { throw IfcException("Argument is not a list of ints"); }
Argument::operator std::vector<std::string>() const { throw IfcException("Argument is not a list of strings"); }
Argument::operator std::vector<boost::dynamic_bitset<> >() const { throw IfcException("Argument is not a list of binaries"); }
Argument::operator IfcEntityList::ptr() const { throw IfcException("Argument is not a list of entity instances"); }
Argument::operator std::vector< std::vector<int> >() const { throw IfcException("Argument is not a list of list of ints"); }
Argument::operator std::vector< std::vector<double> >() const { throw IfcException("Argument is not a list of list of floats"); }
Argument::operator IfcEntityListList::ptr() const { throw IfcException("Argument is not a list of list of entity instances"); }