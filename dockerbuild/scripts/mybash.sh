#!/bin/bash

echo "my docker bash"

while test $# -gt 0
do
    $1
    shift
done

exit 0
