#!/bin/bash

if [[ -z $1 ]]; then
    echo "Usage: $0 <Jenkins build number>"
    exit 1
fi

refpath=$(dirname "$BASH_SOURCE")/../pyutils/perftest/references

for domain in 128 256; do
    for label in ault dom daint-cn tsa; do
        for env in cray hip nvcc_cray nvcc_cray_cxx20 nvcc_gcc; do
            current="${label%-*}_$env/$domain.json"
            src="http://jenkins-mch.cscs.ch/view/GridTools/job/GridTools_perftest_PR/$1/env=$env,label=$label/artifact/build/pyutils/perftest/results/$current"

            tmp=$(mktemp)
            wget -q -O "$tmp" "$src"

            if [[ $? == 0 ]]; then
                dst="$refpath/$current"
                mkdir -p "$(dirname "$dst")"
                mv "$tmp" "$dst"
                echo "Updated $current"
            else
                rm "$tmp"
            fi
        done
    done
done
