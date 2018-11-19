// Psim3 disk device model. 
// Disk scheduling using Shortest-Seek-Time-First (SSTF) discipline with  
// moving-head disk. 
// Arriving requests follow an exponential distribution for 
// inter-arrival periods. 
// Every disk request needs a data transfer (read/write) of a block of 
// data of a given size (data_size). 
// The cylinder to seek is chosen from a normal distribution 
// given the number of cylinders in the disk device. 
// A disk service includes the rotational delay, seek delay, and 
// transfer time. 
// A different disk device would have different physical 
// characteristics. 
// 
// J. M. Garrido Rev. Feb. 2005. Last update June 2016.
// File: dssstfn.cpp 
// 
#include "proc.h" 
#include "queue.h"   // additional library classes
//
using namespace std;

class disk_server;
class request;
class arrivals;      // waiting queue for requests 
//
// physical characteristics of the disk device 
const int data_size = 4;        // data block to read/write, Kbytes 
const double rev_time = 4;      // revolution time (msec.) 
const float transfer_rate = 3;  // in Mb/sec (Kb/msec) 
const float seek_tpc = 15.6;    // seek time per cylinder (msec.) 
const long num_cyl = 300;       // number of cylinders in disk 
// 
squeue *req_queue;              // wait queue for requests 

ofstream statf;      // file for statistics 
ofstream tracef;     // file for trace

double simperiod;               // simulation period 
double close_t;                 // close time for arrivals 
// 
double req_int_arr;             // mean request inter-arrival period 
double acc_serv = 0;            // accumulated service time 
double acc_wait = 0;            // accumulated wait time 
unsigned num_req = 0;           // number of requests serviced 
unsigned arrived = 0;           // number of requests that have arrived 
unsigned cyl_travel = 0;        // total arm movement in cylinders 
// 
disk_server *disk;
arrivals *arr_obj;
simulation *run;
// 
class disk_server: public process {
  request *serv_req;        // request being serviced 
  double sstart;            // time of start of service 
  int current_cyl;          // current cylinder heads are located 
  int min_move;             // minimum number of cylinders to travel 
public: 
  disk_server(string sname);
  void Main_body();
};
// 
class request: public process {
  double r_wait;            // request wait period 
public: 
  double sstart;            // service start time 
  unsigned reqnum;          // request number 
  double arrival_time;
  double service_t;         // service period for the request 
  int req_cyl;              // cylinder to seek in request 
  request(string rname, int cyl);
  void Main_body();
};
// 
class arrivals: public process {
  erand *arr_period;
  normal *cyl_seek;      // random cylinder to seek 
public: 
  arrivals(string aname, double mean_arr);
  void Main_body();
};
// 
request::request(string rname, int cyl): process (rname) {
  arrival_time =  get_clock();
  arrived++;         // number of requests that arrived 
  reqnum = arrived;  // number of the new request 
  req_cyl = cyl;     // request seek cylinder 
  // cout << "Creating " << rname << " cyl: " << cyl << endl;
};
// 
void request::Main_body() { 
  // 
  if ( !req_queue->full()) { 
     cout << get_name() << reqnum << " seeking cyl "
          << req_cyl << " arrives at " <<  get_clock() << endl;
	 tracef << get_name() << reqnum << " seeking cyl "
          << req_cyl << " arrives at " <<  get_clock() << endl;
     // wait for requests to accumulate 
     if ( disk->idle()) { 
       cout << "Activating server" << endl; 
       disk->reactivate(); 
     }
     req_queue->into(this );      // join queue
     suspend();  // suspend itself  
  }
  else 
     terminate();
  //
  // 
  cout << get_name() << reqnum << " terminates at " <<  get_clock() << endl;
  tracef << get_name() << reqnum << " terminates at " <<  get_clock() << endl;
  r_wait = sstart - arrival_time;   // wait period 
  acc_wait += r_wait;               // accumulate wait period 
  service_t = get_clock() - sstart; // service period 
  acc_serv += service_t;
  // write the request number and the duration to data file 
 
  terminate();
};  // end request main_body

// 
// This process creates the disk requests for I/O 
// For every request, this process generates randomly 
// the cylinder to seek 
// 
arrivals::arrivals(string aname, double mean_arr): process (aname) {
  long mean_cyl;       // mean cylinder 
  long stdev;            // standard deviation for cyl 
  // Random variable for next request arrival 
  // seed is 21 
  arr_period = new erand (mean_arr, 21); // Random variable for cylinder to seek 
  mean_cyl = num_cyl / 2;
  stdev = (long) (num_cyl * 0.15);  // std could be given
  cyl_seek = new normal (mean_cyl, stdev, 21);
};
void arrivals::Main_body() {
  request *nreqobj;
  double arr_time;         // time of next request arrival 
  int s_cyl;               // cylinder to seek (random) 
  while(get_clock() < close_t) {
     //  
     arr_time = arr_period->fdraw();
     delay(arr_time);             
     //
     s_cyl = cyl_seek->draw();    // select a cylinder randomly 
     // create a new request with the selected cylinder 
     nreqobj = new request("Request", s_cyl);
     nreqobj->pstart();
  }
  terminate();
};
// 
 disk_server::disk_server(string sname): process (sname) {
    current_cyl = 100;    // initial position of head(s)
    cout << "Creating: " << sname << endl;
};
// 
void disk_server::Main_body() { 
   double t_duration;     // duration of service 
   int cyl_move;          // cylinders to move head(s) 
   request *min_req;      // request with the minimum move 
   int w_requests;        // number of requets in queue 
   int i;
   while(get_clock() < simperiod) {
      // 
      min_move = 25000;    // initially, any high value 
      if ( !req_queue->empty()) { 
         // 
         // get number of waiting requests in request queue 
         w_requests = req_queue->length();
         // Search sequentially for the next request 
         // with the shortest seek time 
         min_req = 0;
         for (i = 1 ; i <= w_requests; i++) { 
            serv_req = ( request*) req_queue->out();
            cyl_move = abs(((serv_req->req_cyl)) - (current_cyl));
            if ( cyl_move < min_move) { 
               min_move = cyl_move;   // number of cyls to move 
               min_req = serv_req;    // req with shortest move yet  
            }
            cout << "Searching: " <<  serv_req->get_name() << serv_req->reqnum  
                 << " cyl: " << serv_req->req_cyl << endl; 
		    tracef << "Searching: " <<  serv_req->get_name() << serv_req->reqnum  
                 << " cyl: " << serv_req->req_cyl << endl; 
            req_queue->into(serv_req );  // put back at end of queue  
         } //end for
         req_queue->remov(min_req );
         serv_req = min_req;
         current_cyl = (serv_req->req_cyl);
         cout << serv_req->get_name() << " " <<  serv_req->reqnum << " cyl: "
              << serv_req->req_cyl << " move: " << min_move << endl; 
	     tracef << serv_req->get_name() << " " <<  serv_req->reqnum << " cyl: "
              << serv_req->req_cyl << " move: " << min_move << endl; 
         // 
         serv_req->sstart = get_clock();  // start time for service 
         // min_move is the minimum head move 
         cyl_travel += min_move;          // accumm cyl move 
         // Compute the total service period for this request 
         t_duration = rev_time / 2 + (data_size / transfer_rate) + (min_move * seek_tpc);
         cout << serv_req->get_name() << serv_req->reqnum << " starts at "
              <<  get_clock() << " service: " << t_duration
              << ", cyl move: " << min_move << endl;
		 tracef << serv_req->get_name() << serv_req->reqnum << " starts at "
              <<  get_clock() << " service: " << t_duration
              << ", cyl move: " << min_move << endl;
         delay(t_duration);
         // 
         serv_req->reactivate();    // reactivate the request 
         num_req++;                    // number of requests serviced 
         // reschedule();
      }
      else {
         // queue is empty
         cout << "Disk server goes idle at " << get_clock() << endl; 
         suspend();
         // 
         cout << "Disk server reactivated at " << get_clock() << endl; 
      }
   } // end while
   terminate();
};
//
class dsstf : public process {
public:
    dsstf(string dsname);
	void Main_body();
};
//
dsstf::dsstf(string sname) : process (sname) {

  cout << "Process " << sname << " created" << endl;
};
//
void dsstf::Main_body() {
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
   //
   tracef << " ------------------------------------------------------------------" << endl;
   tracef << "End of simulation " << get_name() << endl;	
};
//
int main() {
   dsstf * model;
   simperiod = 27500.65;       // simulation periodin msec 
   close_t = 20000.5;          // time to close arrivals 
   req_int_arr = 450;          // mean interarrival period (msec) 
   req_queue = new squeue("Cust_queue");
   //
   run = new simulation("Disk SSTF Sched (Normal)");
   // setup files for reporting
   run->set_statfile("SSTF_stat.txt", statf);
   run->set_tracefile("SSTF_trace.txt", tracef);      
   //
   model = new dsstf("Disk SSTF Normal");
   model->pstart();
   // 
   run->end_sim();
   // cout << "Ending simulation" << endl;
   return 0;
}
