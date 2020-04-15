# Conway's Game of Life for Nintendo 64


The compiled rom file can be found [here](https://github.com/jsdf/n64-gameoflife/raw/master/gameoflife.n64.zip). It can be run with an emulator or an N64 flashcart. Try pressing the A and B buttons.

![recording of the demo](https://media.giphy.com/media/dxyCdZR5iKyfod4ihB/giphy.gif)

## How to build (macOS or linux):

- Install [wine](https://www.winehq.org/)
- Install the n64 sdk. this repo assumes it's in the default location of `C:\ultra` (in the wine filesystem). If you've installed it somewhere else, you'll need to update this path in `compile.bat`
- Edit `./build.sh` so that WINE_PATH points to your wine binary
- Run `./build.sh`. This should build gameoflife.n64
- Run gameoflife.n64 with an emulator or an N64 flashcart
