// A Simple Batch Operating System - Psim3/C++ package
//
// J. Garrido, August 2015
// 
// This system processes one job at a time 
// If the memory requirements of the job are greater than the 
// size of available memory, or if the queue is full, 
// the job is rejected. 
// The job sojourn time is the total time that the job spends inside 
// the computer system 
// File: batch.cpp
// 
#include "proc.h" 
#include "queue.h"

using namespace std;

//
   class CPU;
   class job;
   class environment;
//
   squeue *job_queue;                 // queue for waiting jobs
// System parameters
   int qqsize = 200;             // size of input queue 
   int totmem = 40;              // total system memory
//
// Workload parameters
   double mean_int_arr;               // mean inter-arrival period 
   double mean_ser;                   // mean service period 
   long mem_l;                        // lower bound for memory demand
   long mem_u;                        // upper bound for memory demand
//
// Globals
   double simperiod;                  // simulation period 
   double close_arrival;              // time to stop 
// 
   double time_mem = 0.0;             // product time x memory 
   double acc_proc = 0.0;             // accumulated processor time 
   unsigned arrived = 0;              // number of arrived jobs 
   unsigned rejected = 0;             // number of rejected jobs 
   double idle_time = 0.0;            // acc processor idle time 
   double acc_sojourn_t = 0.0;        // accumulated job sojourn time 
   double acc_wait = 0.0;             // acc job wait time 
   unsigned completed_jobs = 0;       // number of completed jobs 
   
// 
//  Global objects 
   CPU *proc;              // processor
   environment * arr_obj;  // environment generating jobs
   simulation * run;
   
   // files for report
   ofstream statf;
   ofstream tracef;
// 
//  Specifications of processes 
// 
   class CPU: public process {
      job *curr_job;                 // current job object 
      double start;                  // time of start of service 
      double jserv_per;              // cpu job service period 
   public: 
      CPU(string iname);                  // constructor 
      void Main_body();
   };
// 
   class job: public process {
      double arrival_time;
      double start;              // start of service 
      double jwait;              // job wait period 
      double service;            // job service period 
      long jmem;                 // job memory requirement 
      unsigned jobnum;           // job number 
	  string cjobname;           // job name
   public: 
      job(string jname, double ser, long mem);
      double  get_service();
      long  get_mem();
      void  set_start();
      void Main_body();
   };
// 
   class environment: public process {
      erand *arr_period;          // random inter-arrival period 
      erand *ser_period;          // random job service 
      urand *jsize;               // random job memory requirement 
      string sname;               // environment name 
      long memreq;                // job memory requirements 
   public: 
      environment(string einame, double arr_mean, double ser_mean);
      void Main_body();
   };
   
   class batch : public process {
   
   public:
      batch(string bname);
      void Main_body(void);
   };
// 
//   implementations 
// 
   CPU::CPU(string cpuname): process (cpuname) {
   // phase = RUN_JOB;
      cout << "Processor object created" << endl;
   };
// 
   void CPU::Main_body(void) { 
      double t_service;              // service period
      double simper = get_simper();  // simulation period
      long jmemreq;
   
      // cout << "Starting CPU Main_body " << endl;
      while (get_clock() < simper) { 
         start =  get_clock();
         if ( !job_queue->empty()) { 
            curr_job = ( job*) job_queue->out();
            curr_job->set_start(); 
         
            t_service = curr_job->get_service();
            delay(t_service);
            cout << "CPU completed service of Job " << curr_job->get_name()
                 << endl;
            tracef << "CPU completed service of Job " << curr_job->get_name()
                 << endl;
            completed_jobs++;
            jserv_per = get_clock() - start;
            acc_proc += jserv_per;   // total accumulated CPU service
            jmemreq = curr_job->get_mem();
            time_mem += jmemreq * jserv_per;                     
            // cout << "Accumulated service period: " << acc_proc << endl;
            if ( curr_job == NULL )
               cout << "CPU with NULL Job " << endl;
            else
               curr_job->reactivate();   // delay(0);
         }
         else {
            // queue_empty();
            // void queue_empty() { 
         
            cout << "CPU goes idle at " <<  get_clock() << endl;
         
            sleep();
            idle_time += get_clock() - start; // CPU idle period
            cout << "CPU is reactivated at " <<  get_clock() << endl;
         }
      } // endwhile
      cout << "terminating CPU " << endl;
      tracef << "terminating CPU " << endl;
      terminate();
   };
// 
// the constructor is called from 'environment' 
   job::job(string jobname, double ser, long memreq): process (jobname) {
      arrival_time =  get_clock();
      service = ser;     // job service period
      jmem = memreq;     // job memory demand
      arrived++;
      cjobname = jobname;
      // cout << jobname << " CPU service req: " << ser << " mem req: "
      //      << memreq << endl;
   
   };
// 
   double  job :: get_service(void) {
      return ( service );
   };
// 
   long  job :: get_mem(void) {
      return ( jmem );
   };
// 
   void  job :: set_start(void) {
     // this function sets the starting time of the job and displays 
     // the data (name, jobnum, clock time and service period) 
     start =  get_clock();
     cout << cjobname  << " starts service at "
          << start << " with service " << service << endl;
     tracef << cjobname  << " starts service at "
          << start << " with service " << service << endl;
   };
// 
   void job::Main_body(void) { 
      jobnum = arrived;
      cout << cjobname  << " requiring service "
         << service << " arrives at time " <<  get_clock()
         << endl;
      if ( jmem > totmem || job_queue->full() ) { 
         // there  is not sufficient system memory for this job 
         // or the input queue is already full, reject
         if(jmem > totmem) {
            cout << cjobname << " rejected for memory " << endl;
            tracef << cjobname << " rejected for memory " << endl;
         }
         if(job_queue->full()) {
            cout << cjobname << " rejected queue full " << endl;
            tracef << cjobname << " rejected queue full " << endl;
         }
         rejected++;
         terminate();
      }
      else {
         if ( proc->idle() && proc != NULL ) { 
            cout << cjobname << " reactivating CPU" << endl;
            proc->reactivate(); 
         }
         // job will wait in queue until processor executes it
         if(! job_queue->full()) {
            job_queue->into(this );
            cout << cjobname << " joins queue and waiting" << endl;
            tracef << cjobname << " joins queue and waiting" << endl;
            sleep(); // suspend or deactivate
         
            // Service completed, prepare to exit system 
            acc_sojourn_t += ( ( get_clock()) - (arrival_time)) ;
            jwait = (start) - (arrival_time);
            // cout << cjobname << " wait period " << jwait << endl;
            acc_wait += jwait;
            // display "Accumulated sojourn period: ", acc_sojourn_t; 
            cout << cjobname << " terminates at time "
                 <<  get_clock() << endl;
            tracef << cjobname << " terminates at time "
                 <<  get_clock() << endl;
         }
         else
            cout << "Job queue full" << endl;
      
      }
      cout << "Terminating " << cjobname << endl;
      tracef << "Terminating " << cjobname << endl;
      terminate();
   };
// 
   environment::environment(string ename, double arr_mean, double ser_mean):
   process (ename) {
   // create objects for random variables 
      arr_period = new erand (arr_mean, 7);
      ser_period = new erand (ser_mean, 3);
      jsize = new urand (mem_l, mem_u);
      sname = ename;
      cout << get_name() << " created with " << arr_mean << " " << ser_mean << endl;
   };
// 
   void environment::Main_body(void) { 
      double ttr;
      int j = 0;
      string numj;
      string jobn;
      job *mjob;
      double simper = get_simper();             
     // cout << "Starting environment Main_body " << "Close arrival: "
     //      << close_arrival << endl;
      while (get_clock() < simper) {
         // generate random value for inter-arrival period 
         ttr = arr_period->fdraw();
         // cout << "Inter-arrival: " << ttr << endl;
         delay(ttr);
         if (  get_clock() > close_arrival) {
            cout << "Terminating environment " << endl;
            tracef << "Terminating environment " << endl;
            terminate(); 
            return;
         }
         else {
            // generate random value for job service 
            ttr = ser_period->fdraw();
            // generate random value for job size 
            memreq = jsize->draw();
            // create job object with appropriate parameters
            // cout << "environment to create a new Job" << endl;
            jobn = "Job";
            numj = intstr(j); // int to string
            jobn.append(numj);
            mjob = new job(jobn, ttr, memreq);
            j++;
            cout << "environment generating " << jobn << " at " << get_clock() << endl;
            mjob->pstart();
         }
      }  // endwhile
   };

//
   
   batch::batch(string batchn) : process (batchn) {
   
      // cout << "Process " << s << " created" << endl;
   };
//
   void batch::Main_body(void) {
      // cout << "In Main_body of: " << get_name() << endl;
      proc = new CPU("Processor");
   
      arr_obj = new environment("environment", mean_int_arr, mean_ser);
      // cout << "Starting thread objects" << endl;
      arr_obj->pstart();   // start environment object
      proc->pstart();
      run->start_sim(simperiod);

      cout << endl << "Service factor: "
          << acc_proc / acc_sojourn_t << endl;
      cout << "Processor utilization: "
          << acc_proc / simperiod << endl;
      cout << "Total jobs arrived: " << arrived << endl;
      cout << "Total jobs rejected: " << rejected << endl;
      cout << "Throughput (jobs completed): " << completed_jobs << endl;
      cout << "Proportion Rejected: "
          << ( double(rejected) / double(arrived)) << endl;
      statf << endl << "Service factor: "
          << acc_proc / acc_sojourn_t << endl;
      statf << "Processor utilization: "
          << acc_proc / simperiod << endl;
      statf << "Total jobs arrived: " << arrived << endl;
      statf << "Total jobs rejected: " << rejected << endl;
      statf << "Throughput (jobs completed): " << completed_jobs << endl;
      statf << "Proportion Rejected: "
          << ( double(rejected) / double(arrived)) << endl;

      if ( completed_jobs > 0) { 
         cout << "Average job wait period: "
             << acc_wait / double(completed_jobs) << endl;
         statf << "Average job wait period: "
             << acc_wait / double(completed_jobs) << endl; 
      }
      cout << "Average memory used: "
          << time_mem / simperiod << endl;
      statf << "Average memory used: "
          << time_mem / simperiod << endl;
   
   };

int strint(string str) {
  std::stringstream buf(str);
  long int num;
  buf >> num;
  return num;
}

double strdouble(string str) {
   std::stringstream buf(str);
   double num;
   buf >> num;
   return num;
}
// 
   int main() { 
      batch * application;
      string lline;
      simperiod = 3000.0;
      close_arrival = 1000.0;
      mean_int_arr = 10.55;
      mean_ser = 12.65;
      mem_l = 8;
      mem_u = 14;
      run = new simulation("Simple Batch System");  
      run->set_statfile("batch_statf.txt", statf);
      run->set_tracefile("batch_tracef.txt", tracef);
	  
      // create queue with size qqsize for incoming jobs 
      job_queue = new squeue ("cust", qqsize);
      application = new batch("Batch");
      cout << "\n Simulation parameters" << endl;
      cout << "Simulation interval: (" << simperiod << ") ";

      getline(cin, lline);
      if ( ! lline.empty() ) 
          simperiod = strdouble(lline);
      cout << "Close arrival time: (" << close_arrival << ") ";
      getline(cin, lline);
      if (! lline.empty() ) 
	  close_arrival = strdouble(lline);
      cout << "\n Workload Parameters" << endl;
      cout <<  "Mean inter-arrival: (" << mean_int_arr << ") ";
      getline(cin, lline);
      if (! lline.empty() ) 
	   mean_int_arr = strdouble(lline);  	  
      cout << "Mean CPU service demand: (" << mean_ser << ") ";
      getline(cin, lline);
      if (! lline.empty() )    
	   mean_ser = strdouble(lline);
	  	  
      cout << "Lower bound mean memory demand: (" << mem_l << ") ";
      getline(cin, lline);
      if (! lline.empty() )    
	 mem_l = strdouble(lline);
	  
      cout << "Upper bound mean mem demand: (" << mem_u << ") ";
      getline(cin, lline);
      if (! lline.empty() )    
           mem_u = strdouble(lline);  	  
      cout << "\n Systems parameters" << endl;
      cout << "Queue capacity: (" << qqsize << ") ";
      getline(cin, lline);
      if (! lline.empty() )    
          qqsize = strdouble(lline);
	  	  
      cout << "System memory: (" << totmem << ") ";
      getline(cin, lline);
      if (! lline.empty() )    
          totmem = strdouble(lline);
   
      statf << "\n Simulation parameters" << endl;
      statf << "Simulation interval: " << simperiod << " " << endl;
      statf << "Close arrival time: " << close_arrival << endl;
      statf <<  "Mean inter-arrival: " << mean_int_arr << endl;
      statf << "Mean CPU service demand: " << mean_ser << endl;
      statf << "Lower bound mean memory demand: " << mem_l << endl;
      statf << "Upper bound mean mem demand: " << mem_u << endl;
      statf << "\n Systems parameters" << endl;
      statf << "Queue capacity: " << qqsize << endl;
      statf << "System memory: " << totmem << endl;
      statf << " " << endl;

      application->pstart();
      run->end_sim();
      // cout << "Ending simulation" << endl;
      return 0;
   
   }
