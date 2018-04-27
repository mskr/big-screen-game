# big-screen-game
Approach of a game for a 3x4 m touchscreen in a framework using the Simple Graphics Cluster Toolkit

The framework is designed to run on a special GPU cluster at Ulm University. The big screen is installed in a wall. The image is projected by 12 beamers that are installed in a room behind the screen together with the computing cluster. The framework uses [SGCT](https://github.com/opensgct/sgct).

The game is played in bird's perspective and is about building rooms by dragging the finger over the screen. Rooms are devided into parts on a grid. There is an outer influence that can destroy parts of rooms and that is controlled by a cellular automaton. The player's goal is to repair rooms.

Preview GIF of outer influence destroying a room prototype:
![](https://github.com/mskr/big-screen-game/raw/feature/interactive-grid/preview.gif)

Finished version is in [develop branch](https://github.com/mskr/big-screen-game/tree/develop)
