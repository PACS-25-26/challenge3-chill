#define OMPI_SKIP_MPICXX 1

#include "LaplaceSolver.hpp"
#include "functions.hpp"
#include <mpi.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iomanip>

void LaplaceSolver::print_iter_info(int iter, int max_iter, double glob_error, int freq = 20){
    if (rank == 0 && iter % freq == 0) {
            // \r returns the cursor to the beginning of the line
            // std::flush forces the terminal to update immediately
            std::cout << std::scientific << std::setprecision(6);
            std::cout << "\rIter: " << iter << " / " << max_iter << " | Global Error: " << glob_error;
            std::cout << std::fixed << std::flush;
        }
}

LaplaceSolver::LaplaceSolver(int grid_size, int max_iterations, double tolerance, int mpi_rank, int mpi_size) :
                            n(grid_size), max_iter(max_iterations), tol(tolerance), rank(mpi_rank), size(mpi_size){
    h = 1.0 / (n - 1);

    //Compute local number of rows
    local_rows = n / size;

    //Find starting row. Processes with lower rank are assigned "remainder rows" 
    int remainder = n % size;
    start_row = rank*local_rows + std::min(rank, remainder);
    if (rank < remainder) {
        local_rows++;
    }

    //Assigning u and u_new all necessary memory, including enough for ghost nodes to perform the updates
    u.assign((local_rows + 2)*n, 0.0);
    u_new.assign((local_rows + 2)*n, 0.0);

}

void LaplaceSolver::solve(){

    int top_neighbor, bottom_neighbor;
    //If rank is 0, top neighbor is null
    if (rank == 0){
        top_neighbor = MPI_PROC_NULL;
    }
    else{
        top_neighbor = rank - 1;
    }
    //If rank is last (that is, size - 1), bottom neighbor is null
    if (rank == size-1){
        bottom_neighbor = MPI_PROC_NULL;
    }
    else{
        bottom_neighbor = rank + 1;
    }

    double global_error = 1.0;
    int iter = 0;

    //Main solver's loop
    while(global_error > tol && iter < max_iter){
        //Pass and receive top and bottom neighbors to processes
        //First, send information to top neighbor and await information from top neighbor
        //Since rank 0 has top_neighbor = MPI_PROC_NULL, instruction is immediately completed successfully
        
        MPI_Sendrecv(&u[n], n, MPI_DOUBLE, top_neighbor, 0,
                    &u[0], n, MPI_DOUBLE, top_neighbor, 1,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        //Then, send information to bottom neighbor and await information from bottom neighbor
        //Since rank 0 is not waiting, it immediately sends the information to rank 1, which then
        //completes the previous instruction successfully and executes the next one, which allows
        //rank 2 to complete the previous instruction, and so on...
        //This cascade ends when rank n-1 is reached, because it has bottom_neighbor = MPI_PROC_NULL and
        //therefore completes the next instruction immediately
        
        MPI_Sendrecv(&u[n*local_rows], n, MPI_DOUBLE, bottom_neighbor, 1,
                    &u[n*(local_rows + 1)], n, MPI_DOUBLE, bottom_neighbor, 0,
                    MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        
        double local_error_sq = 0.0;

        #pragma omp parallel for reduction(+:local_error_sq)
        for(int i = 1; i <= local_rows; ++i){

            //Check if row is made of boundary points. If it is, skip all operations
            int global_i = start_row + i - 1;
            if (global_i != 0 && global_i != n - 1){

                //y coordinate in the domain
                double y = global_i*h;
                for(int j = 1; j < n - 1; ++j){
                    //x coordinate in the domain
                    double x = j*h;

                    //Update matrix
                    u_new[i*n + j] = 0.25 * (u[(i - 1)*n + j] + u[(i + 1)*n + j]
                                            + u[i*n + j - 1] + u[i*n + j + 1] + f(x,y)*h*h);
                    
                    //Compute local error (squared)
                    double diff_u = u_new[i*n + j] - u[i*n + j];
                    local_error_sq += diff_u*diff_u;

                }
            }
            
        }

        //Aggregate all squared errors among all processes
        double global_error = 0.0;
        MPI_Allreduce(&local_error_sq, &global_error, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        global_error = std::sqrt(h*global_error);

        //Print on terminal
        print_iter_info(iter, max_iter, global_error);

        //Finally, set up next iteration
        std::swap(u, u_new);
        iter++;
    }

    //Final print to show terminal up to date
    print_iter_info(iter, max_iter, global_error);

    if(rank == 0){
        std::cout<<"\n\nProcess ended successfully"<<std::endl;
    }
}

void LaplaceSolver::gatherAndSave(const std::string& filename){
    //Vector containing the number of elements in each process
    std::vector<int> counts_recv(size);
    //Vector containing the displacements for each process
    std::vector<int> displacements(size);

    //Local elements for each process (excluding ghost rows)
    int local_elem = local_rows*n;

    //Gather counts_recv in process 0
    MPI_Gather(&local_elem, 1, MPI_INT, counts_recv.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    //In process 0, compute displacements
    if(rank == 0){
        displacements[0] = 0;
        for(int i = 1; i < size; ++i){
            displacements[i] = displacements[i-1] + counts_recv[i - 1];
        }
    }

    //Global U matrix (only in rank 0)
    std::vector<double> global_u;
    if(rank == 0){
        global_u.assign(n * n, 0.0);
    }

    //Gather all information and construct global U in rank 0. 
    //Gatherv is necessary because different processes may own different number of elements
    MPI_Gatherv(&u[1 * n], local_elem, MPI_DOUBLE, 
                global_u.data(), counts_recv.data(), 
                displacements.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    //In rank 0 
    if (rank == 0){
        //Compute and print global error
        double global_error = 0.0;
        for(int i = 0; i < n; ++i){
            for(int j = 0; j < n; ++j){
                double diff = global_u[i*n + j] - exact(j*h, i*h);
                global_error += diff*diff;
            }
        }

        global_error = std::sqrt(h*global_error);
        std::cout<<"L2 Error: "<<global_error<<std::endl;


        // VTK Export
        std::ofstream vtk(filename);
        vtk << "# vtk DataFile Version 3.0\nLaplace Solution\nASCII\nDATASET STRUCTURED_GRID\n";
        vtk << "DIMENSIONS " << n << " " << n << " 1\nPOINTS " << n * n << " float\n";
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                vtk << j * h << " " << i * h << " 0.0\n";
        vtk << "POINT_DATA " << n * n << "\nSCALARS u float 1\nLOOKUP_TABLE default\n";
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                vtk << global_u[i * n + j] << "\n";
        vtk.close();
    }




}