#!/bin/bash

#Remove this line if needed. It's used to restrict the number of OMP threads. This improves parallel computations on
#machines that are not suited for highly parallel computing
export OMP_NUM_THREADS=1

EXECUTABLE="main"
if [ ! -f $EXECUTABLE ]; then
    echo "Executable not found in current directory! Searching in parent directory..."
    EXECUTABLE="../main"
    if [ ! -f $EXECUTABLE ]; then
        echo "Executable not found! Make sure you have run 'make' in the root directory."
        exit 1
    fi
fi

CSV_FILE="test/results.csv"
echo "GridSize,Cores,Time,L2Error" > $CSV_FILE

echo "Starting Scalability Tests..."


#Small scale test. Loop over 1, 2, and 4 cores
n=64
for cores in 1 2 4; do
    echo "Running: Grid = $n x $n | Cores = $cores"
    
    #Run the solver: Max 200,000 iterations, 1e-6 tolerance
    #We capture the output into a variable
    OUTPUT=$(mpirun -np $cores $EXECUTABLE $n 200000 1e-6)
    
    #Parse the output using grep and awk
    TIME=$(echo "$OUTPUT" | grep "Time Elapsed" | awk '{print $3}')
    L2_ERR=$(echo "$OUTPUT" | grep "L2 Error" | awk '{print $3}')
    
    #Fallback if something goes wrong and it doesn't print
    if [ -z "$TIME" ]; then TIME="NaN"; fi
    if [ -z "$L2_ERR" ]; then L2_ERR="NaN"; fi
    
    #Append to CSV
    echo "$n,$cores,$TIME,$L2_ERR" >> $CSV_FILE
done

echo "Tests complete! Results saved to $CSV_FILE."