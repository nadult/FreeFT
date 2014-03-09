# FreeFT




## Introduction
FreeFT is an open-source, real-time, isometric action game engine
inspired by Fallout Tactics, a game from 2001 done by Australian company,
Micro Forte. Initially the goal was to make an RPG engine, which every
modder could use to create his own game in post-apocalyptic world. However,
because of lack of interest, I've decided that it would be better first to
create a simple, but playable game and only after it gains some interest,
more advanced features and RPG elements will be added.

More information about this project, screensots and compiled binaries can be found here:
[http://freeft.net](http://freeft.net)


## Building
For compilation G++ in version 4.7 is required (lots of C++11 features are
used), and following libraries:

* GLFW, libpng, zlib:  
	these should be available in your distro's package repositories

* baselib:  
	[http://github.com/nadult/baselib](http://github.com/nadult/baselib)

## Running
To run this program, resources from original Fallout Tactics are required.
If you didn't grab a free copy from gog.com, then you can buy it on Amazon,
or on Steam (when it will be available).

FT has to be installed and resources converted with a convert program.
The process is very simple, just run:  
convert.exe -p path\_to\_ft all  
Where path\_to\_ft is simply a path to the original game, for example:  
convert.exe -p "c:/program files/fallout tactics/" all

## Basic controls in the game

* LMB on tile: move (shift: walk)
* LMB on entity: interact
* RMB: attack (shift: secondary attack)
* Middle mouse dragging: change view
* keypad + / -: change stance
* t: teleport to cursor
* up / down: select item
* up / down + CTRL: select item in container
* left / right: move item to / from container

## Used libraries and resources:
* Liberation fonts (licensed under SIL Open Font License)

* fonts converted to bitmaps using BMFont

* GLFW  
	[http://glfw.sourceforge.net/](http://glfw.sourceforge.net/)

* libpng  
	[http://libpng.org](http://libpng.org)

* zlib  
	[http://www.gzip.org/zlib/](http://www.gzip.org/zlib/)

* libzip  
	[http://www.nih.at/libzip/](http://www.nih.at/libzip/)

* rapidxml  
	included in libs/rapidxml*/, licensed under Boost Software License  
	[http://www.nih.at/libzip/](http://rapidxml.sourceforge.net/)

* lz4  
	included in libs/lz4/, licensed under BSD license  
	[http://fastcompression.blogspot.com/p/lz4.html](http://fastcompression.blogspot.com/p/lz4.html)

## Screenshots

Early version of game editor:
![](http://freeft.pl/wp-content/uploads/2014/03/techdemo1c.jpg "Editor")

First tech-demo:
![](http://freeft.pl/wp-content/uploads/2014/03/techdemo1b.jpg "Tech-demo #1")

Second tech-demo:
![](http://freeft.pl/wp-content/uploads/2014/03/techdemo2b.jpg "Tech-demo #2")
