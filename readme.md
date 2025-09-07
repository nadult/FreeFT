# FreeFT [![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## Introduction
FreeFT is an open-source, real-time, isometric action game engine
inspired by Fallout Tactics, a game from 2001 created by an Australian company,
Micro Forte.
 
Old compiled binaries for Windows can be downloaded from SourceForge
[https://sourceforge.net/projects/freeft](https://sourceforge.net/projects/freeft/files)

Note: FreeFT is far from being finished. Many features are missing in FreeFT, for example:
lighting system, RPG elements, dialogues or scripting system.

## Building

FreeFT is based on libfwk framework (which is included as a submodule) and requires the same
tools / dependencies as libfwk. Please take a look at libfwk README first. Besides that,
FreeFT has some additional dependencies: mpg123, libzip, xxd, xz_utils. Those can be installed
by running `libfwk/tools/install_deps.py` in FreeFT directory. Once all dependencies are installed,
FreeFT can be built with Visual Studio 2022. Solution & project files is in `windows/` subdirectory.

Note: libfwk is currently being cleaned-up and during this time linux builds might not work.

## Running
To run this program, resources from original Fallout Tactics are required.
You can buy it on GOG or Steam.

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
DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
