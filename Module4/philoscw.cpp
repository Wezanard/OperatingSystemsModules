/*
 A model for the Five Philosophers problem
 Solution: Use of a critical section disallowing the 
 Circular Wait condition.
 The order for requesting the forks is the
 fork with the smallest enumeration value first.
 This model uses the index values in the fork array.

 (c) J. Garrido 2008.
 Last update June 2016.
 
  File: philoscw.cpp 
*/ 
#include "proc.h" 
#include "res.h"                // queue & resource class libraries

using namespace std;

class philosopher;             // Definition of a philosopher process
// 
const int NUM_PHILOS = 5;   // number of philosophers 

philosopher *philos[NUM_PHILOS];    // philosophers processes
//
ofstream statf;      // file for statistics 
ofstream tracef;     // file for trace

// Shared resources
res *forks [NUM_PHILOS];            // shared forks for the philosphers 
//
//
// Random number generators for thinking and eating periods 
// uses an exponential probability distribution
// 
erand *thinking;                     // thinking random number generator 
erand *eating;                       // eating random nr generator 
// 
simulation *run;                     // simulation control object
//
double simperiod;                   // simulation period 

double ph_wait [NUM_PHILOS];        // wait time per philosopher 
unsigned eat_cycles [NUM_PHILOS];  // numebr of eating cycles 
double tot_wait = 0.0;             // total wait time 

double mean_think;                  // mean thinking interval 
double mean_eat;                    // mean eating interval 
//
//  Specifications of philosopher class 
// 
class philosopher: public process {  // inherits Psim3 class process
public: 
  unsigned k;            // number of the philosopher 
  unsigned kk;           // number of philosopher to the right 
  double start_w;        // start waiting period 
  philosopher(string s, unsigned pnum);
  void Main_body();
};
// 
//  Specification of the model class
// 
class philoscw : public process { // inherits Psim3 class process
public:
   philoscw(string sname);
   void Main_body();
};
//
//      implementation of philosopher class 
// 
philosopher::philosopher(string p_name, unsigned pnum): process (p_name) {
       k = pnum;
       cout << get_name()  << " created at " <<  get_clock() << endl;
       // tracef << get_name()  << " created at " <<  get_clock() << endl;
       ph_wait [k] = 0;
       eat_cycles [k] = 0;
};
// 
void philosopher::Main_body() { 
  double think_per;     // thinking period 
  double eat_per;             // eating period 
  while(get_clock() < simperiod) {

            // adjust random number generator
            for (int i=0; i <= k+1; i++)
                think_per = thinking->fdraw();
            //
            kk = k;
            kk++;         // index of right chopstick 
            if ( kk == NUM_PHILOS) 
                 kk = 0;   // around the other end 
            //
            // generate random variable for thinking period 
            think_per = thinking->fdraw();
            cout << get_name()  << " thinking interval = " << think_per << endl;
            tracef << get_name()  << " thinking interval = " << think_per << endl;
            delay(think_per);       // period for thinking activity 
            //
            // Now, attempt to get forks for eating
            //
            start_w =  get_clock();
            cout << get_name() << " attempting CS at: " <<  get_clock() << endl;
            tracef << get_name() << " attempting CS at: " <<  get_clock() << endl;
            delay(0.5);             // time to get forks     
            //
            // Ok, now attempt to acquire both forks
            //
            // check the index values of the forks
            if ( kk < k ) {
                // reverse the fork request order
                // get the right fork first 
                cout << get_name() << " requesting right fork at: " <<  get_clock() << endl;
                tracef << get_name() << " requesting right fork at: " <<  get_clock() << endl;
                forks [kk]->acquire(1);
                cout << get_name() << k << " acquired right fork at " <<  get_clock() << endl;
                //
                // get the left fork
                cout << get_name() << " requesting left fork at: " <<  get_clock() << endl;
                tracef << get_name() << " requesting left fork at: " <<  get_clock() << endl;
                forks [k]->acquire(1);
                cout << get_name() << " acquired left fork at " <<  get_clock() << endl;
                tracef << get_name() << " acquired left fork at " <<  get_clock() << endl;
            }
            //
            else {
                // get left fork first
                cout << get_name() << " requesting left fork at: " <<  get_clock() << endl;
                tracef << get_name() << " requesting left fork at: " <<  get_clock() << endl;
                forks [k]->acquire(1);
                cout << get_name() << " acquired left fork at " <<  get_clock() << endl;
                tracef << get_name() << " acquired left fork at " <<  get_clock() << endl;
                //
                // now get the right fork
                cout << get_name() << " requesting right fork at: " <<  get_clock() << endl;
                tracef << get_name() << " requesting right fork at: " <<  get_clock() << endl;
                forks [kk]->acquire(1);
                cout << get_name() << " acquired right fork at " <<  get_clock() << endl;
                tracef << get_name() << " acquired right fork at " <<  get_clock() << endl;
            }
             //
             // Now ready to start eating
             //
             //
             //
             ph_wait [k] +=  get_clock() - start_w;
             eat_cycles [k]++;
             cout << get_name() << " starts eating at " <<  get_clock() << endl;
             tracef << get_name() << " starts eating at " <<  get_clock() << endl;
             //
             // take some time to eat 
             // generate random variable for eating period 
             eat_per = eating->fdraw();
             cout << get_name() << " eating period = " << eat_per << endl;
             tracef << get_name() << " eating period = " << eat_per << endl;
             delay(eat_per);         // delay for eating activity      
             //
             // release both forks and start thinking again 
             forks [k]->release(1);
             forks [kk]->release(1);
             cout << get_name() << " released forks at: " <<  get_clock() << endl;
             tracef << get_name() << " released forks at: " <<  get_clock() << endl;
        } // end while simulation period
};
// 
//
// Class implmentation of model 
// (for the main thread)
//
philoscw::philoscw (string s) : process(s) {
   cout << s << " created" << endl;
};
void philoscw::Main_body() {
   string name_fork;
   string num;
   string name_phil;
   int j;
   //
   // Create the objects 
   for (j = 0; j < NUM_PHILOS; j++) { 
      name_fork.append("fork");
      num = intstr(j);
      name_fork.append(num);
      // forks: array of standard mutual exclusive resources  
      forks [j] = new res (name_fork, 1); // create forks
      
      // create each of the the five philosophers
      name_phil.append("Philosopher");
      name_phil.append(num);
      philos[j] = new philosopher(name_phil, j);
      name_phil.clear();
      name_fork.clear();
      cout << endl;
      // start each philosopher
      philos[j]->pstart();
   }
   // now run simulation for the simulation period specified 
   run->start_sim(simperiod);    // start simulation
   //
   tracef << " ------------------------------------------------------------------" << endl;
   tracef << "End of simulation " << get_name() << endl;
   //
   for (j = 0; j < NUM_PHILOS; j++) {
        cout << "Number of eating cycles phil " << j << " = " 
             << eat_cycles [j] << endl;
        cout << "Total waiting interval to eat phil " << j << " = " 
             << ph_wait [j] << endl;
        statf << "Number of eating cycles phil " << j << " = " 
             << eat_cycles [j] << endl;
        statf << "Total waiting interval to eat phil " << j << " = " 
             << ph_wait [j] << endl;
        tot_wait += ph_wait [j]; 
   }
   cout << "Average interval waiting to eat: " 
        << tot_wait / ((double) NUM_PHILOS) << endl;
   statf << "Average interval waiting to eat: " 
        << tot_wait / ((double) NUM_PHILOS) << endl;

}
// 
//
int main() {
    philoscw *ph_model; 
    simperiod = 250.0;
    mean_eat = 9.5;        // mean eating period  
    mean_think = 15.45;    // mean thinking period  
//  //
    run = new simulation("Concurrent Philosophers Problem Circular Wait");

    // setup files for reporting
    run->set_statfile("philoscw_stat.txt", statf);
    run->set_tracefile("philoscw_trace.txt", tracef);
    
    eating = new erand (mean_eat);
    thinking = new erand (mean_think);
//
    /*
    double think_p;
    think_p = thinking->fdraw();
    cout << "First think draw: " << think_p << endl;
    */
    //
    tracef <<  "Simulation of Concurrent Philosophers - Circular Wait" << endl;
    tracef << "-----------------------------------------------------------" << endl;

    // Main object 
    ph_model = new philoscw("Concurrent Philosophers Circular Wait");
    ph_model->pstart();
    //
// 
    run->end_sim();        // end simulation
    // cout << "Concurrent philosophers problem Circular Wait" << endl;

    return 0;
}

