/*

 A model for the Five Philosophers synchronization/deadlock problem
 This model illustrates the occurrence of deadlock.
 The philosophers sit at the same same time and attempt
 to acquire the left and right forks as passive resources
 
 (C) J. Garrido, 2008 for Psim3.
 Last update May 2016.

  File: philos.cpp 
*/ 
#include "proc.h" 
#include "res.h"              // resource class library 

   using namespace std;
//
ofstream statf;      // file for statistics 
ofstream tracef;     // file for trace
   
   class philosopher;
   
   const int NUM_PHILOS = 5; // number of philosophers   
// 
// shared (passive) resources 
   res *forks [NUM_PHILOS];
   
// the philosopher processes (active objects)
   philosopher *philos[NUM_PHILOS];
   
   simulation *run;                 // Psim3 simulation object
// 
// Random number for thinking and eating periods 
   urand *thinking;
   urand *eating;
// 

   double simperiod;   // simulation period 

   double think_l;     // low limit for thinking period
   double think_h;     // high limit 
   double eat_l;       // low limit for eating
   double eat_h;       // high limit
//  
//  Class Specifications of philosopher  
// 
   class philosopher: public process {
      unsigned k;     // index of left fork 
      unsigned kk;    // index of right fork 
    public: 
      philosopher(string s, unsigned pnum);
      void Main_body();
   };
// 
//   Class specification of model class
//
    class phil_model : public process { // inherit class process
    public:
      phil_model(string mname);
      void Main_body();
    };
// 
//      Class implementation of philosopher class 
// 
   philosopher::philosopher(string s, unsigned pnum): process (s) {
      k = pnum; // philosopher number
      cout << get_name()  << " created at " <<  get_clock() << endl;
      // tracef << get_name()  << " created at " <<  get_clock() << endl;
   };
// 
    void philosopher::Main_body() { 
      double think_per;
      double eat_per;
      while(get_clock() < simperiod) {
      
         // The philosophers start eating from the first moment  
         cout << get_name()  << " requesting left fork at: "
              <<  get_clock() << endl;
         tracef << get_name()  << " requesting left fork at: "
              <<  get_clock() << endl;
         forks [k]->acquire(1);
         cout << get_name()  << " acquired left fork at "
            <<  get_clock() << endl;
         tracef << get_name()  << " acquired left fork at "
            <<  get_clock() << endl;
         kk = k;
         kk++;
         if ( kk == NUM_PHILOS) 
            kk = 0;   // around the other end 
         delay(0.0);    // reschedule
      
         //   
         cout << get_name()  << " requesting right fork at: "
            <<  get_clock() << endl;
	     tracef << get_name()  << " requesting right fork at: "
            <<  get_clock() << endl;
         forks [kk]->acquire(1); 
         cout << get_name()  << " acquired right fork at "
            <<  get_clock() << endl;
         tracef << get_name()  << " acquired right fork at "
            <<  get_clock() << endl;
         delay(0.0);
         //  START_EATING
         // take some time to eat 
         eat_per = eating->fdraw();
         delay(2.0);      // delay for eating activity 
         //
         //  RELEASE_FORKS
         // release both forks
         forks [k]->release(1);
         forks [kk]->release(1);
         cout << get_name()  << " releasing forks at: "
            <<  get_clock() << endl;
         tracef << get_name()  << " releasing forks at: "
            <<  get_clock() << endl;
         // Take some finite time to think
         think_per = thinking->fdraw();    // think period
         delay(think_per);                // thinking activity
      }  // end while
   };
//
//
//    Class implementation of model class
	 //
    phil_model::phil_model (string s) : process(s) {
        cout << s << " created" << endl;
    };
	 //
    void phil_model::Main_body() {
      string name_chop;
      string chop_num;
      string phil_name;
   
      //
      // Create objects 
      for (int j = 0; j <NUM_PHILOS; j++) { 
         name_chop.append("fork");
         chop_num = intstr(j); // int to string
         name_chop.append(chop_num);
         forks [j] = new res (name_chop, 1); // create forks
         name_chop.clear();
         //
         phil_name.append("Philosopher" + chop_num);
         philos[j] = new philosopher(phil_name, j);
         philos[j]->pstart();
         phil_name.clear();
      }
      run->start_sim(simperiod);
      tracef << " ------------------------------------------------------------------" << endl;
      tracef << "End of simulation " << get_name() << endl;
   }
//
//
    int main() {
  
      phil_model *ph_object;
   
      unsigned j;
      simperiod = 150.65;
      eat_l = 1.0;
      eat_h = 4.5;
      think_l = 8.45;
      think_h = 15.75;
      run = new simulation("Concurrent philosophers problem");
	  
	  // setup files for reporting
      run->set_statfile("philos_stat.txt", statf);
      run->set_tracefile("philos_trace.txt", tracef);
	  
      tracef <<  "Simulation of Concurrent Philosophers" << endl;
      tracef << "-----------------------------------------------------------" << endl;
      eating =  new urand (eat_l, eat_h);
      thinking =  new urand (think_l, think_h);
      //  
      // Create object of model class 
      ph_object = new phil_model("Concurrent Philosophers");
      ph_object->pstart();  // start life of active object
      //
      run->end_sim();       // end simulation
      return 0;
      //
   }
