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

void vm::newTable( std::string name )	{
	sq_pushroottable(sqvm);
	sq_pushstring(sqvm, name.c_str(), -1);
	sq_newtable(sqvm);
	sq_newslot(sqvm, -3, false);
	sq_settop(sqvm, 0);
}

void vm::runScript( std::string filename )	{
	sq_pushroottable(sqvm);
	SQRESULT r = sqstd_dofile(sqvm, _SC(filename.c_str()), false, true);
	sq_poptop(sqvm);

	if(SQ_FAILED(r))
		throw bad_script(filename);
}

void vm::registerFunction( const char* table, const char* fname, SQFUNCTION func )	{
	sq_pushroottable(sqvm);
	if( table != nullptr )	{
		sq_pushstring(sqvm,table,-1);
		sq_get(sqvm, -2);
	}
	sq_registerfunction(sqvm, fname, func);
	sq_poptop(sqvm);
}


} //namespace sq

