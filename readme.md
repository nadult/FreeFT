# FreeFT




## Introduction
FreeFT is an open-source, real-time, isometric action game engine
inspired by Fallout Tactics, a game from 2001 created by an Australian company,
Micro Forte.
 
Compiled binaries can be downloaded from SourceForge
[https://sourceforge.net/projects/freeft](https://sourceforge.net/projects/freeft/files)


## Building
To compile a linux environment with Clang 8.0 is recommended (G++ 9.0 or newer can also be used).
Following libraries are required:

* libfwk (included as submodule):
	[https://github.com/nadult/libfwk](https://github.com/nadult/libfwk)  
    This library also depends on few other libs (SDL2, libogg, libvorbis, freetype2, libpng).

* zlib, OpenAL, mpg123, libzip:  
	These should be available in your distro's package repositories.

To build for windows, you have to cross-compile with MinGW.  
The easiest way is with MXE ([http://mxe.cc](http://mxe.cc)) with following flags:  

	export MXE_TARGETS=x86_64-w64-mingw32.static.posix
	export MXE_PLUGIN_DIRS=plugins/gcc9

## Running
To run this program, resources from original Fallout Tactics are required.
You can buy it on GOG or Steam.

FT has to be installed and resources converted with a convert program.
After instaling FT just run convert.exe. It might ask you to provide a
path to original Fallout Tactics. The conversion shouldn't take more than
a few minutes.  

To convert resources under linux, you can use this command:
./convert -p "/home/user\_name/.wine/drive\_c/tactics/" all

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
* **mpg123**  
	[http://www.mpg123.de/index.shtml](http://www.mpg123.de/index.shtml)

* **zlib**  
	[http://www.gzip.org/zlib/](http://www.gzip.org/zlib/)

* **libzip**  
	[http://www.nih.at/libzip/](http://www.nih.at/libzip/)

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

Third tech-demo:
![](https://cloud.githubusercontent.com/assets/3227675/6886151/e705b616-d634-11e4-8f22-ee2f7c1dca01.jpg "Tech-demo #3")

Fourth tech-demo:
![](https://cloud.githubusercontent.com/assets/3227675/6886145/be9290d2-d634-11e4-80de-95b558f82cf9.jpg "Tech-demo #4")

Early version of game editor:
![](https://cloud.githubusercontent.com/assets/3227675/6886143/a67135a8-d634-11e4-93ef-e98c754e5cad.jpg "Editor")


## Videos

[Multi-player gamplay with bots](https://vimeo.com/101652935)

[Single-player gameplay demo](https://vimeo.com/91863672)

[First tech-demo](https://vimeo.com/58703722)

[Editor in action](https://vimeo.com/88563626)

[Path-finding demo](https://vimeo.com/58703723)

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
