#ifndef SQUIRREL_HPP
#define SQUIRREL_HPP

#include <cstdio>
#include <cstdarg>
#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>

//include all the stdlib headers
#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>

//opens a vm and reqisters the stdlibs
HSQUIRRELVM sqstd_open( SQInteger );

namespace sq	{

struct invalid_type : std::exception	{
	invalid_type( std::string who, std::string req )	{
		std::stringstream ss;
		ss << who << " is not a " << req;
		msg = ss.str();
	}
	virtual const char* what() const throw() {
		return msg.c_str();
	}
	std::string msg;
};

struct invalid_key : std::exception	{
	invalid_key(std::string _table, std::string _key)	{
		std::stringstream ss;
		ss << _table << "[" << _key << "] does not exist";
		msg = ss.str();
	}
	virtual const char* what() const throw()	{
		return msg.c_str();
	}
	std::string msg;
};

struct bad_script : std::exception	{
	bad_script( std::string file )	: msg(file)	{}
	virtual const char* what() const throw()	{
		return msg.c_str();
	}
	std::string msg;
};

struct bad_call : std::exception	{
	bad_call( std::string m )	{
		msg = "a bad call was made: " + m;
	}

	virtual const char* what() const throw()	{
		return msg.c_str();
	}
	std::string msg;
};

struct object	{
	object( HSQUIRRELVM v, std::string n, object *root ) : sqvm(v), parent(root)	{
		name = n;
		indexed = false;
		stack = false;
		init();
	}

	object( HSQUIRRELVM v, int idx, object *root, bool onStack=true ) : sqvm(v), parent(root)	{
		name = std::to_string(idx);
		index = idx;
		indexed = true;
		stack = onStack;
		init();
	}

	~object()	{ sq_release(sqvm, &obj); }

	object operator[] (const std::string field)	{
		return object(sqvm, field, this);
	}

	object operator[] (const int idx)	{
		return object(sqvm, idx, this);
	}

	void init()	{
		if( stack )	{
			getStackObj(index);
		} else {
			int parentspushed = sq_gettop(sqvm);
			getParent();
			parentspushed = sq_gettop(sqvm) - parentspushed;

			if(indexed)
				sq_pushinteger(sqvm, index);
			else
				sq_pushstring(sqvm, name.c_str(), -1);
			get(-2);
			getStackObj(-1);
			sq_pop(sqvm, parentspushed);
		}
	}

	void get(int idx)	{
		if(SQ_FAILED(sq_get(sqvm, idx)))	{
			if(parent == nullptr)
				throw invalid_key("root", name);
			else
				throw invalid_key(parent->name, name);
		}
	}

	void getStackObj(int idx)	{
		sq_resetobject(&obj);
		sq_getstackobj(sqvm, idx, &obj);
		sq_addref(sqvm, &obj);
	}

	void getParent()	{
		//if the parent is null we push the root table, else we push the parent
		if(parent == nullptr)
			sq_pushroottable(sqvm);
		else
			parent->push();
	}

	//push the object onto the stack
	void push()	{
		sq_pushobject(sqvm, obj);
	}

	int getType()	{
		push();
		int i = sq_gettype(sqvm, -1);
		sq_pop(sqvm,1);
		return i;
	}

	template<typename t>
	t to();

	template<typename t>
	void setfield(std::string field, t val);

	object *parent;
	HSQUIRRELVM sqvm;
	HSQOBJECT obj;

	std::string name;
	int index;
	bool indexed;
	bool stack;
};

template<>
inline std::string object::to()	{
	if(getType() != OT_STRING)
		throw invalid_type(name, "string");
	return sq_objtostring(&obj);
}

template<>
inline int object::to()	{
	if(getType() != OT_INTEGER)
		throw invalid_type(name, "integer");
	return sq_objtointeger(&obj);
}

template<>
inline float object::to()	{
	if(getType() != OT_FLOAT)
		throw invalid_type(name, "float");
	return sq_objtofloat(&obj);
}

template<>
inline void object::setfield( std::string field, std::string val )	{
	push();
	sq_pushstring(sqvm, field.c_str(), -1);
	sq_pushstring(sqvm, val.c_str(), -1);
	sq_newslot(sqvm, -3, false);
	sq_pop(sqvm, 1);
}

struct vm	{
	vm()	{}
	vm(HSQUIRRELVM v) : sqvm(v) {}

	object operator[]( const std::string name )	{
		return object(sqvm, name, nullptr);
	}

	object operator[]( const int _idx )	{
		return object(sqvm, _idx, nullptr);
	}

	void open( int stacksize )	{ sqvm = sqstd_open( stacksize ); }

	void close()	{ sq_close(sqvm); }

	void newTable( std::string name );
	void runScript( std::string filename );
	void registerFunction( const char* table, const char* fname, SQFUNCTION func );

	int gettop()	{ return sq_gettop(sqvm); }

	HSQUIRRELVM sqvm;
};

} //namespace sq

#endif // SQUIRREL_HPP
