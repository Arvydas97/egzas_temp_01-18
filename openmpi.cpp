#include <iostream>
#include <mpi/mpi.h>
#include <iomanip>

void mainThread();
const int mainAddress = 0;

int main() {

    MPI::Init();
    int rank = MPI::COMM_WORLD.Get_rank();
    if( rank == mainAddress)
        mainThread();
    else{
        int t= rank*rank+rank*rank*rank;
        MPI::COMM_WORLD.Send(&t, 1, MPI::INT, mainAddress, 000);
    }

    MPI::Finalize();
}
void mainThread(){
    int arr[10];
    int counter =0;
    int res;

    int rank = MPI::COMM_WORLD.Get_rank();
    int t= rank*rank+rank*rank*rank;
    arr[counter++] = t;

    while(counter<10){
        MPI::COMM_WORLD.Recv(&res, 1, MPI::INT, MPI::ANY_SOURCE, MPI::ANY_TAG);
        arr[counter++] = res;
    }

    for(int i : arr){
        std::cout<< "Apskaiciuota: " << i << std::endl;
    }

};