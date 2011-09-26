#ifndef NEWBORN_ERROR_LOGGER_H
#define NEWBORN_ERROR_LOGGER_H

#include "base.h"
#include <baselib_threads.h>

namespace sys {

	enum ErrorLevel {
		elError,
		elWarning,
		elNormal,
		elDebug,

		elCount
	};

	enum ErrorType {
		etBase,
		etSys,
		etAudio,
		etPhys,
		etGfx,
		etGame,

		etCount
	};

	enum ErrorOutput {
		eoStdout	= 1,
		eoStderr	= 2,
		eoFile		= 4,
	};

	//! Efektywnie wysyla komunikaty w kosmos :D
	class EmptyErrorLogger {
	public:
		template <class ... Args>
		void Log(ErrorType,ErrorLevel,const char*,int,const Args&...) { }

		static EmptyErrorLogger instance;

	private:
		EmptyErrorLogger() { }
		EmptyErrorLogger(const EmptyErrorLogger&) { }
		void operator=(const EmptyErrorLogger&) { }
	};

	class ErrorLogger {
	public:
		ErrorLogger();

		template <class ... Args> 
		void Log(ErrorType t,ErrorLevel l,const char *file,int line,const Args& ... args) {
			string text=Stringize(args...,'\n');
			Log(t,l,file,line,text);
		}
		void Log(ErrorType,ErrorLevel,const char*,int,const string&);
		void LoadConfig(const char *file="data/logger.cfg");
		void LoadDefault();

		static ErrorLogger instance;

	private:
		ErrorLogger(const ErrorLogger&) { }
		void operator=(const ErrorLogger&) { }

		int outputs[etCount][elCount];
		int detail;
		string fileName;
		Mutex mutex;
	};

	template <bool enabled> struct TLogger { typedef ErrorLogger Result; };
	template<> struct TLogger<false> { typedef EmptyErrorLogger  Result; };

}

#define DECLARE_LOGGER(name,type,level,enabled) \
		template <class ... Args> void name(const char *file,int line,const Args& ... args) { \
			typedef typename sys::TLogger<enabled>::Result Logger; \
			Logger::instance.Log(sys::type,sys::level,file,line,args...); }

#ifdef NDEBUG
	#define DBG_ENABLED 0
#else
	#define DBG_ENABLED 1
#endif

#define DECLARE_LOGGERS(type,en1,en2,en3,en4) \
	DECLARE_LOGGER(type##Error_		,et##type	,elError	,(en1)) \
	DECLARE_LOGGER(type##Warning_	,et##type	,elWarning	,(en2)) \
	DECLARE_LOGGER(type##Msg_		,et##type	,elNormal	,(en3)) \
	DECLARE_LOGGER(type##Debug_		,et##type	,elDebug	,(en4))

DECLARE_LOGGERS(Base, 1, 1, 1, DBG_ENABLED)
#define BaseError(...) 		BaseError_	(__FILE__,__LINE__,__VA_ARGS__)
#define BaseWarning(...) 	BaseWarning_(__FILE__,__LINE__,__VA_ARGS__)
#define BaseMsg(...) 		BaseMsg_	(__FILE__,__LINE__,__VA_ARGS__)
#define BaseDebug(...)		BaseDebug_	(__FILE__,__LINE__,__VA_ARGS__)

DECLARE_LOGGERS(Sys, 1, 1, 1, DBG_ENABLED)
#define SysError(...) 		SysError_	(__FILE__,__LINE__,__VA_ARGS__)
#define SysWarning(...) 	SysWarning_ (__FILE__,__LINE__,__VA_ARGS__)
#define SysMsg(...) 		SysMsg_		(__FILE__,__LINE__,__VA_ARGS__)
#define SysDebug(...)		SysDebug_	(__FILE__,__LINE__,__VA_ARGS__)

DECLARE_LOGGERS(Audio, 1, 1, 1, DBG_ENABLED)
#define AudioError(...) 	AudioError_	(__FILE__,__LINE__,__VA_ARGS__)
#define AudioWarning(...) 	AudioWarning_(__FILE__,__LINE__,__VA_ARGS__)
#define AudioMsg(...) 		AudioMsg_	(__FILE__,__LINE__,__VA_ARGS__)
#define AudioDebug(...)		AudioDebug_	(__FILE__,__LINE__,__VA_ARGS__)

DECLARE_LOGGERS(Gfx, 1, 1, 1, DBG_ENABLED)
#define GfxError(...) 		GfxError_	(__FILE__,__LINE__,__VA_ARGS__)
#define GfxWarning(...) 	GfxWarning_	(__FILE__,__LINE__,__VA_ARGS__)
#define GfxMsg(...) 		GfxMsg_		(__FILE__,__LINE__,__VA_ARGS__)
#define GfxDebug(...)		GfxDebug_	(__FILE__,__LINE__,__VA_ARGS__)

DECLARE_LOGGERS(Phys, 1, 1, 1, DBG_ENABLED)
#define PhysError(...) 		PhysError_	(__FILE__,__LINE__,__VA_ARGS__)
#define PhysWarning(...)	PhysWarning_(__FILE__,__LINE__,__VA_ARGS__)
#define PhysMsg(...) 		PhysMsg_	(__FILE__,__LINE__,__VA_ARGS__)
#define PhysDebug(...)		PhysDebug_	(__FILE__,__LINE__,__VA_ARGS__)

DECLARE_LOGGERS(Game, 1, 1, 1, DBG_ENABLED)
#define GameError(...) 		GameError_	(__FILE__,__LINE__,__VA_ARGS__)
#define GameWarning(...) 	GameWarning_(__FILE__,__LINE__,__VA_ARGS__)
#define GameMsg(...) 		GameMsg_	(__FILE__,__LINE__,__VA_ARGS__)
#define GameDebug(...)		GameDebug_	(__FILE__,__LINE__,__VA_ARGS__)

#undef DECLARE_LOGGER
#undef DECLARE_LOGGERS

#endif

