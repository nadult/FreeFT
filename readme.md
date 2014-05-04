# FreeFT




## Introduction
FreeFT is an open-source, real-time, isometric action game engine
inspired by Fallout Tactics, a game from 2001 done by Australian company,
Micro Forte.
 
More information about this project, screensots and compiled binaries can be found here:
[http://freeft.net](http://freeft.net)


## Building
For compilation, G++ in version 4.7 is required (many C++11 features are
used), and following libraries:

* GLFW, libpng, zlib, OpenAL, libmad:
	these should be available in your distro's package repositories

* baselib:  
	[http://github.com/nadult/baselib](http://github.com/nadult/baselib)

## Running
To run this program, resources from original Fallout Tactics are required.
If you didn't grab a free copy from gog.com, then you can buy it on Amazon,
or on Steam (when it will be available).

FT has to be installed and resources converted with a convert program.
After instaling FT just run convert.exe. It might ask you to provide a
path to FreeFT. The conversion shouldn't take more than few minutes.  

To convert resources under linux, type:
./convert -p path\_to\_ft all  
Where path\_to\_ft is simply a path to the original game, for example:  
./convert -p "/home/user_name/.wine/drive_c/tactics/" all

## Basic controls in the game

* LMB on tile: move (shift: walk)
* LMB on entity: interact
* RMB: attack (shift: secondary attack)
* Middle mouse dragging: change view
* keypad + / -: change stance
* T key: teleport to cursor

Inventory (bottom left corner):

* up / down: select item
* up / down + CTRL: select item in container
* left / right: move item to / from container
* E key: equip / unequip selected item

## Used libraries and resources
* **GLFW**  
	[http://glfw.sourceforge.net/](http://glfw.sourceforge.net/)

* **rapidxml**  
	included in libs/rapidxml*/, licensed under Boost Software License  
	[http://www.nih.at/libzip/](http://rapidxml.sourceforge.net/)

* **libpng**  
	[http://libpng.org](http://libpng.org)

* **mpg123**  
	[http://www.mpg123.de/index.shtml](http://www.mpg123.de/index.shtml)

* **zlib**  
	[http://www.gzip.org/zlib/](http://www.gzip.org/zlib/)

* **libzip**  
	[http://www.nih.at/libzip/](http://www.nih.at/libzip/)

* **lz4**  
	included in libs/lz4/, licensed under BSD license  
	[http://fastcompression.blogspot.com/p/lz4.html](http://fastcompression.blogspot.com/p/lz4.html)

* **OpenAL**

* **Fonts**  
  Liberation (licensed under SIL Open Font License)  
  Transformers (freeware)  

  BMFont was used to convert fonts to bitmaps  
  [http://www.angelcode.com/products/bmfont/](http://www.angelcode.com/products/bmfont/)

## Screenshots

Early version of game editor:
![](http://freeft.pl/wp-content/uploads/2014/03/techdemo1c.jpg "Editor")

First tech-demo:
![](http://freeft.pl/wp-content/uploads/2014/03/techdemo1b.jpg "Tech-demo #1")

Third tech-demo:
![](http://freeft.pl/wp-content/uploads/2014/04/techdemo3a.jpg "Tech-demo #3")
