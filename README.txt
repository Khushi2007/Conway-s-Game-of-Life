# Conway's Game of Life - C Implementation

## Description
A C implementation of Conway's Game of Life, a cellular automaton devised by mathematician John Horton Conway in 1970.

## About the Game
The Game of Life is a zero-player game where the evolution is determined by its initial state. The universe is an infinite two-dimensional grid of square cells, each in one of two states: alive or dead.

### Rules
1. Any live cell with 2 or 3 live neighbors survives
2. Any dead cell with exactly 3 live neighbors becomes alive
3. All other live cells die, and all other dead cells stay dead

## Features
- Console-based visualization
- Customizable initial patterns
- Step-by-step or continuous simulation
- Audio integration
- Configurable grid size and speed

## Building the Project
To build the project, ensure you have a C compiler installed (like GCC).
This project already includes a Makefile for easy compilation.
For Windows (MinGW): execute `mingw32-make` in the project directory terminal.
For Linux/MacOS: execute `make` in the project directory terminal.
This will generate an executable file named `main`.

## Running the Simulation
After building the project, run the executable from the terminal: ./main