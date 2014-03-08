1. Introduction
FreeFT is an open-source, real-time, isometric action game engine
inspired by Fallout Tactics, a game from 2001 done by Australian company,
Micro Forté. Initially the goal was to make an RPG engine, which every
modder could use to create his own game in post-apocalyptic world. However,
because of lack of interest, I’ve decided that it would be better first to
create a simple, but playable game and only after it gains some interest,
more advanced features and RPG elements will be added.

More information about the project can be found here:
http://freeft.net

2. Building
For compilation G++ in version 4.7 is required (lots of C++11 features are
used), and following libraries:
- GLFW, libpng, zlib
	these should be available in your distro's package repositories
- baselib
	https://github.com/nadult/baselib

Compiled binaries are available at SourceForge:
https://sourceforge.net/projects/freeft/files/?source=navbar

3. Running
To run this program, resources from original Fallout Tactics are required.
f you didn't grab a free copy from gog.com, then you can buy it on Amazon,
or on Steam (when it will be available). FT has to be installed and resources
converted with a convert program. The process is very simple, just run:
convert.exe -p path_to_ft all
Where path_to_ft is a path to the original game, for example:
convert.exe -p "c:/program files/fallout tactics/" all

4. Used libraries / resources:
- Liberation fonts (licensed under SIL Open Font License)
- fonts converted to bitmaps using BMFont
- GLFW
	http://glfw.sourceforge.net/
- libpng
	http://libpng.org
- zlib
	http://www.gzip.org/zlib/
- libzip
	http://www.nih.at/libzip/
- rapidxml
	included in libs/rapidxml*/, licensed under Boost Software License
	http://rapidxml.sourceforge.net/
- lz4
	included in libs/lz4/, licensed under BSD license
	http://fastcompression.blogspot.com/p/lz4.html
