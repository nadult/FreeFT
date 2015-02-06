# FreeFT




## Introduction
FreeFT is an open-source, real-time, isometric action game engine
inspired by Fallout Tactics, a game from 2001 done by Australian company,
Micro Forte.
 
More information about this project, screenshots and compiled binaries can be found here:
[http://freeft.net](http://freeft.net)

Compiled binaries can also be downloaded from SourceForge
[http://sourceforge.net/projects/freeft](https://sourceforge.net/projects/freeft/files)


## Building
For compilation, G++ in version 4.8 is required (many C++11 features are
used), and following libraries:

* GLFW, libpng, zlib, OpenAL, mpg123, libzip:  
	these should be available in your distro's package repositories

* baselib:  
	[http://github.com/nadult/baselib](http://github.com/nadult/baselib)

## Running
To run this program, resources from original Fallout Tactics are required.
If you didn't grab a free copy from gog.com, then you can buy it on Amazon,
or on Steam (when it will be available).

FT has to be installed and resources converted with a convert program.
After instaling FT just run convert.exe. It might ask you to provide a
path to original Fallout Tactics. The conversion shouldn't take more than
a few minutes.  

To convert resources under linux, you can use this command:
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


## License

The code if freely available but if you want to use it, commercially or not, please
contact the author (e-mail: nadult (at) fastmail.fm).


## Screenshots

First tech-demo:
![](http://freeft.pl/wp-content/uploads/2014/04/techdemo3a.jpg "Tech-demo #3")

Fourth tech-demo:
![](http://freeft.pl/wp-content/uploads/2014/07/techdemo4a.jpg "Tech-demo #4")

Early version of game editor:
![](http://freeft.pl/wp-content/uploads/2014/03/techdemo1c.jpg "Editor")


## Videos

[Multi-player gamplay with bots](http://vimeo.com/101652935)

[Single-player gameplay demo](http://vimeo.com/91863672)

[First tech-demo](http://vimeo.com/58703722)

[Editor in action](http://vimeo.com/88563626)

[Path-finding demo](http://vimeo.com/58703723)

## Disclaimer

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
