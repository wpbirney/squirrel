#include <squirrel.hpp>

#ifdef SQUNICODE
#define scvprintf vfwprintf
#else
#define scvprintf vfprintf
#endif

//default print function, writes to stdout
void _printfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	printf("[squirrel]: ");
	va_list vl;
	va_start(vl, s);
	scvprintf(stdout, s, vl);
	va_end(vl);
	std::cout << std::endl;
}

//default error function, writes to stderr
void _errorfunc(HSQUIRRELVM v,const SQChar *s,...)
{
	printf("[squirrel]: ");
	va_list vl;
	va_start(vl, s);
	scvprintf(stderr, s, vl);
	va_end(vl);
}

//register all the libs
void sqstd_register_libs(HSQUIRRELVM v)	{
	sq_pushroottable(v);
	sqstd_register_bloblib(v);
	sqstd_register_iolib(v);
	sqstd_register_systemlib(v);
	sqstd_register_mathlib(v);
	sqstd_register_stringlib(v);
	sq_pop(v, 1);
}

//opens a vm and reqisters the stdlibs
HSQUIRRELVM sqstd_open(SQInteger stacksize)     {
	HSQUIRRELVM v;
	v = sq_open(1024);
	sqstd_register_libs(v);
	sqstd_seterrorhandlers(v); //sets default error handlers
	sq_setprintfunc(v, _printfunc, _errorfunc);
	sq_enabledebuginfo(v, SQTrue);
	return v;
}

namespace sq	{

void object::init( std::string n ) {
	name = n;
	getParent();
	sq_pushstring(sqvm, n.c_str(), -1);
	get(-2);
	getStackObj(-1);
	sq_pop(sqvm, 2);
}

void object::init( int idx ) {
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

void object::get(int idx)	{
	if(SQ_FAILED(sq_get(sqvm, idx)))	{
		sq_pop(sqvm, 1);
		if( parent == nullptr )
			throw invalid_key("root", name);
		else
			throw invalid_key(parent->name, name);
	}
}

int object::getType()	{
	push();
	int r = sq_gettype(sqvm, -1);
	sq_poptop(sqvm);
	return r;
}

void object::getParent()	{
	if(parent == nullptr)
		sq_pushroottable(sqvm);
	else
		parent->push();
}

void object::getStackObj( int idx )	{
	sq_resetobject( &obj );
	sq_getstackobj(sqvm, -1, &obj);
	sq_addref( sqvm, &obj );
}

std::string object::asString()	{
	if(getType() == OT_STRING)
		return sq_objtostring( &obj );
	else
		throw invalid_type(name, "string");
}

int object::asInt()	{
	if(getType() == OT_INTEGER)
		return sq_objtointeger( &obj );
	else
		throw invalid_type(name, "integer");
}

float object::asFloat()	{
	if(getType() == OT_FLOAT)
		return sq_objtofloat( &obj );
	else
		throw invalid_type(name ,"float");
}

SQUserPointer object::asPointer()	{
	if(getType() == OT_USERPOINTER)
		return sq_objtouserpointer( &obj );
	else
		throw invalid_type(name, "userpointer");
}

}

