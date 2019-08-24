#!/bin/bash

errors=0

for i in $(seq 22)
do
    file="./samples/P3B-test_$i.csv"
    solution="./samples/P3B-test_$i.err"
    printf "Testing with $file\n"
    python3 ./lab3b.py $file > output
    errors+=$(diff output $solution | wc -l | xargs)
done

echo "Test completed with $((errors)) errors."