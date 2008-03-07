#!/bin/bash
#
# Transform celar domain files so that they are suitable for Matlab

# Load domains
datadir="$1"

while read domainId domain;
do
    domains[$domainId]=$domain;
done <"${datadir}/dom.txt"

# Load variables

while read variableId domainId constraints;
do
    echo ${domains[$domainId]};
done <"${datadir}/var.txt"
