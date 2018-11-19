/*
 A model for the Five Philosophers problem 
 Solution: Deadlock prevention by disallowing
 the hold and wait condition. 
 A philosopher first checks, in a critical section, if the two forks 
 are available. If they are, the philosopher requests the two forks.
 Otherwise, the philosopher waits (is suspended). 
 (C) J. Garrido, 2008. Updated May 2016.
 
  File: philoshw.cpp 
*/ 
#include "proc.h" 
// #include "res.h"              // resource class 
#include "semaphore.h"        // include semaphore classes
#include "queue.h"


   using namespace std;
//
   class philosopher;         // Class of philosopher process
   class philoshw;            // main class of model
   
   //
   //
   ofstream statf;      // file for statistics 
   ofstream tracef;     // file for trace
//
   const int NUM_PHILOS = 5;  // number of philosophers 

// Semaphores (used for exclusive access to forks) 
   Bsemaphore *mutex;         // semaphore for mutual exclusion 
   // forks are mutual exclusive resources
   res *forks [NUM_PHILOS];   // fork semaphores 
//
   philosopher *philos[NUM_PHILOS];   // the philosopher processes
   
   philoshw *ph_model;                // simulation model object
    //
   simulation *run;                   // Psim3 simulation control object   

// simple queue for waiting philosopher 
   squeue *entry_queue;       // for waiting philosopher 
//
   // Random number generators for think and eat periods 
   // 
   erand *thinking;           // think random nr generator 
   erand *eating;             // eat random nr generator 
   

   double simperiod;          // simulation period    
   
   double ph_wait [NUM_PHILOS];      // wait time per philosopher 
   unsigned eat_cycles [NUM_PHILOS]; // number of eat cycles 
   double mean_th[NUM_PHILOS];       // mean think period
   double mean_e[NUM_PHILOS];        // mean eat period
   double tot_wait = 0.0;            // total wait time 
//
//
   double mean_think;        // mean think interval 
   double mean_eat;          // mean eat interval 
//

//
//  Specifications of model class 
//
//
class philoshw : public process { // inherit class process
   public:
      philoshw(string pname);
      void Main_body();     
 };
//
//  Class specification of philosopher class
//
class philosopher: public process { 
      unsigned k;        // number of the philosopher 
      unsigned kk;       // number of philosopher to the right 
      double start_w;    // start waiting period 
      normal *thinking;  // random numb generator
      normal *eating;
      double mean_think;
      double mean_eat;
   public: 
      philosopher(string s, unsigned pnum, double m_think, double m_eat);
      void Main_body();
};
// 
//
// class implementations
//
// implementation of philosopher class 
// 
philosopher::philosopher(string p_name, unsigned pnum, double mthink, 
      double meat): process (p_name) {
      k = pnum;
      ph_wait [k] = 0.0;
      eat_cycles [k] = 0;
      mean_think = mthink;
      mean_eat = meat;
      kk = k;
      kk++;
      // number of philosopher to the right   
      kk = kk % NUM_PHILOS;     // modulus
      cout << get_name() << k << " created with " << mean_think << " "
           << mean_eat  << endl;
};
    // 
void philosopher::Main_body() { 
      double think_per;        // think period 
      double eat_per;          // eat period
      double stdt;             // standard dev for think per
      double stde;             // std for eat per
      philosopher *temp_phil;
      unsigned left_chop;      // left fork
      unsigned right_chop;     // right fork (only need one of each) 
      //
      // adjust behavior of random number generation
      // mainly for the MinGW C++ compiler
      //
      stdt = mean_think * 0.15;   // using 0.15 as coeff of variance
      thinking = new normal (mean_think, stdt);
      stde = mean_eat * 0.15;
      eating = new normal(mean_eat, stde);
      /*
      for (int i = 0; i < 30; i++) {
          think_per = thinking->fdraw();
          cout << get_name() << k << " think per: " << think_per << endl;
      }
      */
      /*
      for (int j = 0; j < k+1; j++) {

          // generate random variable for think period 
          think_per = thinking->fdraw();
      }
      */
      while(get_clock() < simperiod) {
         think_per = thinking->fdraw();
         cout << get_name() << k << " think period = " << think_per << endl;
         //
         //
         delay(think_per);       // period for think activity 
         //
         // Now, attempt to get forks for eating
         //
         start_w =  get_clock();
         cout << get_name() << k << " attempting access to CS at: " 
              <<  get_clock() << endl;
         mutex->wait();         // only one philosopher 
         //
         // will check availability of forks
         // in a mutual exclusive manner
         // this is a critical section
         //
         left_chop = forks [k]->num_avail();
         right_chop = forks [kk]->num_avail();
         // Are both forks (left and right) available? 
         while ( (left_chop == 0) || (right_chop == 0)) { 
            mutex->signal();               
            cout << get_name() << k << " does NOT find both forks available at: " << get_clock() 
                 << endl;
            tracef << get_name() << k << " does NOT find both forks available at: " << get_clock()  
                 << endl;    
            // need to wait because both forks are not available             
            entry_queue->into(this);
            // cout << get_name() << k << " suspending self (waiting) at " 
            //      <<  get_clock() << endl;
            suspend();              // wait for left and right forks 
            //
            //      Try again        
            mutex->wait();      // only one philosopher
            left_chop = forks [k]->num_avail(); 
            right_chop = forks [kk]->num_avail();
         } // end while
         //
         // Ok, now acquire both forks
         //
         // attempt to get left fork
         cout << get_name() << k << " requesting left fork at: " 
              <<  get_clock() << endl;
         tracef << get_name() << k << " requesting left fork at: " 
              <<  get_clock() << endl;
         forks [k]->acquire(1);
         cout << get_name() << k << " acquired left fork at " 
              <<  get_clock() << endl;
         tracef << get_name() << k << " acquired left fork at " 
              <<  get_clock() << endl;
         //
         // Attempt to get right fork
         cout << get_name() << k << " requesting right fork at: " 
              <<  get_clock() << endl;
         tracef << get_name() << k << " requesting right fork at: " 
              <<  get_clock() << endl;
         forks [kk]->acquire(1);
         cout << get_name() << k << " acquired right fork at " 
              <<  get_clock() << endl;
         tracef << get_name() << k << " acquired right fork at " 
              <<  get_clock() << endl;
         //
         // release mutual exclusion for other philosophers to  
         // attempt acquiring both forks 
         mutex->signal();
         // exit critical section 
         // 
         // Now ready to start eating
         //     
         delay(0.0);  // reschedule
         //
         // start to eat
         ph_wait [k] += get_clock() - start_w;
         eat_cycles [k]++;
         cout << get_name() << k << " starts eating at " <<  get_clock() << endl;
         tracef << get_name() << k << " starts eating at " <<  get_clock() << endl;
         // take some time to eat 
         // generate random variable for eat period 
         eat_per = eating->fdraw();
         cout << get_name() << k << " eat period = " << eat_per << endl;
         // 
         delay(eat_per);         // time interval for eating activity      
         //
         // release both forks and start thinking again 
         forks [k]->release(1);
         forks [kk]->release(1);
         cout << get_name() << k << " released forks at: " <<  get_clock() << endl;
         tracef << get_name() << k << " released forks at: " <<  get_clock() << endl;
         //
         // Any philosopher waiting for forks?
         //   
         while ( !entry_queue->empty()) { 
            temp_phil = ( philosopher*) entry_queue->out();
            temp_phil->reactivate();
         }
         
      } // end while simulation period
};
// 
//   Implementation of model class
//
philoshw::philoshw (string s) : process(s) {
      cout << s << " created" << endl;      
};
//
void philoshw::Main_body() { 

      string name_chop;
      string chop_num;
      string phil_name;
      int j;
      // Create Philosopher processes (active objects)
      for ( j = 0; j < NUM_PHILOS; j++) {
         chop_num = intstr(j); // int to string
         phil_name.append("Philosopher" + chop_num);
         philos[j] = new philosopher(phil_name, j, mean_th[j], mean_e[j]);
         philos[j]->pstart();
         phil_name.clear();
      }
      // now run simulation for the simulation period specified 
      run->start_sim(simperiod);
      //
      for ( j = 0; j < NUM_PHILOS; j++) {
         cout << "Number of eat cycles " << philos[j]->get_name() << j << " = " 
             << eat_cycles [j] << endl;
         statf << "Number of eat cycles " << philos[j]->get_name() << j << " = " 
             << eat_cycles [j] << endl;      
         cout << "Total waiting time to eat " << philos[j]->get_name() << j << " = " 
             << ph_wait [j] << endl;
         statf << "Total waiting time to eat " << philos[j]->get_name() << j << " = " 
             << ph_wait [j] << endl;             
         tot_wait += ph_wait [j]; 
      }
      cout << "Average time waiting to eat: " 
         << tot_wait / ((double) NUM_PHILOS) << endl;
      statf << "Average time waiting to eat: " 
         << tot_wait / ((double) NUM_PHILOS) << endl;
         
      tracef << "End of simulation run" << endl;
      return;
}
//
int main() {
      //
      simperiod = 250.0;      // simulation period
      mean_e[0] = 9.5;        // mean eat period  
      mean_th[0] = 15.45;     // mean think period  
      mean_e[1] = 19.5;       // mean eat period  
      mean_th[1] = 25.45;     // mean think period
      mean_e[2] = 35.5;       // mean eat period  
      mean_th[2] = 45.45;     // mean think period
      mean_e[3] = 5.5;        // mean eat period  
      mean_th[3] = 20.45;     // mean think period
      mean_e[4] = 12.5;       // mean eat period  
      mean_th[4] = 7.45;      // mean think period  
      //
      string name_chop;
      string chop_num;
      //  
      //
      run = new simulation("Concurrent Philosophers - Hold & Wait");
      
      // setup files for reporting
      run->set_statfile("philoshw_stat.txt", statf);
      run->set_tracefile("philoshw_trace.txt", tracef);
      
      entry_queue = new squeue ("Entry Queue");
      
      for (int j = 0; j < NUM_PHILOS; j++) { 
         name_chop.append("fork");
         chop_num = intstr(j);
         name_chop.append(chop_num);
         // one semaphore for each fork
         cout << "creating fork " << j << endl;
         forks [j] = new res (name_chop, 1); // create forks
         name_chop.clear();
      }
      //
      //
      // Create objects for the semaphores 
      mutex =   new Bsemaphore ("Mutex", 1);        // for mutual exclusion
      //
      // Main object
      ph_model = new philoshw("Concurrent Philosophers");
      ph_model->pstart();  // start life of main object
      //
      // 
      run->end_sim();      // end of simulation
      cout << "Concurrent Philosophers - Hold & Wait" << endl;
      return 0;
}  // end main
