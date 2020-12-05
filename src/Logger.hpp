//
//    Logger.hpp
//

#ifndef    LOGGER_HPP
#define    LOGGER_HPP


#include <iostream>
#include <memory>
#include <mutex>

#include <string>
#include <thread>
#include <chrono>
#include <atomic>

#include <fstream>

#include <iomanip>  /* for put_time()  */
#include <ctime>

#include <sstream> /** for stringstream */

#include <boost/format.hpp> /** for format() */
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>

#include <future> /**for async() */


#include <cxxabi.h>  /**for demangle() */

#include <map>

#include <regex>

#include <condition_variable>

//#include  <mcheck.h>






using namespace std;
namespace filesys = boost::filesystem;





// Log levels
enum class  LogLevel : int   {  
      DEBUG=1, INFO=2, WARN=3, ERROR=4 
};


struct  _some_vars
{
    bool SignalFlag;           /* It is time to comlete log_thread */
    std::atomic <int> ThrEnd;  /* Have log_thread been completed yet */
    int ThrCycles;             /** number of cycles to wait of thread completing. */
    int thrWaitTime;           /** msec, period of time for one cycle to wait of thread completing. */
    ofstream fout;             /** Descriptor of Log file.   */
    
    int reg_size;              /** the size of one Log file*/
    int num_regs;              /** max. number of Log files, after exceeding that one of existing ones will be rewritten.*/
    int curr_reg_file;         /** the number of current Log file in which do writing. Counting from 0.*/
    string CurrLogFile; /** the name of current  Log file in which do writing. */

   /** a map with parameters: key - parameter's name, value - its value */
    std::map <std::string, std::string>  cmdStr;
    
    string LogDir;             /** Directoty to put Logs. */
    string barrier;            /** barrier level of logging */
    int level_barrier;    /** barrier level of logging as type int */
    string unit_of_measure;     /** Assign  unit of measure for reg_size - the size of one Log file*/ 
                                /** BYTES - in bytes, KBYTES - in Kb, MBYTES - in Mb */
                                
    string is_stdout;     /** Do Print Logs to console stdout. YES - print, NO - do not print. */    
    string pattern;       /** Pattern to filter records in Log files. Pattern must satisfy regex rules. */ 
    string confPath;      /** path to config . file - absolute or relative. */
    int overstocking;     /** The limit of overstocking for queue. This is max. number of elements in the queue. */
    
};



#define      FCFSTR(pLogLib, pUserObj,Lev, msg)                pLogLib->callFileName=__FILE__;\
                                         pLogLib->callClassName=typeid(pUserObj).name();\
                                         pLogLib->callFuncName=__FUNCTION__;\
                                         pLogLib->PutToLog(pLogLib->callFileName,pLogLib->callClassName,pLogLib->callFuncName,Lev,msg);\



template<typename T>
class threadsafe_queue
{
   private:
   struct node
   {
       std::shared_ptr<T> data;
       std::unique_ptr<node> next;
   };
   std::mutex head_mutex;
   std::unique_ptr<node> head;
   std::mutex tail_mutex;
   node* tail;
   std::condition_variable queueCondVar;
   std::future<void> log_thread;
   
   node* get_tail();

   std::unique_ptr<node> pop_head();
   
   /*
    * Clean all the threadsafe_queue.
    * Wants mutex or other mutually exclusiive means.
    * 
    */
   void clean_queue(void);
   
   

 public:
     
   std::atomic <int> size_threadsafe_queue;      /** the size of  the threadsafe_queue.*/
   
     /*
      *  Return false if this threadsafe_queue is not empty.
      *  Otherwise, if empty returns true.
      */
    bool isEmpty();
    
   
   std::string demangle(char const* mangled);
     
string   callFileName;  /** file from what call done. */    
string   callClassName;  /** class from what call done. */  
string   callFuncName;  /** function from what call done. */     
     

//Constructor. Parameter is path to config . file - absolute or relative. */     
   threadsafe_queue(string cpath);
  
//Destructor  
   ~threadsafe_queue();

   threadsafe_queue(const threadsafe_queue& other)=delete;
 
   threadsafe_queue& operator=(const threadsafe_queue& other)=delete;
   
   std::shared_ptr<T> try_pop();

   void push(T new_value);
   
   
//The other funcs.
   
   /*
    *   Print all messages from the list on stdout. 
    */
   
   void go_around_list(void);   
   
   /*
    *   Add a user's message to the list.
    *   Parameters: 
    *             fl - file name
    *             cla - class name
    *             func - function name 
    *             LogLeve -  level 
    *             message -  user's message
    */   
   void  PutToLog( string fl, string cla, string func, LogLevel Lev, string& message);
   

   
  /* Look the size of current Log file. If it greater then wanted close it and give the next one.
   * If all existing Log files are full, take the first one.
   * some_vars.CurrLogFile
   * return:
   *           0 - success
   *          -1 - failure
   */
   int SwitchCurrLogFile(void);
   
   
   
    /*  
     *     Read config. file for example "Logger.cfg" and form  std::map,
     *     If Logger does not find its config file, it uses default settings.
     *     in which key field is one of configuration parameters and value field
     *     is its value. Considered, that each configuration parameter has a unique name.
     */
    void ReadConfig(std::map <std::string, std::string> & cmdToInsert);
   
   /*

    * Check if given string path is of a Directory

    */

   bool checkIfDirectory(std::string filePath);
   
    
   /*
    *  Create directory tree recursively.
    *  parameters:
    *                   s -    end path
    *                   mode - attributes
    */
   int mkpath(std::string s,mode_t mode);

   
   
    /*
     *  This function configures settings taken them from
     *  configuration file at start up.
     *  Parameters:
     *    cpath - path to config.file, full or relative.
     */
 
    int Configure(string cpath);   
    
 
    /*
     *  This function reconfigures settings taken them from
     *  configuration file.
     *  Parameters:
     *    cpath - path to config.file, full or relative.
     */
 
    int ReConfigure(string cpath);   
    
    
  /*
   *   Log Thread function to do reading from the List
   *   and write it in Log file.
   */   
   void  LogThreadFunc(void);



  /*
   *  Start Log Thread function
   */  
   void RunLogging(void);
   
   
   
}; /* class threadsafe_queue  */


   
/** Fictitious global func. Otherwise, the library not linked. */   
   extern void TemporaryFunction(void);
   


#endif   //#ifndef    LOGGER_HPP
