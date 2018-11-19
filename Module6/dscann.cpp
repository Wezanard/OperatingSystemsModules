// Psim3 disk device model. 
// Disk scheduling using SCAN discipline with  
// moving-head disk. 
// Arriving requests follow an exponential distribution for inter- 
// arrival periods. 
// Every disk request needs a data transfer (read/write) of a block of 
// data of a given size (data_size). 
// The cylinder to seek is chosen from a normal distribution 
// given the number of cylinders in the disk device. 
// A disk service includes the rotational delay, seek delay, and 
// transfer time. 
// Different disk devices have different physical characteristics. 
// 
// (C) J. M. Garrido Rev. Feb. 2005. Last update June 2016.
// File: dscann.cpp 
// 
#include "proc.h" 
#include "queue.h"     // additional library classes

//
using namespace std;

class disk_server;
class request;
class arrivals;

ofstream statf;      // file for statistics 
ofstream tracef;     // file for trace

// waiting queue for requests

// physical characteristics of the disk device 
int data_size = 4;           // data block to read/write, Kbytes 
double rev_time = 4;         // revolution time (msec.) 
double transfer_rate = 3;    // in Mb/sec (Kb/msec) 
double seek_tpc = 15.6;      // seek time per cylinder (msec.) 
long num_cyl = 300;          // number of cylinders in disk 
// 
squeue *req_queue;              // wait queue for requests 
double simperiod;               // simulation period 
double close_t;                 // time to close arrivals 
// 
double req_int_arr;             // mean req inter-arrival period 
double acc_serv = 0;            // accumulated service time 
double acc_wait = 0;            // accumulated wait time 
unsigned num_req = 0;           // number of requests serviced 
unsigned arrived = 0;           // number of requests that have arrived 
unsigned cyl_travel = 0;        // total arm movement in cylinders 
// 
const int DIN = 0;
const int DOUT = 1;              // for the direction of the disk scan 
// 
disk_server *disk;
simulation *run;
arrivals *arr_obj;
// 
class disk_server: public process {
  request *serv_req;         // request being serviced 
  double start_d;            // time of start of service 
  int current_cyl;           // current cylinder heads are located 
  int direction;             // direction of scan (DIN/DOUT) 
public: 
  disk_server(string sname);
  void Main_body(void);
}; // end disk server
// 
class request: public process {
  double r_wait;        // request wait period 
public: 
  double start_d;       // request service start time 
  unsigned reqnum;      // request number 
  double arrival_time;
  double service_t;     // service period for the request 
  int req_cyl;          // cylinder to seek in request 
  request(string rname, int cyl);
  void Main_body(void);
}; // end request
// 
class arrivals: public process {
  erand *arr_period;
  normal *cyl_seek;       // random cylinder to seek 
public: 
  arrivals(string aname, double arr_mean);
  void Main_body(void);
}; // end arrivals
//
// implementations
//
 request::request(string rname, int cyl): process (rname) {
   arrival_time =  get_clock();
   arrived++;
   reqnum = arrived;    // number of the new request 
   req_cyl = cyl;       // request seek cylinder 
   cout << "Creating " << rname << endl;
};
// 
void request::Main_body(void) { 
   // 
   if ( !req_queue->full()) { 
      cout << get_name() << reqnum << " seeking cyl " << req_cyl
           << " at " <<  get_clock() << endl;
	  tracef << get_name() << reqnum << " seeking cyl " << req_cyl
           << " at " <<  get_clock() << endl;
      if ( disk->idle()) 
         disk->reactivate(); 
      req_queue->into(this );
      suspend();   // suspend itself  
   }
   else 
      terminate();     // request  rejected
	  
   cout << get_name() << reqnum << " terminates at " <<  get_clock() << endl;
   tracef << get_name() << reqnum << " terminates at " <<  get_clock() << endl;
   // write the request number and the duration to data file 
   //
   r_wait = (start_d) - (arrival_time);    // wait period 
   acc_wait += r_wait;                     // accumulate wait period 
   service_t = ( get_clock()) - (start_d); // service period 
   acc_serv += service_t;                  // accumulate service time 
   terminate(); 
};   // end request Main_body
//
//
// This process creates the disk requests for I/O 
// For every request, this process generates randomly 
// the cylinder to seek 
// 
arrivals::arrivals(string aname, double arr_mean): process (aname) {
  long mean_cyl;
  long std;
  // Random variable for next request arrival 
  // seed is 21 
  arr_period = new erand (arr_mean, 21);
  // Random variable for cylinder to seek
  mean_cyl = num_cyl / 2;
  std = (long) (num_cyl *.15);
  cyl_seek = new normal (mean_cyl, std, 21);
  cout << "Creating " << get_name() << " with arrival mean: " << arr_mean
       << " cyl mean: " << mean_cyl << endl;
};
//
void arrivals::Main_body(void) { 
  double arr_time;   // time of next request arrival 
  int s_cyl;         // cylinder to seek (random)
  request *nreq;
  while (get_clock() < close_t) { 
     //  
     arr_time = arr_period->fdraw();
     cout << "Delaying arrivals for " << arr_time << endl;
     delay(arr_time); 
     // 
     s_cyl = cyl_seek->draw();      // select a cylinder randomly
     cout << "Selecting cyl: " << s_cyl << " creating request" << endl;
	 tracef << "Selecting cyl: " << s_cyl << " creating request" << endl;
     // create a new request with the selected cylinder 
     nreq = new request("Request", s_cyl);
     nreq->pstart();
  }  // end while
  terminate(); 
};
// 
disk_server::disk_server(string sname): process (sname) {
   current_cyl = 100;    // position head(s) at this cylinder
   direction = DOUT;     // initial direction of disk seek scan
   cout << get_name() << " Initializing" << endl;
};
// 
void disk_server::Main_body(void) { 
  double t_duration;       // duratio of service 
  int cyl_move;            // cylinders to move head(s) 
  int min_move;      // minimum number of cylinders to travel 
  request *min_req;  // request with the minimum move 
  int w_requests;    // number of requets in queue 
  int i;
  bool selected;
  // switch() {
  while (get_clock() < simperiod) { 
    //  
    if ( !req_queue->empty()) { 
       //
       min_move = 1000;
       selected = false;
       w_requests = req_queue->length();  // number of waiting requests 
       cout << "Number of waiting disk requests: " << w_requests << endl;
	   tracef << "Number of waiting disk requests: " << w_requests << endl;
       min_req = NULL;
       if ( w_requests == 1) 
          selected = true;
       while ( selected != true ) { 
         for (i = 1 ; i <= w_requests; i++) { 
            serv_req = ( request*) req_queue->out();
            cyl_move = ((serv_req->req_cyl)) - (current_cyl);
            if ( (cyl_move >= 0) && (direction == DOUT)) { 
               if ( cyl_move < min_move) { 
                  min_move = cyl_move;
                  min_req = serv_req; 
               } 
            }
            else {
               if ( (cyl_move < 0) && (direction == DIN)) { 
                  cyl_move =  - (cyl_move);
                  if ( cyl_move < min_move) { 
                     min_move = cyl_move;
                     min_req = serv_req; 
                  } 
               } 
           }
           req_queue->into(serv_req );    // put back at end of queue  
         } // end for
         if ( min_req == NULL) {   // no requests in current direction 
            if ( direction == DOUT) { 
               direction = DIN;     // reverse direction  
            }
            else {
               direction = DOUT; 
            } 
         }
         else {
            selected = true; 
         } 
       } // while 
       if ( w_requests == 1) {   // there was only 1 request waiting 
         serv_req = ( request*) req_queue->out(); 
       }
       else {
         req_queue->remov(min_req );  // remove min_req disk request 
         serv_req = min_req; 
       }
       // 
       serv_req->start_d =  get_clock(); // start time for service 
       min_move = abs(serv_req->req_cyl - current_cyl);
       cyl_travel += min_move;
       current_cyl = (serv_req->req_cyl);
       // min_move is the minimum head move 
       // Compute the total service duration for this request 
       t_duration = ( rev_time / 2 ) + ( data_size / transfer_rate ) + (min_move * seek_tpc);
       cout << serv_req->get_name() << serv_req->reqnum << " cyl: "
            << serv_req->req_cyl << " starts at " <<  get_clock()
            << " duration: " << t_duration << endl;
	   tracef << serv_req->get_name() << serv_req->reqnum << " cyl: "
            << serv_req->req_cyl << " starts at " <<  get_clock()
            << " duration: " << t_duration << endl;
       delay(t_duration);
       //  CONTINUE_SERV: 
          serv_req->reactivate();  // reactivate the request 
       num_req++;           // number of requests serviced 
    }
    else {
       // queue is empty
       cout << "Disk server goes idle at " <<  get_clock() << endl;
       suspend();
       // 
       cout << "Disk server activated at " << get_clock() << endl; 
    }
  } // end while
  // terminate();
};
//
class dscann : public process {
public:
   dscann(string dsname);
   void Main_body(void);
};
//
dscann::dscann(string sname) : process (sname) {
   cout << "Process " << sname << " created" << endl;
};
//
void dscann::Main_body(void) {
   // cout << "In Main_body of: " << get_name() << endl;
   //
   disk = new disk_server("Disk");
   disk->pstart();
   arr_obj = new arrivals("Arrivals", req_int_arr);

   arr_obj->pstart();
   //
   run->start_sim(simperiod);
   //
   cout << " " << endl;
   cout << "Total number of requests that arrived: "
        << arrived << endl;
   cout << "Total number of serviced requests: "
        << num_req << endl;
   cout << "Total head movement in cylinders: "
        << cyl_travel << endl;
   cout << "Average waiting time: "
        << ( acc_wait / num_req)  << endl;
   cout << "Disk utilization: "
        << ( acc_serv / simperiod)  << endl;
		
   statf << " " << endl;
   statf << "Total number of requests that arrived: "
        << arrived << endl;
   statf << "Total number of serviced requests: "
        << num_req << endl;
   statf << "Total head movement in cylinders: "
        << cyl_travel << endl;
   statf << "Average waiting time: "
        << ( acc_wait / num_req)  << endl;
   statf << "Disk utilization: "
        << ( acc_serv / simperiod)  << endl;
		
   tracef << " ------------------------------------------------------------------" << endl;
   tracef << "End of simulation " << get_name() << endl;		
}
//
int main(void) {
   dscann * model;
   simperiod = 27500.65;      // in msec 
   close_t = 20000.5;         // close arrivals 
   req_int_arr = 450;         // interarrival period (msec) 
   req_queue = new squeue("Cust_queue");

   run = new simulation("Disk SCAN Sched with Normal Dist.");
   //
   // setup files for reporting
   run->set_statfile("scan_stat.txt", statf);
   run->set_tracefile("scan_trace.txt", tracef);   
   
   model = new dscann("Disk SCAN Normal");
   model->pstart();
   //
   run->end_sim();
   // cout << "Ending simulation" << endl;
   return 0;
}
