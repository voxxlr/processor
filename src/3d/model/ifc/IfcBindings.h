#pragma once

#include <set>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

#include "Ifc4enum.h"

#include <boost/shared_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/blank.hpp>

#include "json_spirit/json_spirit_reader_template.h"
#include "json_spirit/json_spirit_writer_template.h"

enum ArgumentType {
	Argument_NULL,
	Argument_DERIVED,
	Argument_INT,
	Argument_BOOL,
	Argument_DOUBLE,
	Argument_STRING,
	Argument_BINARY,
	Argument_ENUMERATION,
	Argument_ENTITY_INSTANCE,

	Argument_EMPTY_AGGREGATE,
	Argument_AGGREGATE_OF_INT,
	Argument_AGGREGATE_OF_DOUBLE,
	Argument_AGGREGATE_OF_STRING,
	Argument_AGGREGATE_OF_BINARY,
	Argument_AGGREGATE_OF_ENTITY_INSTANCE,

	Argument_AGGREGATE_OF_EMPTY_AGGREGATE,
	Argument_AGGREGATE_OF_AGGREGATE_OF_INT,
	Argument_AGGREGATE_OF_AGGREGATE_OF_DOUBLE,
	Argument_AGGREGATE_OF_AGGREGATE_OF_ENTITY_INSTANCE,

	Argument_UNKNOWN
};


class IfcEntityList;
class IfcEntityInstanceData;
class Argument;

class IfcBaseClass 
{
	public:

		static std::map<uint32_t, IfcBaseClass*> sDATA;
		static std::map<uint32_t, std::vector<int>> sINVERSE;

		virtual ~IfcBaseClass() {}
		virtual bool is(Type::Enum v) const = 0;
		virtual Type::Enum type() const = 0;

		virtual unsigned int getArgumentCount() const = 0;
		virtual ArgumentType getArgumentType(unsigned int i) const = 0;
		virtual Type::Enum getArgumentEntity(unsigned int i) const = 0;
		virtual Argument* getArgument(unsigned int i) const = 0;
		virtual const char* getArgumentName(unsigned int i) const = 0;

		template <class T>
		T* as() {
			return is(T::Class())
				? static_cast<T*>(this)
				: static_cast<T*>(0);
		}

		template <class T>
		const T* as() const {
			return is(T::Class())
				? static_cast<const T*>(this)
				: static_cast<const T*>(0);
		}

		std::string toString() const;

		IfcEntityInstanceData* entity;

		virtual json_spirit::mArray toJson();

};





class IfcEntityInstanceData 
{
	friend class IfcLoader;

	protected:

		unsigned id_;
		Type::Enum type_;
		mutable std::vector<Argument*> attributes_;

	public:

		IfcEntityInstanceData(unsigned id)
		: id_(id)
		{}

		IfcEntityInstanceData(Type::Enum type, unsigned id)
		: id_(id), type_(type)
		{}

		IfcEntityInstanceData(Type::Enum type)
		: type_(type)
		{}

		~IfcEntityInstanceData();

		boost::shared_ptr<IfcEntityList> getInverse(Type::Enum type, int attribute_index);

		Argument* getArgument(unsigned int i) const;
		void setArgument(unsigned int i, Argument* a, ArgumentType attr_type = Argument_UNKNOWN);

		unsigned int getArgumentCount() const { return (unsigned int)attributes_.size(); }
		Type::Enum type() const { return type_; }
		std::string toString(bool upper = false) const;
		json_spirit::mArray toJson() const;
		unsigned int id() const { return id_; }
		std::vector<Argument*>& attributes() const { return attributes_; }
};







class IfcBaseEntity : public IfcBaseClass 
{
	public:

		Argument* getArgumentByName(const std::string& name) const;
		std::vector<std::string> getAttributeNames() const;
		std::vector<std::string> getInverseAttributeNames() const;
		unsigned id() const { return entity->id(); }
};

class IfcBaseType : public IfcBaseEntity 
{
	public:

		unsigned int getArgumentCount() const;
		Argument* getArgument(unsigned int i) const;
		const char* getArgumentName(unsigned int i) const;
		Type::Enum getArgumentEntity(unsigned int /*i*/) const { return Type::UNDEFINED; }
};








template <class T> class IfcTemplatedEntityList;

class IfcEntityList 
{
	private: 
		std::vector<IfcBaseClass*> ls;

	public:
		typedef boost::shared_ptr<IfcEntityList> ptr;
		typedef std::vector<IfcBaseClass*>::const_iterator it;
		void push(IfcBaseClass* l);
		void push(const ptr& l);
		it begin();
		it end();
		IfcBaseClass* operator[] (int i);
		unsigned int size() const;
		void reserve(unsigned capacity);
		bool contains(IfcBaseClass*) const;
		template <class U>
		typename U::list::ptr as() {
			typename U::list::ptr r(new typename U::list);
			const bool all = U::Class() == Type::UNDEFINED;
			for (it i = begin(); i != end(); ++i) if (all || (*i)->is(U::Class())) r->push((U*)*i);
			return r;
		}
		void remove(IfcBaseClass*);
		IfcEntityList::ptr filtered(const std::set<Type::Enum>& entities);
		IfcEntityList::ptr unique();
};

template <class T> class IfcTemplatedEntityList 
{
	private:
		std::vector<T*> ls;

	public:
		typedef boost::shared_ptr< IfcTemplatedEntityList<T> > ptr;
		typedef typename std::vector<T*>::const_iterator it;
		void push(T* t) { if (t) { ls.push_back(t); } }
		void push(ptr t) { if (t) { for (typename T::list::it it = t->begin(); it != t->end(); ++it) push(*it); } }
		it begin() { return ls.begin(); }
		it end() { return ls.end(); }
		unsigned int size() const { return (unsigned int)ls.size(); }
		IfcEntityList::ptr generalize() {
			IfcEntityList::ptr r(new IfcEntityList());
			for (it i = begin(); i != end(); ++i) r->push(*i);
			return r;
		}
		bool contains(T* t) const { return std::find(ls.begin(), ls.end(), t) != ls.end(); }
		template <class U>
		typename U::list::ptr as() {
			typename U::list::ptr r(new typename U::list);
			const bool all = U::Class() == Type::UNDEFINED;
			for (it i = begin(); i != end(); ++i) if (all || (*i)->is(U::Class())) r->push((U*)*i);
			return r;
		}
		void remove(T* t) {
			typename std::vector<T*>::iterator it;
			while ((it = std::find(ls.begin(), ls.end(), t)) != ls.end()) {
				ls.erase(it);
			}
		}
};

template <class T> class IfcTemplatedEntityListList;

class IfcEntityListList 
{
	private:
		std::vector< std::vector<IfcBaseClass*> > ls;

	public:
		typedef boost::shared_ptr< IfcEntityListList > ptr;
		typedef std::vector< std::vector<IfcBaseClass*> >::const_iterator outer_it;
		typedef std::vector<IfcBaseClass*>::const_iterator inner_it;
		void push(const std::vector<IfcBaseClass*>& l) {
			ls.push_back(l);
		}
		void push(const IfcEntityList::ptr& l) {
			if (l) {
				std::vector<IfcBaseClass*> li;
				for (std::vector<IfcBaseClass*>::const_iterator jt = l->begin(); jt != l->end(); ++jt) {
					li.push_back(*jt);
				}
				push(li);
			}
		}
		outer_it begin() const { return ls.begin(); }
		outer_it end() const { return ls.end(); }
		int size() const { return (int)ls.size(); }
		int totalSize() const {
			int accum = 0;
			for (outer_it it = begin(); it != end(); ++it) {
				accum += (int)it->size();
			}
			return accum;
		}
		bool contains(IfcBaseClass* instance) const {
			for (outer_it it = begin(); it != end(); ++it) {
				const std::vector<IfcBaseClass*>& inner = *it;
				if (std::find(inner.begin(), inner.end(), instance) != inner.end()) {
					return true;
				}
			}
			return false;
		}
		template <class U>
		typename IfcTemplatedEntityListList<U>::ptr as() {
			typename IfcTemplatedEntityListList<U>::ptr r(new IfcTemplatedEntityListList<U>);
			const bool all = U::Class() == Type::UNDEFINED;
			for (outer_it outer = begin(); outer != end(); ++outer) {
				const std::vector<IfcBaseClass*>& from = *outer;
				typename std::vector<U*> to;
				for (inner_it inner = from.begin(); inner != from.end(); ++inner) {
					if (all || (*inner)->is(U::Class())) to.push_back((U*)*inner);
				}
				r->push(to);
			}
			return r;
		}
};

template <class T> class IfcTemplatedEntityListList 
{
	private:
		std::vector< std::vector<T*> > ls;

	public:
		typedef typename boost::shared_ptr< IfcTemplatedEntityListList<T> > ptr;
		typedef typename std::vector< std::vector<T*> >::const_iterator outer_it;
		typedef typename std::vector<T*>::const_iterator inner_it;
		void push(const std::vector<T*>& t) { ls.push_back(t); }
		outer_it begin() { return ls.begin(); }
		outer_it end() { return ls.end(); }
		int size() const { return (int)ls.size(); }
		int totalSize() const {
			int accum = 0;
			for (outer_it it = begin(); it != end(); ++it) {
				accum += it->size();
			}
			return accum;
		}
		bool contains(T* t) const {
			for (outer_it it = begin(); it != end(); ++it) {
				const std::vector<T*>& inner = *it;
				if (std::find(inner.begin(), inner.end(), t) != inner.end()) {
					return true;
				}
			}
			return false;
		}
		IfcEntityListList::ptr generalize() {
			IfcEntityListList::ptr r(new IfcEntityListList());
			for (outer_it outer = begin(); outer != end(); ++outer) {
				const std::vector<T*>& from = *outer;
				std::vector<IfcBaseClass*> to;
				for (inner_it inner = from.begin(); inner != from.end(); ++inner) {
					to.push_back(*inner);
				}
				r->push(to);
			}
			return r;
		}
};





class Argument 
{
	public:
		virtual operator int() const;
		virtual operator bool() const;
		virtual operator double() const;
		virtual operator std::string() const;
		virtual operator boost::dynamic_bitset<>() const;
		virtual operator IfcBaseClass*() const;

		virtual operator std::vector<int>() const;
		virtual operator std::vector<double>() const;
		virtual operator std::vector<std::string>() const;
		virtual operator std::vector<boost::dynamic_bitset<> >() const;
		virtual operator IfcEntityList::ptr() const;

		virtual operator std::vector< std::vector<int> >() const;
		virtual operator std::vector< std::vector<double> >() const;
		virtual operator IfcEntityListList::ptr() const;

		virtual bool isNull() const = 0;
		virtual unsigned int size() const = 0;

		virtual ArgumentType type() const = 0;
		virtual Argument* operator [] (unsigned int i) const = 0;
		virtual std::string toString(bool upper=false) const = 0;
		virtual void toJson(json_spirit::mArray& iArray) const = 0;
	
		virtual ~Argument() {};
};














class IfcException : public std::exception {
private:
	std::string message;
public:
	IfcException(const std::string& m)
			: message(m) {}
	virtual ~IfcException () throw () {}
	virtual const char* what() const throw() {
		return message.c_str(); 
	}
};

class IfcAttributeOutOfRangeException : public IfcException {
public:
	IfcAttributeOutOfRangeException(const std::string& e)
		: IfcException(e) {}
	~IfcAttributeOutOfRangeException () throw () {}
};

class IfcInvalidTokenException : public IfcException {
public:
	IfcInvalidTokenException(
		int token_start,
		const std::string& token_string,
		const std::string& expected_type
	)
		: IfcException(
			std::string("Token ") + token_string + " at " + 
			boost::lexical_cast<std::string>(token_start) + 
			" invalid " + expected_type
		)
	{}
	IfcInvalidTokenException(
		int token_start,
		char c
	)
		: IfcException(
			std::string("Unexpected '") + std::string(1, c) + "' at " +
			boost::lexical_cast<std::string>(token_start)
		)
	{}
	~IfcInvalidTokenException() throw () {}
};



class IfcEnumerationDescriptor 
{
	private:
		Type::Enum type;
		std::vector<std::string> values;

	public:
		IfcEnumerationDescriptor(Type::Enum type, const std::vector<std::string>& values)
			: type(type), values(values) {}
		std::pair<const char*, int> getIndex(const std::string& value) const {
			std::vector<std::string>::const_iterator it = std::find(values.begin(), values.end(), value);
			if (it != values.end()) {
				return std::make_pair(it->c_str(), (int)std::distance(it, values.begin()));
			} else {
				throw IfcException("Invalid enumeration value");
			}
		}
		const std::vector<std::string>& getValues() {
			return values;
		}
		Type::Enum getType() {
			return type;
		}
};

class IfcEntityDescriptor 
{
	public:

		class IfcArgumentDescriptor
		{
			public:
				std::string name;
				bool optional;
				ArgumentType argument_type;
				Type::Enum data_type;
				IfcArgumentDescriptor(const std::string& name, bool optional, ArgumentType argument_type, Type::Enum data_type)
					: name(name), optional(optional), argument_type(argument_type), data_type(data_type) {}
		};

	private:

		Type::Enum type;
		IfcEntityDescriptor* parent;
		std::vector<IfcArgumentDescriptor> arguments;
		unsigned argument_start() const {
			return parent ? parent->getArgumentCount() : 0;
		}
		const IfcArgumentDescriptor& get_argument(unsigned i) const {
			if (i < arguments.size()) return arguments[i];
			else throw IfcAttributeOutOfRangeException("Argument index out of range");
		}
	public:

		IfcEntityDescriptor(Type::Enum type, IfcEntityDescriptor* parent)
			: type(type), parent(parent) {}
		void add(const std::string& name, bool optional, ArgumentType argument_type, Type::Enum data_type = Type::UNDEFINED) {
			arguments.push_back(IfcArgumentDescriptor(name, optional, argument_type, data_type));
		}
		unsigned getArgumentCount() const {
			return (parent ? parent->getArgumentCount() : 0) + (unsigned)arguments.size();
		}
		const std::string& getArgumentName(unsigned i) const {
			const unsigned a = argument_start();
			return i < a
				? parent->getArgumentName(i)
				: get_argument(i-a).name;
		}
		ArgumentType getArgumentType(unsigned i) const {
			const unsigned a = argument_start();
			return i < a
				? parent->getArgumentType(i)
				: get_argument(i-a).argument_type;
		}
		bool getArgumentOptional(unsigned i) const {
			const unsigned a = argument_start();
			return i < a
				? parent->getArgumentOptional(i)
				: get_argument(i-a).optional;
		}
		Type::Enum getArgumentEntity(unsigned i) const {
			const unsigned a = argument_start();
			return i < a
				? parent->getArgumentEntity(i)
				: get_argument(i-a).data_type;
		}
		unsigned getArgumentIndex(const std::string& s) const {
			unsigned a = argument_start();
			for(std::vector<IfcArgumentDescriptor>::const_iterator i = arguments.begin(); i != arguments.end(); ++i) {
				if (i->name == s) return a;
				a++;
			}
			if (parent) return parent->getArgumentIndex(s);
			throw IfcException(std::string("Argument ") + s + " not found on " + Type::ToString(type));
		}		
	};   









class IfcWriteArgument : public Argument
{
	public: 
		IfcWriteArgument () {};

		operator int() const { throw IfcException("Unimplemented"); }
		operator bool() const { throw IfcException("Unimplemented"); }
		operator double() const { throw IfcException("Unimplemented"); }
		operator std::string() const { throw IfcException("Unimplemented"); }

		bool isNull() const { throw IfcException("Unimplemented"); };
		unsigned int size() const { throw IfcException("Unimplemented"); };
		ArgumentType type() const { throw IfcException("Unimplemented"); };
		Argument* operator [] (unsigned int i) const { throw IfcException("Unimplemented"); };
		std::string toString(bool upper=false) const { throw IfcException("Unimplemented"); };

		class Derived {};

		void set(const IfcEntityList::ptr& v) { throw IfcException("Unimplemented"); };
		void set(const IfcEntityListList::ptr& v) { throw IfcException("Unimplemented"); };
		void set(IfcBaseClass*const& v) { throw IfcException("Unimplemented"); };
		void set(double& v) { throw IfcException("Unimplemented"); };
		void set(bool& v) { throw IfcException("Unimplemented"); };
		void set(int& v) { throw IfcException("Unimplemented"); };
		void set(std::string& v) { throw IfcException("Unimplemented"); };
		void set(std::vector<int>& v) { throw IfcException("Unimplemented"); };
		void set(std::vector<double>& v) { throw IfcException("Unimplemented"); };
		void set(std::vector<std::string>& v) { throw IfcException("Unimplemented"); };
		void set(std::vector<std::vector<double>>& v) { throw IfcException("Unimplemented"); };
		void set(std::vector<std::vector<int>>& v) { throw IfcException("Unimplemented"); };
		template <typename T> typename boost::disable_if<boost::is_base_of<IfcBaseClass, typename boost::remove_pointer<T>::type>, void>::type set(const T& t) 
		{
			throw IfcException("Invalid cast");
		}

		void toJson(json_spirit::mArray& iArray) const { throw IfcException("Unimplemented"); };

		class EnumerationReference {
		public:
			int data;
			const char* enumeration_value;
			EnumerationReference(int data, const char* enumeration_value)
				: data(data), enumeration_value(enumeration_value) {}
		};	

		void set(EnumerationReference& v) { throw IfcException("Unimplemented"); };
	
};