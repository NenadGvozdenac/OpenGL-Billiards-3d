# 3D Billiards Game (9-Ball Pool)

This is a 3D implementation of the 9-Ball pool game, developed in C++ using **GLEW**, **GLFW**, **GLM**, and **FreeType** libraries. The game offers a realistic billiards experience with physics-based interactions, camera controls, and player statistics.

## Features

- **3D Billiard Table**: Includes a table with edges and collision detection for balls.
- **Cue Ball**: The cue ball is positioned on the table with a cue stick attached to it.
- **Cue Stick Control**: The cue stick is controlled by the mouse, and pressing the **Spacebar** hits the cue ball. The cue stick can change speed (1-5) for more precise control.
- **9 Balls**: Nine balls are placed on the table at the other end, following the rules of 9-Ball pool.
- **Multiple Cameras**: Five cameras provide different perspectives of the game (four side views and one top-down bird's-eye view).
- **Camera Switching**: Cameras can be switched with **CTRL + 1, 2, 3, 4, 5**.
- **Zoom Levels**: Three zoom levels are available, switched by **ALT + 1, 2, 3**.
- **Lighting**: Phong lighting is applied to the entire table for realistic lighting effects.
- **Cue Stick Separation**: The cue stick detaches from the cue ball when the player hits the ball, based on the cue stick speed.
- **FPS Limiting**: The game is limited to 120 FPS for optimal physics simulation.
- **Two-Player Mode**: The game supports two players, with player statistics displayed during the game.
- **Player Stats**: Current player, next ball to pocket, and current turn are displayed in the top left corner.
- **Player Info**: Name, surname, and index number are displayed in the top right corner.
- **Pause Menu**: The game can be paused by pressing **Esc**, which brings up a menu to continue or exit the game.
- **End Game**: The winner is displayed when the final ball is pocketed.

## Controls

- **Mouse**: Move the cue stick and aim the shot.
- **Left Click**: Hold to rotate the cue stick.
- **Spacebar**: Hit the cue ball with the cue stick.
- **CTRL + 1, 2, 3, 4, 5**: Switch between different camera perspectives.
- **ALT + 1, 2, 3**: Change the zoom level.
- **Esc**: Pause the game and open the pause menu.

## Libraries Used

- **GLEW**: OpenGL Extension Wrangler Library for handling OpenGL extensions.
- **GLFW**: OpenGL Framework for creating windows and handling user input.
- **GLM**: OpenGL Mathematics for handling 3D transformations and vector/matrix operations.
- **FreeType**: For text rendering to display player stats and information.

## Setup

### Prerequisites

Make sure you have the following installed on your system:

- **C++ Compiler** (e.g., GCC, Clang, MSVC)
- **GLEW**
- **GLFW**
- **GLM**
- **FreeType**

### Building the Game

1. Clone the repository:

```bash
git clone https://github.com/yourusername/3D-Billiards-Game.git
cd OpenGL-Billiards-3D/Project1
```

2. Open the .sln file.
3. Go into nugget packages, and restore packages that are missing!

## Game Logic Overview

- **Physics**: The game physics is implemented to simulate realistic ball movement and collisions. The cue ball and other balls interact with the table edges, bouncing off of them based on the physics engine.
- **Camera Control**: The game has five different camera angles. The user can switch between them to get different views of the table and enhance the playing experience.
- **Cue Stick**: The cue stick's speed can be adjusted using keyboard input, and it follows the player's mouse for aiming. The speed of the cue stick determines how hard the cue ball is hit.

## Player Statistics

- **Top Left Corner**: Displays which player is currently playing, the next ball to be pocketed.
- **Top Right Corner**: Shows the player's name, surname, and index.

## Pause Menu

Press **Esc** to pause the game. The pause menu will provide options to either resume the game or exit.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.