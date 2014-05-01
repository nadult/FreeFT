/* Copyright (C) 2013-2014 Krzysztof Jakubowski <nadult@fastmail.fm>

   This file is part of FreeFT.
 */

#include <memory.h>
#include <cstdio>
#include "sys/platform.h"
#include "sys/config.h"
#include "sys/xml.h"
#include "net/host.h"
#include <list>
#include <algorithm>

using namespace net;

class LobbyServer: public net::LocalHost {
public:
	LobbyServer(int port) :LocalHost(Address(port)) { }

private:
};

int safe_main(int argc, char **argv)
{


	return 0;
}

int main(int argc, char **argv) {
	try {
		return safe_main(argc, argv);
	}
	catch(const Exception &ex) {
		printf("%s\n\nBacktrace:\n%s\n", ex.what(), cppFilterBacktrace(ex.backtrace()).c_str());
		return 1;
	}
}

