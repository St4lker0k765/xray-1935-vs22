#ifndef xrDebugH
#define xrDebugH
#pragma once

typedef	void		crashhandler		(void);

class XRCORE_API	xrDebug
{
private:
	crashhandler*	handler	;

public:
	void			_initialize		();
	void			_destroy		();
	
public:
	crashhandler*	get_crashhandler	()							{ return handler;	};
	void			set_crashhandler	(crashhandler* _handler)	{ handler=_handler;	};

	LPCSTR			error2string		(long  code	);
	void			fail				(const char *e1, const char *file, int line);
	void			fail				(const char *e1, const char *e2, const char *file, int line);
	void			fail				(const char *e1, std::string &e2, const char *file, int line);
	void			fail				(const char *e1, const char *e2, const char *e3, const char *file, int line);
	void			error				(long  code, const char* e1, const char *file, int line);
	void _cdecl		fatal				(const char* F,...);
	void			backend				(const char* reason, const char* file, int line);
	void			do_exit				(const std::string &message);
};

// warning
// this function can be used for debug purposes only
IC	std::string __cdecl	make_string		(LPCSTR format,...)
{
	va_list		args;
	va_start	(args,format);

	char		temp[4096];
	vsprintf	(temp,format,args);

	va_end(args);

	return temp;
}

extern XRCORE_API	xrDebug		Debug;

#include "xrDebug_macros.h"

#endif
