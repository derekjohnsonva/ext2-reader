#!/bin/bash
#
# functionality check script for Project 4A

# compile the executable `p4exp1`
make
# Run the executable `p4exp1` with the first argument as the input file
# pipe the output to a file `test.csv`
echo $1
echo $2
./p4exp1 $1 > test.csv
# if the second argument is not a file, exit with an error
if [ ! -f $2 ]; then
    echo "Input file $2 does not exist"
    rm test.csv
    exit 1
fi

# compare the output `test.csv` with the second argument
diff <(sort $2) <(sort test.csv)

# remove the temporary file
rm test.csv