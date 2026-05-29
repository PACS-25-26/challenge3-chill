#define OMPI_SKIP_MPICXX 1

#include "LaplaceSolver.hpp"
#include <mpi.h>
#include <iostream>

//Compile with: make
//Execute with: mpirun -np [number of processors] ./main [n, dimension of problem] [max iterations] [tolerance]
int main(int argc, char** argv) {
    //MPI setup
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //Default values
    int n = 128;
    int max_iter = 100000;
    double tol = 1e-6;

    //User-assigned values
    if (argc > 1) n = std::atoi(argv[1]);
    if (argc > 2) max_iter = std::atoi(argv[2]);
    if (argc > 3) tol = std::atof(argv[3]);


    //Solver
    LaplaceSolver solver(n, max_iter, tol, rank, size);

    double start_time = MPI_Wtime(); //Start timer
    solver.solve();
    double end_time = MPI_Wtime(); //Stop timer

    //Print time on terminal
    if (rank == 0) {
        std::cout << "\n--------------------------------------------------" << std::endl;
        std::cout << "Time Elapsed: " << (end_time - start_time) << " seconds." << std::endl;
        std::cout << "--------------------------------------------------" << std::endl;
    }

    //Create .vtk file
    solver.gatherAndSave("solution.vtk");

    MPI_Finalize();
    return 0;
}