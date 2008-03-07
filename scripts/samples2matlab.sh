#!/bin/bash
#
# This script transforms exact results into text file suitable for
# Matlab.
#
# It acts as a filter, ie. expects the file to be on its stdin, and outputs its contents
# to stdout

sed -n 's/^SAMPLE //p' | sed 's/|//' | sed 's/[0-9]\+://g' | sed 's/,/ /g'
