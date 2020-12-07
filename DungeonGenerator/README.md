## Table of contents
* [General Information](#general-information)
* [Technologies](#technologies)
* [Setup](#setup)
* [Features](#features)
* [To Do](#to-do)
* [Sources](#sources)

## General Information
This project is intended to serve as a robust and customizable room generator
for use in semi-random map creation. The simplest use case is for quick
creation of dungeon maps in a roleplaying game such as D&D, though it is
possible to tie the code into a game as a level generator.

## Technologies
Created with:
* Python version 3.9.0
* numpy version 1.19.4
* Pillow version 8.0.1

## Setup
This project can be installed through the following:  
  Clone the repository  
  Create a virtual environment  
  Install the dependencies using 'pip install -r requirements.txt'  
At this point the user can import DungeonGenerator.py and begin using it in their own projects

## Features
  * Generate rooms of variable size/distance from each other
  * Create maze around rooms

#### To Do
  * Connect rooms to corridors
  * More varied room generation (cellular automata?)
  * Prune dead end corridors

## Sources
http://journal.stuffwithstuff.com/2014/12/21/rooms-and-mazes/  
http://weblog.jamisbuck.org/2011/1/27/maze-generation-growing-tree-algorithm
