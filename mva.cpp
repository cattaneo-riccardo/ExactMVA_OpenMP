#include <iostream>
#include <chrono>
#include <fstream>
#include <vector>
#include <math.h>
#include <getopt.h>
#include <omp.h>

#define FILENAME_RESIDENCE "./residences.txt"

#define NUM_STATIONS_DEFAULT 512
#define NUM_JOBS_DEFAULT 6e4
#define THINK_TIME_DEFAULT 0

#define ZERO_APPROX 1e-3

void exactMVA(std::vector<double> &response, const std::vector<double> &demand, uint num_stations, uint tot_jobs, double think_time=0)
{
    std::vector<double> num_jobs(num_stations, 0.0); //Initialize number of jobs in each station to zero
    double tot_resp=0.0, thr=0.0;
    //Main cycle of Exact MVA Algorithm
    for (uint jobs=1; jobs<=tot_jobs; jobs++)
    {
        tot_resp=0.0;
        for (uint k=0; k<num_stations; k++)
        {
            num_jobs[k]=thr*response[k];
            response[k]=demand[k]*(1+num_jobs[k]); //Num jobs contains the value of the previous iteration
            tot_resp+=response[k];
        }
        thr=((double)jobs)/(think_time + tot_resp);
    }
}

void exactMVA_MT(std::vector<double> &response, const std::vector<double> &demand, uint num_stations, uint tot_jobs, double think_time=0)
{
    std::vector<double> num_jobs(num_stations, 0.0);
    double tot_resp=0.0, thr=0.0;

    #pragma omp parallel
    {
        #pragma omp master
        { std::cout<<"Threads: "<<omp_get_num_threads()<<std::endl;}

        //Main cycle of Exact MVA Algorithm
        for (uint jobs=1; jobs<=tot_jobs; jobs++)
        {
            #pragma omp for reduction(+:tot_resp) schedule(static)
            for (uint k=0; k<num_stations; k++)
            {
                num_jobs[k]=thr*response[k];
                response[k]=demand[k]*(1+num_jobs[k]); //Num jobs contains the value of the previous iteration
                tot_resp+=response[k];
            }

            #pragma omp single
            {
            thr=jobs/(think_time + tot_resp);
            tot_resp=0.0;
            }
        }

    }
}

uint readFromFile(std::ifstream &inputFile, std::vector<double> &demands)
{
    while (inputFile.good())
    {
        std::string char_value;
        getline (inputFile, char_value, ',' );
        demands.push_back(std::stof(char_value));  //May have problems with double
    }
    int num_station=demands.size();
    int remaining_size= pow(2, ceil(log(demands.size())/log(2)))-demands.size();
    demands.insert(demands.end(), remaining_size, 0);

    return num_station;
}

void generateRandom(std::vector<double> &demands, uint num_stations)
{
    const double MULT_FACTOR=0.8;
    srand(time(nullptr));
    for (uint k=0; k<num_stations; k++)
        demands.push_back(((double)rand())*MULT_FACTOR/RAND_MAX);

    int remaining_size= pow(2, ceil(log(demands.size())/log(2)))-demands.size();
    demands.insert(demands.end(), remaining_size, 0);
}

void checkArrays(std::vector<double> &arr1, std::vector<double> &arr2)
{
    uint fails=0;
    double max_diff=0.0;
    if (arr1.size()==arr2.size())
    {
        for (uint i=0; i<arr1.size(); i++)
        {
            double diff=fabs(arr1[i]-arr2[i]);
            if (diff>ZERO_APPROX)
            {
                fails++;
            }
            if (diff>max_diff)
                max_diff=diff;
        }
        if (fails==0)
            std::cout<<"Arrays are (almost) Equals."<<std::endl;
        else
            std::cout<<"ATTENTION: Residences with difference grater than "<<ZERO_APPROX<<": "<<fails<<std::endl;
        std::cout<<"Max Difference: "<<max_diff<<std::endl;
    }
    else
    {
        std::cout<<"Arrays to be compared have different sizes! "<<std::endl;
    }
}

int main(int argc, char *argv[])
{
    using namespace std::chrono;
    std::ifstream inputFile;

    std::vector<double> demands;
    uint num_stations=NUM_STATIONS_DEFAULT;
    double think_time=THINK_TIME_DEFAULT;
    uint num_jobs=NUM_JOBS_DEFAULT;

    ////////////////////////////////////////////////////////////////////////////////
    //parsing input arguments
    ////////////////////////////////////////////////////////////////////////////////
    int next_option;
    //a string listing valid short options letters
    const char* const short_options = "n:z:d:k:h:";
    //an array describing valid long options
    const struct option long_options[] = { { "help", no_argument, NULL, 'h' }, //help
          { "demands", required_argument, NULL, 'd' }, //demands file
          { "think", required_argument, NULL, 'z' }, //think time
          { "jobs", required_argument, NULL, 'n' }, //number of jobs
          { "stations", required_argument, NULL, 'k' }, //Number of stations, if -d not specified
          { NULL, 0, NULL, 0 } /* Required at end of array.  */
    };

    next_option = getopt_long(argc, argv, short_options, long_options, NULL);
    while(next_option != -1)
    {
        switch (next_option)
        {
            case 'n':
                num_jobs=atoi(optarg);
                break;
            case 'z':
                think_time=(double)atof(optarg);
                break;
            case 'k':
                num_stations=atoi(optarg);
                break;
            case 'd':
                inputFile.open(optarg);
                if (inputFile.is_open() && num_stations==NUM_STATIONS_DEFAULT){
                    try
                    {  num_stations=readFromFile(inputFile, demands);     }
                    catch (std::invalid_argument exc)
                    {  std::cerr<< " ---! Demands File has wrong format !--- " << std::endl; exit(0);}
                    inputFile.close();
                    break;
                }
            case '?':
            case 'h':
            default: /* Something else: unexpected.  */
                std::cerr << std::endl << "USAGE: " << argv[0] << std::endl;
                std::cerr << "[-n NUMBER_JOBS] - Specify Number of Jobs" << std::endl;
                std::cerr << "[-z THINK_TIME] - Specify a Think Time" << std::endl;
                std::cerr << "[-d FILEPATH] - Specify File Path of Demands" << std::endl;
                std::cerr << "---------------------------------------------------" << std::endl;
                std::cerr << "In the input text file, Demands values should be separated by comma, without spaces. " << std::endl;
                std::cerr << "The output text file with all Residence Times will be saved at path " <<FILENAME_RESIDENCE<< std::endl;
                std::cerr << std::endl;
                exit(EXIT_FAILURE);
        }
        next_option = getopt_long(argc, argv, short_options, long_options, NULL);
    }


    if(demands.size()==0){
        generateRandom(demands, num_stations);
    }

    std::vector<double> responseST(num_stations, 0);
    ///CPU computation
    high_resolution_clock::time_point start_time = high_resolution_clock::now();
    exactMVA(responseST, demands, num_stations, num_jobs, think_time);
    high_resolution_clock::time_point end_time = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(end_time - start_time);
    std::cout<<"Time required by Single-Threaded Exec: "<<time_span.count()<<std::endl;

    //Printing Results
    double sys_resp=0.0;
    for (uint k=0; k<num_stations; k++)
        sys_resp+=responseST[k];
    double throughput=((double)num_jobs)/(think_time+sys_resp);
    std::cout << "Global Throughput: "<<throughput<<std::endl;
    std::cout<<"System Response Time: "<<sys_resp<<std::endl;
    std::cout<<"---------------------------------------------"<<std::endl;

    std::vector<double> responseMT(num_stations, 0);
    ///OpenMP MultiThreaded Execution
    start_time = high_resolution_clock::now();
    exactMVA_MT(responseMT, demands, num_stations, num_jobs, think_time);
    end_time = high_resolution_clock::now();
    time_span = duration_cast<duration<double>>(end_time - start_time);
    std::cout<<"Time required by Multi-Threaded Exec: "<<time_span.count()<<std::endl;

    //Printing Results
    sys_resp=0.0;
    for (uint k=0; k<num_stations; k++)
        sys_resp+=responseST[k];
    throughput=num_jobs/(think_time+sys_resp);
    std::cout << "Global Throughput: "<<throughput<<std::endl;
    std::cout<<"System Response Time: "<<sys_resp<<std::endl<<std::endl;

    //Check equality between CPU and GPU Arrays
    checkArrays(responseST, responseMT);

    //Saving Residence Times on File
    std::ofstream residence_file;
    residence_file.open(FILENAME_RESIDENCE);
    for (uint i=0; i<num_stations; i++)
    {
        residence_file<<responseST[i]<<",";
        if ((i+1)%10==0)
            residence_file<<std::endl;
    }

    residence_file<<std::endl<<std::endl;
    for (uint i=0; i<num_stations; i++)
    {
        residence_file<<responseMT[i]<<",";
        if ((i+1)%10==0)
            residence_file<<std::endl;
    }
    std::cout<<"Residence Times saved in text file at: '"<<FILENAME_RESIDENCE<<"'"<<std::endl;
    residence_file.close();

    return 0;
}
