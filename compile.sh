#!/bin/bash

gcc -W -g -lvlc -lm -lpthread main.c player.c player.h -o play
