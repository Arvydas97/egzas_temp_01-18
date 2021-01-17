#include <iostream>
#include <fstream>
#include <cuda_runtime.h>
#include <cuda.h>
#include "device_launch_parameters.h"
#include <json.hpp>
#include "Car.h"
using json = nlohmann::json;

using namespace std;
__device__  void filterNewCars(int start_index, int end_index, Car *cars, Car *new_cars, size_t *counter);
__global__ void kernel(Car *cars, Car *new_cars, size_t *counter) ;
void printToFile(Car * result_cars, size_t counter_rez);

const size_t ARRAY_SIZE = 26;
const size_t YEAR_TO_CHECK = 2020;
const float PRICE_TO_CHECK = 15.00;


int main() {
    json cars_json;
    std::ifstream cars_file("data1.json", std::ifstream::binary);
    cars_file >> cars_json;

    Car * cars = new Car[ARRAY_SIZE];
    Car * new_cars = new Car[ARRAY_SIZE];
    Car * result_cars =new Car[ARRAY_SIZE];
    Car * device_cars, *device_new_cars;

    size_t* counter = 0;
    size_t * device_counter;
    size_t counter_rez = 0;

    cudaError_t cuda_status;

    size_t i = 0;
    for (auto& to_c : cars_json["autoList"]) {
        Car c = Car(to_c["name"], to_c["year"], to_c["price"]);
        cars[i++]=c; 
    }

    // Pradiniu duomeny perkelimas
    cuda_status = cudaMalloc((void**)&device_cars, ARRAY_SIZE*sizeof(Car));
    if(cuda_status != cudaSuccess ){
        fprintf(stderr, "cudaMalloc error \n");
        exit (0);
    }
    cuda_status = cudaMemcpy(device_cars, cars, ARRAY_SIZE*sizeof(Car), cudaMemcpyHostToDevice);
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        fprintf(stderr, "cudaMemcpy error \n");
        exit (0);
    }

    // Atrinktu auto perkelimas
    cuda_status = cudaMalloc((void**)&device_new_cars, ARRAY_SIZE*sizeof(Car));
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        fprintf(stderr, "cudaMalloc error \n");
        exit (0);
    }


    cuda_status = cudaMemcpy(device_new_cars, new_cars, ARRAY_SIZE*sizeof(Car), cudaMemcpyHostToDevice);
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        cudaFree(device_new_cars);
        fprintf(stderr, "cudaMemcpy error \n");
        exit (0);
    }

    // Counterio perkelimas
    cuda_status = cudaMalloc((void**)&device_counter, sizeof(size_t));
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        cudaFree(device_new_cars);
        fprintf(stderr, "cudaMalloc error \n");
        exit (0);
    }
    cudaMemcpy(device_counter, counter, sizeof(size_t), cudaMemcpyHostToDevice);

    // Kernelis kviecia gpu
    // sinchronizavcija
    kernel <<< 1, 5 >>> (device_cars, device_new_cars, device_counter);
    cuda_status = cudaDeviceSynchronize();
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        cudaFree(device_new_cars);
        cudaFree(device_counter);
        fprintf(stderr, "cudaDeviceSynchronize error \n");
        exit (0);
    }

    // Rezultatu duomenu perkelimas is gpu
    cuda_status = cudaMemcpy(result_cars, device_new_cars, ARRAY_SIZE*sizeof(Car), cudaMemcpyDeviceToHost);
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        cudaFree(device_new_cars);
        cudaFree(device_counter);
        fprintf(stderr, "cudaMemcpy error \n");
        exit (0);
    }
    cuda_status = cudaMemcpy(&counter_rez, device_counter, sizeof(size_t), cudaMemcpyDeviceToHost);
    if(cuda_status != cudaSuccess ){
        cudaFree(device_cars);
        cudaFree(device_new_cars);
        cudaFree(device_counter);
        fprintf(stderr, "cudaMemcpy error \n");
        exit (0);
    }


    printToFile(result_cars, counter_rez);

    cudaFree(device_cars);
    cudaFree(device_new_cars);
    cudaFree(device_counter);
}


void printToFile(Car * result_cars, size_t counter_rez)
{
    ofstream results("results");
    results << "Thread"<<setw(10) << "Code"<<setw(11) << "Price" <<setw(9)<< "Year"<<endl;
    for(size_t i =0; i < counter_rez; i++){
        results<<setw(4) <<result_cars[i].thread <<setw(14) <<result_cars[i].code <<
        setw(9) <<result_cars[i].price<<setw(9) <<result_cars[i].year<< endl;
    }
    results.close();
}

__global__ void kernel(Car *cars, Car *new_cars, size_t *counter) 
{
    int thread_id = threadIdx.x;
    const auto slice_size =ARRAY_SIZE / blockDim.x;
    unsigned int start_index = slice_size*thread_id;
    unsigned int end_index;

    if(thread_id == blockDim.x - 1)
        end_index =ARRAY_SIZE;
    else
        end_index = slice_size*(thread_id + 1);
   
    filterNewCars(start_index, end_index, cars, new_cars, counter);
   
}

__device__ void filterNewCars(int start_index, int end_index, Car *cars, Car *new_cars, size_t *counter)
{
    int thread_id = threadIdx.x;
    size_t value;
    for(auto i = start_index; i < end_index; i++){
        int z = 0;
        char code[20]={}; 
        while (cars[i].name[z] != '\0')
            code[z++] = cars[i].name[z];

        code[z++] = ':';
        code[z++] = ':';

        if(cars[i].price > PRICE_TO_CHECK)
            code[z++] = 'B';
        else
            code[z++] = 'P';
        
        if(YEAR_TO_CHECK ==0){
            value = atomicAdd((unsigned int *)&counter[0], 1);
            for(size_t j =0;j<sizeof(cars[i].code);j++)
                cars[i].code[j] = code[j];
            
            cars[i].thread = thread_id;
            new_cars[value] = cars[i];
        }    
        if( cars[i].year == YEAR_TO_CHECK){
            value = atomicAdd((unsigned int *)&counter[0], 1);
            for(size_t j =0;j<sizeof(cars[i].code);j++)
                cars[i].code[j] = code[j];
            
            cars[i].thread = thread_id;
            new_cars[value] = cars[i];
        }
    }
}