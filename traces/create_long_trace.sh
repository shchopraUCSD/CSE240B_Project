#!/bin/bash

# Concatenate the files into a single file
cat deepsjeng.bz2 lbm.bz2 parest.bz2 x264.bz2 > ABCD.bz2

# Number of repetitions for the long trace
num_repetitions=5

# Create the long trace by repeating the concatenated file
for i in $(seq 1 $num_repetitions); do
    cat ABCD.bz2 >> long_trace.bz2
done

echo "Long trace file created successfully."
