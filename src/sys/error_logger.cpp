#include "sys/error_logger.h"
#include "script.h"
#include <iostream>
#include <fstream>

namespace sys {

	enum { defaultOutput = eoStdout };

	EmptyErrorLogger EmptyErrorLogger::instance;
	ErrorLogger ErrorLogger::instance;

	ErrorLogger::ErrorLogger() {
		LoadDefault();
	}

	namespace {

		const char *errTypeName[etCount]={ "", "Sys", "Aud", "Phs", "Gfx", "Gam" };
		const char *errLevelName[elCount]={ "Err", "Wng", "Msg", "Dbg" };

	};

	void ErrorLogger::Log(ErrorType t,ErrorLevel l,const char *file,int line,const string &str) {
		MutexLocker locker(mutex);

		string tstr;
		if(l!=elNormal) {
			if(detail>0) tstr+=Stringize('[',errTypeName[t],errLevelName[l],"] ");
			if(detail>1) tstr+=Stringize(file,':',line,": ");
		}
		tstr+=str;

		int outs=outputs[t][l];

		if(outs&eoStdout) std::cout << tstr;
		if(outs&eoStderr) std::cerr << tstr;
		if(outs&eoFile) {
			std::ofstream out;
			out.open(fileName.c_str(),std::ios::app);
			out << tstr;
		}
	}

	void ErrorLogger::LoadDefault() {
		MutexLocker locker(mutex);

		for(int t=0;t<etCount;t++)
			for(int l=0;l<elCount;l++)
				outputs[t][l]=defaultOutput;
		fileName="log.txt";
	}

	void  ErrorLogger::LoadConfig(const char *file) {
		MutexLocker locker(mutex);

		int filters[etCount],outs[etCount];

		script::Script(file).Parse() 
			("fileName", fileName)
			("detail", detail)
			["filters"]
				("base",	filters[0])
				("sys",		filters[1])
				("audio",	filters[2])
				("physics",	filters[3])
				("gfx",		filters[4])
				("game",	filters[5])
				[-1]
			["outputs"]
				("base",	outs[0])
				("sys",		outs[1])
				("audio",	outs[2])
				("physics",	outs[3])
				("gfx",		outs[4])
				("game",	outs[5]);
		
		for(int n=0;n<etCount;n++)
			filters[n]=clamp(filters[n],0,4);

		for(int t=0;t<etCount;t++)
			for(int l=0;l<elCount;l++)
				outputs[t][l]=filters[t]>l?outs[t]:0;
	}
}

