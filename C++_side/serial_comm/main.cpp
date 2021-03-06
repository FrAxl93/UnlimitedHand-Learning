#include <iostream>
#include <cstdio>
#include <ctime>
#include <stdio.h>
#include "SerialHandler.h"
#include <queue>
#include <condition_variable>
#include <fstream>

#include <thread>
#include <mutex>

#define N_CHANNELS 8
#define WAIT_FOR_INPUT 0
#define STOP 1000

std::mutex mut;
std::queue<__int16 unsigned> input_data_queue;

std::condition_variable data_cond;

bool flag_time_elapsed = 0;
bool acquisition_completed = 0;
int movement;

/*

    Documentation in "readme.md"

*/

void read_data_thread(double ms_time)
{
    //For COM > 9 USE \\\\.\\COM10

    SerialHandler _S("\\\\.\\COM10",
                     115200,
                     1,
                     0,
                     8);

    _S.init_serial_port();
    _S.init_timeouts();

    __int8 unsigned start_receiving = 0x73;
    __int8 unsigned stop_receiving = 0x80;
    std::clock_t start;
    double duration;
    int samples;

    //while(!_S.WriteAChar(&start_receiving)); //starts communication


    while(!acquisition_completed)
    {

        std::cin >> movement;

        flag_time_elapsed = false;
        if(movement == STOP){
            acquisition_completed = true;
            flag_time_elapsed = true; //don't enter the while
        }

        while(!_S.WriteAChar(&start_receiving)); //starts communication

        start = std::clock(); //start counting
        duration=0;
        samples = 0;

        std::cout << "Start movement \n";

        while(!flag_time_elapsed)
        {
            std::lock_guard<std::mutex> lk(mut); //lk is the lock guard

            if(_S.Read_FSM(input_data_queue))  //if read succeded
            {
                data_cond.notify_one();
            }

            samples++;


            if (samples == ms_time){
                duration = ( std::clock() - start ) / (double) CLOCKS_PER_SEC;
                std::cout << "Stop movement \n";
                flag_time_elapsed = true;
                while(!_S.WriteAChar(&stop_receiving)); //stops communication
                while(!_S.FlushBuffer()); //Flush Buffer (some data remains in the buffer during the time
                                          //period that passes between the last useful read and the call to "stop receiving"
                std::cout<< "Elapsed: " << duration << '\n';
            }
        }
    }
}

void process_data_thread(char* namefile){

    std::ofstream record;
    record.open(namefile);

    int samples;

    while(!acquisition_completed)

    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait( lk, []{return !input_data_queue.empty();});
        int upper_limit = input_data_queue.size();
        //std::cout << "size queue:" << upper_limit << '\n';
        //discard corrupted lines and at the same time check if we are aquiring new data
        if(upper_limit != N_CHANNELS || !flag_time_elapsed)
        {
            for(int i = 0;i<N_CHANNELS;i++)
            {
                //std::cout << input_data_queue.front() << std::endl;
                record << input_data_queue.front() << ',';
                input_data_queue.pop();
            }

            record << movement << std::endl; //x will be the type of movement.
        }


        lk.unlock();

    }

    record.close();

}

int main(int argc, char** argv)
{
    double ms_time = 1;
    char* namefile = "default.txt";
    for(int arg = 0;arg<argc;arg++)
    {
        char* new_arg = argv[arg];

        char identifier = new_arg[1];

        switch(identifier)
            {
            case 't':
                ms_time = static_cast<double> (atoi(argv[arg+1]) );
                //ms_time = ms_time/1000;
            case 'n':
                namefile = argv[arg+1];

            }
    }

    std::cout << "Starting Counting: " << ms_time << '\n';

    std::thread read(read_data_thread,ms_time);
    std::thread process(process_data_thread,namefile);


    read.join();
    process.join();

    return 0;
}
