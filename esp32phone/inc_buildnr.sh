#!/bin/bash
if [ ! -f BUILDNR ]; then
	echo "0" > BUILDNR
else
	v=$(cat BUILDNR)
	echo $(($v + 1)) > BUILDNR 
fi

