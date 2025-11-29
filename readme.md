# FreeFT [![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0) [![Build status](https://github.com/nadult/freeft/actions/workflows/test.yml/badge.svg?branch=main)](https://github.com/nadult/freeft/actions)

## Introduction
FreeFT is an open-source, real-time, isometric action game engine strongly inspired by Fallout
Tactics, a game from 2001 created by an Australian company, Micro Forte.
 
Old compiled binaries for Windows can be downloaded from SourceForge
[https://sourceforge.net/projects/freeft](https://sourceforge.net/projects/freeft/files)

**Important note: FreeFT is far from being finished and is not being actively developed. Many
features are missing, for example: lighting system, RPG elements, dialogues or scripting
system.**

## Building

FreeFT is based on CMake and [libfwk](https://github.com/nadult/libfwk) (which is included as a
submodule) and requires the same tools / dependencies as libfwk. Please take a look at libfwk README
first to learn how to build it. Besides libfwk, FreeFT has some additional dependencies: mpg123,
libzip. Those can be installed by using libfwk's
[configure.py](https://github.com/nadult/libfwk/blob/main/tools/configure.py) script for
downloading/building dependencies. 

There are [github actions](https://github.com/nadult/FreeFT/blob/main/.github/workflows/test.yml)
available which build FreeFT for Windows & linux. They are a good reference of what is required to
properly build this project.

## Running
To run this program, resources from original Fallout Tactics are required. You can buy it on GOG,
Steam or in Epic Store.

FT has to be installed and resources converted with a convert program. After instaling FT just run
convert.exe. It might ask you to provide a path to original Fallout Tactics. The conversion
shouldn't take more than a few minutes.  

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
* **libfwk and all of its dependencies**  
  Used under MIT license.  
	[https://github.com/nadult/libfwk](https://github.com/nadult/libfwk)

* **mpg123**  
  Used under LGPL 2.1 license.  
	[https://www.mpg123.de/](https://www.mpg123.de/)

* **libzip**  
  Used under BSD 3-clause license.  
	[http://www.nih.at/libzip/](http://www.nih.at/libzip/)

* **Fonts**  
  Liberation (used under SIL Open Font License)  
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

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
