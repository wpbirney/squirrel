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

#ifdef SQUNICODE
#define scvprintf vfwprintf
#else
#define scvprintf vfprintf
#endif

//default print function, writes to stdout
inline void _printfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	printf("[squirrel]: ");
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
	std::cout << std::endl;
}

//default error function, writes to stderr
inline void _errorfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	printf("[squirrel]: ");
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

//register all the libs
inline void sqstd_register_libs(HSQUIRRELVM v)	{
	sq_pushroottable(v);
	sqstd_register_bloblib(v);
	sqstd_register_iolib(v);
	sqstd_register_systemlib(v);
	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);
	sq_pop(v, 1);
}

//opens a vm and reqisters the stdlibs
inline HSQUIRRELVM sqstd_open(SQInteger stacksize)	{
	HSQUIRRELVM v;
	v = sq_open(1024);
	sqstd_register_libs(v);
	sqstd_seterrorhandlers(v); //sets default error handlers
	sq_setprintfunc(v, _printfunc, _errorfunc);
	sq_enabledebuginfo(v, SQTrue);
	return v;
}

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

	void init( std::string n ) {
		name = n;
		getParent();
		sq_pushstring(sqvm, n.c_str(), -1);
		get(-2);
		getStackObj(-1);
		sq_pop(sqvm, 2);
	}

	void init( int idx ) {
		name = std::to_string(idx);
		if(parent == nullptr)	{
			getStackObj(idx);
			return;
		}
		getParent();
		sq_pushinteger(sqvm, idx);
		get(-2);
		getStackObj(-1);
		sq_pop(sqvm, 2);
	}

	void push()	{
		sq_pushobject( sqvm, obj );
	}

	//calls sq_get, throws an exception if not succesful
	void get(int idx)	{
		if(SQ_FAILED(sq_get(sqvm, idx)))	{
			sq_pop(sqvm, 1);
			if( parent == nullptr )
				throw invalid_key("root", name);
			else
				throw invalid_key(parent->name, name);
		}
	}

	int getType()	{
		push();
		int r = sq_gettype(sqvm, -1);
		sq_poptop(sqvm);
		return r;
	}

	void getParent()	{
		if(parent == nullptr)
			sq_pushroottable(sqvm);
		else
			parent->push();
	}

	void getStackObj( int idx )	{
		sq_resetobject( &obj );
		sq_getstackobj(sqvm, -1, &obj);
		sq_addref( sqvm, &obj );
	}

	std::string asString()	{
		if(getType() == OT_STRING)
			return sq_objtostring( &obj );
		else
			throw invalid_type(name, "string");
	}

	int asInt()	{
		if(getType() == OT_INTEGER)
			return sq_objtointeger( &obj );
		else
			throw invalid_type(name, "integer");
	}

	float asFloat()	{
		if(getType() == OT_FLOAT)
			return sq_objtofloat( &obj );
		else
			throw invalid_type(name ,"float");
	}

	SQUserPointer asPointer()	{
		if(getType() == OT_USERPOINTER)
			return sq_objtouserpointer( &obj );
		else
			throw invalid_type(name, "userpointer");
	}

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

	SQRESULT runScript( std::string filename )	{
		sq_pushroottable(sqvm);
		SQRESULT r = sqstd_dofile(sqvm, _SC(filename.c_str()), false, true);
		sq_poptop(sqvm);
		return r;
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
