#ifndef LAPLACESOLVER_H
#define LAPLACESOLVER_H

#include <vector>
#include <string>

// Solver class for the Laplace equation
class LaplaceSolver {
private:
    // Number of points along each coordinate direction
    int n;
    // Maximum number of iterations
    int max_iter;
    // Tolerance
    double tol;
    // Mesh spacing
    double h;
    // Process rank
    int rank;
    // Number of processes
    int size;
    // Number of rows local to each process
    int local_rows;
    // Index of first row in each process
    int start_row;
    
    // Solution matrix at the current step
    std::vector<double> u;
    // Updated solution matrix
    std::vector<double> u_new;

    void print_iter_info(int iter, int max_iter, double glob_error, int freq);
public:

    LaplaceSolver(int grid_size, int max_iterations, double tolerance, int mpi_rank, int mpi_size);
    void solve();
    void gatherAndSave(const std::string& filename);

};



#endif