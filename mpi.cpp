///mpicxx -o main.o -Ilib/nlohmann_json_cmake_fetchcontent/include -L/usr/lib/x86_64-linux-gnu/openmpi/include main.cpp ResultMonitor.cpp Car.cpp DataMonitor.cpp
///usr/bin/mpirun -np 4 main.o
/// arvymikl@mpilab.elen.ktu.lt
///arvymikl  i>l_n!mAK
/// mpicxx -o main.o main.cpp
///mpirun -np 4 main.o
/// anydesk 6+keE2K&3Q
#include <iostream>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <omp.h>
#include "Car.h"
#include "DataMonitor.h"
#include "Worker.h"
#include "ResultMonitor.h"
#include <mpi/mpi.h>
#include <iomanip>

vector<Car> ReadFromFile(string file);
void from_json(const json& j, Car& p);
void to_json(json& j, const Car& p);
void mainThread();
void dataThread();
void workerThread();
void resultThread();
void print(const string& fileName, const vector <Car>& cars );
//string to_string( const Car& p);
Car to_car(string car);

const int mainAddress = 0;
const int dataAddress = 1;
const int resultAddress = 2;
const int newCarCoff = 60;
const int poison = -1;
const int totalWorkers = 3;

/// TAGS
/// 999 -connections betweens threads
/// 00* -main messages
/// 20* -result messages
/// 50* -workers

int main()
{
    MPI::Init();
    int rank = MPI::COMM_WORLD.Get_rank();
    if( rank == mainAddress)
        mainThread();
    else if (rank == dataAddress)
        dataThread();
    else if (rank == resultAddress)
        resultThread();
    else
        workerThread();

    MPI::Finalize();
}

void mainThread(){
    vector<Car> cars;
    vector<Car> resCars;
    int rank = MPI::COMM_WORLD.Get_rank();
    cars = ReadFromFile("IFF8_10_MiklovisA_L1_dat_1.json");

    int total = static_cast<int>(cars.size());
    MPI::COMM_WORLD.Send(&total, 1, MPI::INT, dataAddress, 000);
    for (int i = 0; i < total; i++) {
        int res = dataAddress;
        while(res == dataAddress){
            MPI::COMM_WORLD.Send(&rank, 1, MPI::INT, dataAddress, 999);
            MPI::COMM_WORLD.Recv(&res, 1, MPI::INT, dataAddress, 999);
        }

        string message = cars[i].to_string();
        int serialized_size =static_cast<int>(message.size());
        const char* serialized_chars = message.c_str();
        MPI::COMM_WORLD.Send(&serialized_size, 1, MPI::INT, dataAddress, 001);
        MPI::COMM_WORLD.Send(serialized_chars, serialized_size, MPI::CHAR,dataAddress,002);

    }
    while(true){
        int size;

        MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, resultAddress, 201);
        char serialized[size];
        if(size == poison){
            break;
        }
        MPI::COMM_WORLD.Recv(&serialized, size, MPI::CHAR, resultAddress, 202);
        //cout << "serelized" << serialized<< endl; // is kur siukles???

        resCars.push_back(to_car(string(serialized, 0, size)));
    }
    print("IFF8_10_MiklovisA_L1_rez.txt", resCars);

}
void dataThread(){
    DataMonitor dataMonitor;
    int h = 0;
    int total;
    MPI::COMM_WORLD.Recv(&total, 1, MPI::INT, mainAddress, 000);

    while( dataMonitor.total < total ){
        int source;
        MPI::COMM_WORLD.Recv(&source, 1, MPI::INT, MPI::ANY_SOURCE, 999);

        if(source == mainAddress){
            if(dataMonitor.counter < 15){
                MPI::COMM_WORLD.Send(&source, 1, MPI::INT, source, 999);
                int size;
                MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, mainAddress, 001);
                char serialized[size];
                MPI::COMM_WORLD.Recv(serialized, size, MPI::CHAR, mainAddress, 002);
                Car c = to_car(string(serialized, 0, size));
                dataMonitor.put(c);
            }else {
                MPI::COMM_WORLD.Send(&dataAddress, 1, MPI::INT, source, 999);
            }
        }else if(dataMonitor.counter > 0){
            dataMonitor.total++;
            MPI::COMM_WORLD.Send(&source, 1, MPI::INT, source, 999);

            string car = dataMonitor.takeOut().to_string();
            int serialized_size =static_cast<int>(car.size());
            const char* serialized_chars = car.c_str();

            MPI::COMM_WORLD.Send(&serialized_size, 1, MPI::INT, source, 501);
            MPI::COMM_WORLD.Send(serialized_chars, serialized_size, MPI::CHAR, source,502);
        }else{
            MPI::COMM_WORLD.Send(&dataAddress, 1, MPI::INT, source, 999);
        }
    }
    int temp;
    while (h < totalWorkers){
        MPI::COMM_WORLD.Recv(&temp, 1, MPI::INT, MPI::ANY_SOURCE, 999);
        MPI::COMM_WORLD.Send(&poison, 1, MPI::INT,temp, 999);
        h++;
    }
}
void workerThread(){
    int rank = MPI::COMM_WORLD.Get_rank();
    while (true){
        int res = dataAddress;
        while(res == dataAddress){
            MPI::COMM_WORLD.Send(&rank, 1, MPI::INT, dataAddress, 999);
            MPI::COMM_WORLD.Recv(&res, 1, MPI::INT, dataAddress, 999);
        }
        if(res == -1)
            break;

        int size;
        MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, dataAddress, 501);
        char serialized[size];
        MPI::COMM_WORLD.Recv(&serialized, size, MPI::CHAR, dataAddress, 502);
        Car car = to_car(string(serialized, 0, size));
        car.CalculatePriceCoefficient();
        if( car.coff > newCarCoff) {
            MPI::COMM_WORLD.Send(&rank, 1, MPI::INT, resultAddress, 999);
            MPI::COMM_WORLD.Recv(&res, 1, MPI::INT, resultAddress, 999);

           string message = car.to_string();
           int sizea = static_cast<int>(message.size());
           const char* serialized_chars = message.c_str();

            MPI::COMM_WORLD.Send(&sizea, 1, MPI::INT, resultAddress, 503);
            MPI::COMM_WORLD.Send(serialized_chars, sizea, MPI::CHAR,resultAddress,504);
        }
    }
   MPI::COMM_WORLD.Send(&poison, 1, MPI::INT, resultAddress, 999);
}
void resultThread(){
    ResultMonitor resMonitor;
    int source;
    int deadWorker=0;

    while(true){
        MPI::COMM_WORLD.Recv(&source, 1, MPI::INT, MPI::ANY_SOURCE, 999);
        if(source == -1){
            deadWorker++;
            if(deadWorker == totalWorkers){
                resMonitor.sendCars(mainAddress);
                break;
            }
           continue;
        }
        MPI::COMM_WORLD.Send(&source, 1, MPI::INT, source, 999);

        int size;
        MPI::COMM_WORLD.Recv(&size, 1, MPI::INT, source, 503);
        char serialized[size];
        MPI::COMM_WORLD.Recv(&serialized, size, MPI::CHAR, source, 504);

        resMonitor.put(to_car(string(serialized, 0, size)));
    }
    MPI::COMM_WORLD.Send(&poison, 1, MPI::INT, mainAddress, 201);
}

vector<Car>ReadFromFile(string file)
{
    using json = nlohmann::json;
    std::ifstream in(file,std::ifstream::in);
    json j = json::parse(in);
    vector<Car> Cars;

    for (auto& to_c : j["autoList"]) {
        auto c = to_c.get<Car>();
        Cars.push_back(c);
    }
    return Cars;
}
void from_json(const json& j, Car& p)
{
    j.at("name").get_to(p.name);
    j.at("year").get_to(p.year);
    j.at("price").get_to(p.price);
}
void to_json(json & j, const Car& p)
{
    j = json{ {"name", p.name}, {"year", p.year}, {"price", p.price} };
}
Car to_car(string car){
    using json = nlohmann::json;
    json j = json::parse(car);
    Car c ;
    j.at("name").get_to(c.name);
    j.at("year").get_to(c.year);
    j.at("price").get_to(c.price);
    j.at("coff").get_to(c.coff);
    return c;
}
void print(const string& fileName, const vector <Car>& cars ){
    ofstream out(fileName);
    out << "  #  |     Coff       |      Name      |    Price     | Year" << endl <<
        "---------------------------------------------------------------" << endl;
    int i=1;
    for (Car car :cars) {
        if(car.name != ""){
            out << setw(3)<< i++ << setw(12) << car.coff << " " << setw(17) << car.name
                << " " << setw(13) << car.price << " " << setw(12) << car.year << endl;
        }
    }
    out.close();
}