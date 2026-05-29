#!/bin/bash

#Remove this line if needed. It's used to restrict the number of OMP threads. This improves parallel computations on
#machines that are not suited for highly parallel computing
export OMP_NUM_THREADS=1

#Find the path to the 'test/' folder where this script lives
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

#Find the project root (that is, the challenge3-chill directory)
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

EXECUTABLE="$PROJECT_ROOT/main"
CSV_FILE="$SCRIPT_DIR/results.csv"

#Ensure executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    echo "Make sure you have run 'make' in the root directory."
    exit 1
fi

#Move to the project root before running MPI
cd "$PROJECT_ROOT"

echo "GridSize,Cores,Time,L2Error" > $CSV_FILE

echo ""
echo "---------------------------------------------------------"
echo "             Starting Scalability Tests...               "
echo "---------------------------------------------------------"
echo ""


#Full scale test. Loop over 1, 2, and 4 cores and increasing grid sizes
for k in 4 5 6 7 8; do
    n=$((2 ** k))
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
done

echo ""
echo "---------------------------------------------------------"
echo "    Tests complete! Results saved to $CSV_FILE"
echo "---------------------------------------------------------"
echo ""

echo "Final Results:"
column -s, -t < full_scal_test/results.csv