#!/bin/bash

for file in *.wcsp; do
    problem_id=`echo "$file" | sed -n 's/^\([^\.]*\)\..*/\1/p'`
    sed -n '2p' "$file" >"${RESULTS_PATH}/${problem_id}.domains.csv"
done
