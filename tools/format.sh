#!/bin/bash
#usage: ./format file
astyle --style=linux --suffix=.orig1 $@ 
astyle --indent=spaces=4 --suffix=.orig2 $@
