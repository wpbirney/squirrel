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

struct object	{
	object( HSQUIRRELVM v, std::string n, object *root ) : sqvm(v), parent(root)	{
		init(n);
	}

	object( HSQUIRRELVM v, int idx, object *root ) : sqvm(v), parent(root)	{
		init(idx);
	}

	~object()	{
		sq_release(sqvm, &obj);
	}

	object operator[] (const std::string field)	{
		return object(sqvm, field, this);
	}

	object operator[] (const int idx)	{
		return object(sqvm, idx, this);
	}

	//init functions, need to be templated
	void init( std::string );
	void init( int );

	//push the object onto the stack
	void push()	{
		sq_pushobject( sqvm, obj );
	}

	//calls sq_get, throws an exception if not succesful
	void get( int );

	//returns the OT_* type of the current object
	int getType();

	//pushes the parent onto the stack, if parent is null, then pushes the root-table
	void getParent();

	//sets the current object to the one at stack posistion specified
	void getStackObj( int );

	//as* functions
	std::string asString();
	int asInt();
	float asFloat();
	SQUserPointer asPointer();

	HSQOBJECT obj;
	object *parent;
	HSQUIRRELVM sqvm;

	std::string name;
};

struct vm	{
	vm()	{}
	vm(HSQUIRRELVM v) : sqvm(v) {}

	object operator[]( const std::string name )	{
		return object(sqvm, name, nullptr);
	}

	object operator[]( const int _idx )	{
		return object(sqvm, _idx, nullptr);
	}

	void open( int stacksize )	{
		sqvm = sqstd_open( stacksize );
	}

	void close()	{
		sq_close(sqvm);
	}

	void newTable( std::string name )	{
		sq_pushroottable(sqvm);
		sq_pushstring(sqvm, name.c_str(), -1);
		sq_newtable(sqvm);
		sq_newslot(sqvm, -3, false);
		sq_settop(sqvm, 0);
	}

	void runScript( std::string filename )	{
		sq_pushroottable(sqvm);
		SQRESULT r = sqstd_dofile(sqvm, _SC(filename.c_str()), false, true);
		sq_poptop(sqvm);

		if(SQ_FAILED(r))
			throw bad_script(filename);
	}

	void registerFunction( const char* table, const char* fname, SQFUNCTION func )	{
		sq_pushroottable(sqvm);
		if( table != nullptr )	{
			sq_pushstring(sqvm,table,-1);
			sq_get(sqvm, -2);
		}
		sq_registerfunction(sqvm, fname, func);
		sq_poptop(sqvm);
	}

	int gettop()	{ return sq_gettop(sqvm); }

	HSQUIRRELVM sqvm;
};

} //namespace sq

#endif // SQUIRREL_HPP
