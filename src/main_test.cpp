#include "Logger.hpp"

#include <iostream>
//#include <string>
#include <thread>
#include <chrono>


//#include <mcheck.h>



#include <signal.h>
#include <unistd.h>



using namespace std;



static int  NUM_TEST_THREADS=4;
static int  NUM_TEST_CYCLES= 10; ;   /** The Num of writing cycles for debugging. */
static int  PERIOD_OF_LOG_MESSAGES=10; /**period at  which Log messages appear, msec. For debugging */
static bool  isINDEFINITE=false;   /** how works cycles in threads: one cycle only or indefinite number of cycles.For debugging*/

static  bool ThreadComplete=false;
std::atomic <int> test_thread_end;  /** counter of all thread and processes running. */


//
//   Some classes used for debugging purposes.
//
//


class  First
{
  public:
      int  i;
      double koef;
      threadsafe_queue<string>* ptrTHRQ;

       First(threadsafe_queue<string>* mptr)  
       { 
          i=0;
          koef=34.78; 
          ptrTHRQ=mptr;
       }


        int  first_test_func(LogLevel Lev, string& first_str)
        {
             int x=0;
             int val=5;
             x=val+val;

             FCFSTR(ptrTHRQ, this, Lev,first_str);
            
             return (val+x);
        }

};/* class First */



class Second 
{
  public:
      int  i;
      double koef;
      threadsafe_queue<string>* ptrTHRQ;

       Second(threadsafe_queue<string>* mptr)  
       { 
          i=0;
          koef=34.78; 
          ptrTHRQ=mptr;
       }


        double second_test_func(LogLevel Lev, string& second_str)
        {
             double x=0.0;
             double val=5.5;
             x=val*val;

             FCFSTR(ptrTHRQ, this, Lev,second_str);
            
             return (val+x);
        }

    
    
};/* class Second */



class Third : public First
{
public:    
       Third(threadsafe_queue<string>* mptr):First(mptr)
       {
           
       }
    
       bool third_test_func(LogLevel Lev, string& third_str)
        {
             double x;
             double val=5.5;
             x=val*val;

             FCFSTR(ptrTHRQ, this, Lev,third_str);
            
             return (x>val);
        }
    
    
};/* class Third */



class Fourth : public Third
{
public:    
       Fourth(threadsafe_queue<string>* mptr):Third(mptr)
       {
           
       }
    
       string&& fourth_test_func(LogLevel Lev, string& fourth_str)
        {
             string x=" done";
             string val="well";
             x=val+x;

             FCFSTR(ptrTHRQ, this, Lev,fourth_str);
            
             return std::move(x);
        }
    
    
};/* class Third */


//
//   END: Some classes used for debugging purposes.
//







void thread_func(threadsafe_queue<string>* Lq, int thr_num,  LogLevel Lev, string pathToLog)
{

   First frst(Lq);
   First* ptrFirstObj=&frst;
   Second scnd(Lq);
   Second* ptrSecondObj=&scnd;
   Third thrd(Lq);
   Third* ptrThirdObj=&thrd;
   Fourth frth(Lq);
   Fourth* ptrFourthObj=&frth;
   
   int rcount=0;   /** To count the number of calling ReConfigure(). */

   //
   //string strFunc = "******* Hello, This is symply a func() ! ****** ";
   //FCFSTR(Lq, nullptr, LogLevel::INFO,strFunc);   
   //
   
 do        {   
   
  for(int i=0; i<NUM_TEST_CYCLES; i++)        {
      
     if(ThreadComplete==true)       {
        cout << "<Got ThreadComplete=true," << "  size_threadsafe_queue = " << Lq->size_threadsafe_queue << endl;   
        test_thread_end--;
        return;
     }
      
     string strThread = "Hello, This is thread " + to_string(thr_num) +  " stroka " + to_string(i);

     ////Lq->PutToLog(  Lev, strThread); 
     
     
     if(thr_num == 1)   {
          ptrFirstObj->first_test_func(Lev,strThread);
     }
     else if(thr_num == 2)   {
          ptrSecondObj->second_test_func(Lev,strThread);
     }
     else if(thr_num == 3)   {
          ptrThirdObj->third_test_func(Lev,strThread);
     }
     else if(thr_num == 4)   {
          ptrFourthObj->fourth_test_func(Lev,strThread);
     }

     //// For thread number #3, check ReConfig() call making if (NUM_TEST_CYCLES>30). 
     //// This done only for Debugging purposes.
     //// Each thread number #3 will run ReConfig(), so the number of records in Log files 
     //// about ReConfigure() calls should be equal 8, see code below.
     if( (thr_num==3) && ( (i==4) || (i==7) || (i==12) || (i==15) || (i==21) || (i==23) || (i==25) || (i==27) )  )        { 
         if ( NUM_TEST_CYCLES > 30 )       { 
             Lq->ReConfigure(pathToLog); 
             cout << "ReConfigure() called " << rcount << " times" << "   pathToLog = " << pathToLog << endl;
            rcount++;
        }
     }/*ref. to  if(thr_num==3) */

     
    std::this_thread::sleep_for(std::chrono::milliseconds(PERIOD_OF_LOG_MESSAGES));
  }/* ref. to for() */
  
  // In common case if number of cycles assigned the thread completes here .
  if(isINDEFINITE==false)    {
     test_thread_end--;
     break;
  }
 
 } while( isINDEFINITE);   /*ref. to while() */
    
};/*end to thread_func() */





void sighandler (int signal_number)
{

  sigset_t mask;
  sigemptyset(&mask);
 
  sigaddset(&mask, SIGINT);
   
  sigprocmask(SIG_BLOCK, &mask, NULL); 
 
  
  if(signal_number==SIGINT)   {
      ThreadComplete=true;
      cout << "<Got signal SIGINT !!!" << endl;   
      
  }
 
  sigprocmask(SIG_UNBLOCK, &mask, NULL);  
      
}/*end sighandler() */









/*
 *    With no any options run all threads and processes with
 *    limited number of cycles, assigned by NUM_TEST_CYCLES. 
 *    Option "-u" runs all threads and processes with
 *    unlimited number of cycles. To stop press CTRL-c. 
 *    
 *    Parameters:  1)  PATH to config file, absolute or relative (option -p).
 *                 2)  The Num of writing cycles (NUM_TEST_CYCLES) for debugging or "undef" if unlimited number of cycles  (option -c).
 *                 3)  period at which Log messages appear, msec. For debugging  (option -d).
 *                 4)  number of threads run concurrently  (option -t).
 * 
 *    Examles of calling:
 *                              ./main_test
 *                              ./main_test -p ./Logger.cfg  -c 10
 *                              ./main_test -p ./Logger.cfg  -c 10  -d 10
 *                              ./main_test -p ./Logger.cfg  -c undef  -d 10
 *                              ./main_test -p ./Logger.cfg  -c undef  -d 10  -t 10
 *                              ./test_all.sh 
 *                              
 */
int   main(int argc, char* argv[])
{
   
  // mtrace();     
   
  string cfgPath;
   
  char c;
  
while((c = getopt(argc, argv, "p:c:d:t:")) != -1) {
    switch(c) {
    case 'p':
        /**PATH to config file, absolute or relative.*/
        cfgPath = string(optarg);
        break;
        
    case 'c':
        /** The Num of writing cycles (NUM_TEST_CYCLES) for debugging or "undef" if unlimited number of cycles.*/
        if( string(optarg) == "undef" )      {
          isINDEFINITE=true;
        }
        else   {
          NUM_TEST_CYCLES=std::atoi(optarg);
        }
        break;
        
    case 'd':
        /** period at which Log messages appear, msec. For debugging.*/
        PERIOD_OF_LOG_MESSAGES=std::atoi(optarg);
        break;
        
    case 't':
        /** number of threads run concurrently.*/
        NUM_TEST_THREADS=std::atoi(optarg);
        break;
        
    }
}   
   
 cout << "Parameters got from command line at starting test program: " << endl;
 cout << "isINDEFINITE = " << isINDEFINITE << endl;
 cout << "PATH to config file is = " << cfgPath << endl;
 cout << "NUM_TEST_CYCLES = " << NUM_TEST_CYCLES << endl;
 cout << "PERIOD_OF_LOG_MESSAGES = " << PERIOD_OF_LOG_MESSAGES << " msec." << endl;
 cout << "NUM_TEST_THREADS = " << NUM_TEST_THREADS << endl;
 
 sigset_t mask; 
 struct sigaction act;    
   
 memset(&act, 0, sizeof(act)); 
 sigemptyset(&mask);
 
 sigaddset(&mask, SIGINT); 
 
 act.sa_handler = sighandler; 
 act.sa_mask = mask; 
 sigaction(SIGINT, &act, NULL); 

   
  
   threadsafe_queue<string> LogQueue(cfgPath); 
   LogQueue.RunLogging();
   
   test_thread_end=0;
   
   //
   //string strMain = "******* Hello, This is main() ! ****** ";
   //threadsafe_queue<string>* ptrQ = &LogQueue;
   //FCFSTR(ptrQ, nullptr, LogLevel::INFO,strMain);
   //
   
   
  //Create some threads and place its futures in a vector. 
  vector<future<void>> vthrs;
  
   
   int  row=0;
   int bf[4]={1,2,3,4};
   for(int i=0; i<NUM_TEST_THREADS;i++)     {
       
       LogLevel Lev; 
       if( bf[row] == 2  )   {
            Lev = LogLevel::INFO;
       }
       else if( bf[row] == 3 )   {
            Lev = LogLevel::WARN;
       }
       else if( bf[row] == 4 )   {
            Lev = LogLevel::ERROR;
       }
       else if (bf[row] == 1)   {
            Lev = LogLevel::DEBUG;
       }       

       auto thr = std::async(std::launch::async,thread_func, &LogQueue, bf[row], Lev, cfgPath);
       test_thread_end++;
       
       vthrs.push_back(move(thr));
       
       row++; 
       if(row>3)     {
           row=0;
       }

   }
 
    
    cout << "\n++++Run " << test_thread_end << " Test Threads At All." << endl << endl; 
  
   return 0;   
}
