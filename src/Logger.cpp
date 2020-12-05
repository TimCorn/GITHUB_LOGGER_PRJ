//thread safe queue with microgranular blocking

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
    string CurrLogFile;        /** the name of current  Log file in which do writing. */

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
    
}some_vars;



#define      FCFSTR(pLogLib, pUserObj, Lev, msg)                pLogLib->callFileName=__FILE__;\
                                         pLogLib->callClassName=typeid(pUserObj).name();\
                                         pLogLib->callFuncName=__FUNCTION__;\
                                         pLogLib->PutToLog(pLogLib->callFileName,pLogLib->callClassName,pLogLib->callFuncName,Lev, msg);\





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
   
   node* get_tail()
   {
     std::lock_guard<std::mutex> tail_lock(tail_mutex);
     return tail;
   };

   std::unique_ptr<node> pop_head()
   {
     std::unique_ptr<node> old_head;  
    { 
       std::lock_guard<std::mutex> head_lock(head_mutex);
     
       if(head.get()==get_tail())
       {
         return nullptr;
       }
       old_head=std::move(head);
       head=std::move(old_head->next);
    }/*called ~std::lock_guard() */
       
      size_threadsafe_queue--;
      return old_head;
     
   };/*end of pop_head() */
   

   /*
    * Clean all the threadsafe_queue.
    * Wants mutex or other mutually exclusiive means.
    * 
    */
   void clean_queue(void)
   {
      std::shared_ptr<string> retObj;
      
          while(true)       {
              
             retObj=std::move(try_pop());
             if(retObj!= nullptr)      {   
                continue; 
             }
             else    {  /** removed the last element. */
                break;
             }   
             
          }/* while(true) */
       
   }/*end of clean_queue() */

   
   
 public:
     
    std::atomic <int> size_threadsafe_queue;      /** the size of  the threadsafe_queue.*/
     
     /*
      *  Return false if this threadsafe_queue is not empty.
      *  Otherwise, if empty returns true.
      */
    bool isEmpty()      
    {
        
       return  !(size_threadsafe_queue > 0);
    };
     

std::string demangle(char const* mangled) {

    auto ptr = std::unique_ptr<char, decltype(& std::free)>{

        abi::__cxa_demangle(mangled, nullptr, nullptr, nullptr),

        std::free

    };

    return {ptr.get()};

}       
       
     
string   callFileName;  /** file from what call done. */   
string   callClassName;  /** class from what call done. */  
string   callFuncName;  /** function from what call done. */     
     
     
//Constructor. Parameter is path to config . file - absolute or relative. */     
   threadsafe_queue(string cpath):
   head(new node),tail(head.get())    
   {
       head->next = nullptr;
       
       /** Some settings by default */
       some_vars.SignalFlag=false;   /* It is time to comlete log_thread */
       some_vars.ThrEnd=0;           /* Have log_thread been completed yet */
       some_vars.ThrCycles=1000;      /** number of cycles to wait of thread completing. */
       some_vars.thrWaitTime=10;     /** msec, period of time for one cycle to wait of thread completing. */   
       
       some_vars.reg_size=700;       /** bytes, the size of one Log file.*/
       some_vars.num_regs=25;        /** max. number of Log files, after exceeding that one of existing ones will be rewritten.*/
       some_vars.curr_reg_file=0;          /** the number of current Log file in which do writing. Counting from 0.*/
       
       some_vars.LogDir = ".";              /** Directory for Log files by default. */
       some_vars.barrier="DEBUG";           /** barrier level of logging */
       some_vars.level_barrier=1;            /** barrier level of logging as type int */
       some_vars.unit_of_measure="BYTES";   /** Assign  unit of measure for reg_size */   
       
       some_vars.is_stdout="YES";     /** Do Print Logs to console stdout. YES - print, NO - do not print. */
       some_vars.pattern="";     /** Pattern to filter records in Log files. Pattern must satisfy regex rules. */ 
       some_vars.confPath="./Logger.cfg";   /** Default path to config . file - absolute or relative. */
       
       size_threadsafe_queue=0;      /** the size of  the threadsafe_queue at start up time.*/
       some_vars.overstocking=500;   /** The limit of overstocking for queue. This is max. number of elements in the queue. */
       
       Configure(cpath);   
       
   };/*end Constructor */
  
//Destructor  
   ~threadsafe_queue()
   {
      // Close log_thread
      some_vars.SignalFlag=true;
      queueCondVar. notify_all();
      for(int i=0;i<some_vars.ThrCycles;i++)     {
         if( some_vars.ThrEnd == 0)      {
              some_vars.SignalFlag=false;
              break;
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(some_vars.thrWaitTime));
 
      }   
      
      some_vars.fout.close();
      cout << "Destructor of threadsafe_queue called.Program Complete." << endl;  
      
   };/*end of Destructor */
   

   threadsafe_queue(const threadsafe_queue& other)=delete;
 
   threadsafe_queue& operator=(const threadsafe_queue& other)=delete;
   
   std::shared_ptr<T> try_pop()
   {
     std::unique_ptr<node> old_head=pop_head();
     return old_head?old_head->data:std::shared_ptr<T>();
   };
   
   std::shared_ptr<T> trial_pop()
   {

     std::unique_ptr<node> old_head;  
    { 
       std::lock_guard<std::mutex> head_lock(head_mutex);
     
       if(head.get()==get_tail())
       {
         return nullptr;
       }
       old_head=std::move(head);
       head=std::move(old_head->next);
       if(head.get()==get_tail())
       {
         ////return nullptr;
        // cout << "trial_pop(): DEB POINT INTERSTING PLACE !!! " << endl;
       }
    }/*called ~std::lock_guard() */

    size_threadsafe_queue--;
      ////return old_head;       
     
     return old_head?old_head->data:std::shared_ptr<T>();
   };/*trial_pop() */
    
   

   void push(T new_value)
   {
// Advanced realization. 

       

      std::shared_ptr<T> new_data(

      std::make_shared<T>(std::move(new_value)));



      std::unique_ptr<node> p(new node); 

      node* const new_tail=p.get();

     {

      std::lock_guard<std::mutex> tail_lock(tail_mutex);

      tail->data=new_data;

      tail->next=std::move(p);

      tail=new_tail;

      }/* called  ~std::lock_guard() */

      size_threadsafe_queue++;

      queueCondVar.notify_one(); /**react to every message. */

      

    //// OVERSTOCKING.

       int tmpsize=size_threadsafe_queue.load();

       if( tmpsize > some_vars.overstocking)     {

          clean_queue();

          string tmpMsg = "OVERSTOCKING: cleaned size_threadsafe_queue = " + to_string(tmpsize);

          push(tmpMsg);

          cout << tmpMsg <<  endl;

       }/*ref. to if(size_threadsafe_queue > excess) */ 

// END: Advanced realization.        


   };/*ref. to push() */
   

   
//The other funcs.


   /*
    *   Print all messages from the list on stdout. 
    */
   
   void go_around_list(void)      
   {
      node* pListCurrent = head.get();
      while( pListCurrent->next != nullptr )    {  
          
            cout <<  (*pListCurrent->data) << endl;
            cout.flush();
            pListCurrent = pListCurrent->next.get();
      }       
      
   };/*end of go_around_list()  */
   
   
   /*
    *   Add a user's message to the list.
    *   Parameters: 
    *             fl - file name
    *             cla - class name
    *             func - function name 
    *             LogLeve -  level 
    *             message -  user's message
    */   
   void  PutToLog( string fl, string cla, string func, LogLevel Lev, string& message)
   {
       string msg;
       string LevStr;
       string sep = ", ";
       string gap = "   "; 
       string delimiter = "/";
       string space = " ";
       std::stringstream ss;
       string DateTime;
       
       string mangledClassName =cla;
       string demangledClassName = demangle(mangledClassName.c_str() );
       
       
       auto now = chrono::system_clock::now();
       time_t t = chrono::system_clock::to_time_t(now);
       tm* nowTM = localtime(&t);
       ss << std::put_time(nowTM, "date: %x time: %X");
       DateTime = ss.str();
       
       boost::filesystem::path p(fl);
       std::string chfl=p.filename().c_str();
       
       if( (Lev == LogLevel::DEBUG) )   {
          LevStr = "DEBUG";
          if(  static_cast<int>(Lev) >= some_vars.level_barrier )      {
               msg = DateTime+gap+chfl+delimiter+demangledClassName+delimiter+func+delimiter+gap+LevStr+sep+message; 
          }
          else       {
              /**Messages < barier does not write to Log */
              return; 
          }
       }
       if( Lev == LogLevel::INFO)   {
          LevStr = "INFO";
          if(  static_cast<int>(Lev) >= some_vars.level_barrier )      {
              msg = DateTime+gap+chfl+delimiter+demangledClassName+delimiter+func+delimiter+gap+LevStr+sep+message; 
          }
          else       {
              /**Messages < barier does not write to Log */
              return; 
          }
       }
       if( Lev == LogLevel::WARN)   {
          LevStr = "WARN";
          if(  static_cast<int>(Lev) >= some_vars.level_barrier )      {
              msg = DateTime+gap+chfl+delimiter+demangledClassName+delimiter+func+delimiter+gap+LevStr+sep+message; 
          }
          else       {
              /**Messages < barier does not write to Log */
              return; 
          }
       }
       if( Lev == LogLevel::ERROR)   {
          LevStr = "ERROR";
          if(  static_cast<int>(Lev) >= some_vars.level_barrier )      {  
              msg = DateTime+gap+chfl+delimiter+demangledClassName+delimiter+func+delimiter+gap+LevStr+sep+message; 
          }
          else       {
              /**Messages < barier does not write to Log */
              return; 
          }
       }
        
       if(some_vars.pattern !="")      {    /** print out through filter.*/
          regex reg1(some_vars.pattern); 
          bool found = regex_search(msg, reg1); 
          if(found)       {   
             push(msg); 
          }
       }
       else      {   /** print out as usually */
         push(msg);
       }
   
   };/* end of PutToLog()*/
   

   
  /* Look the size of current Log file. If it greater then wanted close it and give the next one.
   * If all existing Log files are full, take the first one.
   * some_vars.CurrLogFile
   * return:
   *           0 - success
   *          -1 - failure
   */
   int SwitchCurrLogFile(void)
   {
        some_vars.fout.flush();
        int file_size = some_vars.fout.tellp();

         if( file_size > some_vars.reg_size)     {
             some_vars.fout.close(); /** close current Log file. */  
             // Open the next Log file where to write or rewrite the first of existing.
             some_vars.curr_reg_file++;  /**look for the nex Log file to write. */
             std::stringstream ss;
             if( some_vars.curr_reg_file < some_vars.num_regs)        {   /** take the next Log file*/
                 
                 ss << boost::format("register%1%.txt") % some_vars.curr_reg_file;
                 some_vars.CurrLogFile =some_vars.LogDir+"/"+ss.str(); 
                 
                 some_vars.fout.open(some_vars.CurrLogFile, ios::out); 
                 if (!some_vars.fout.is_open())     {
                     cout << "Can not open Log file for writing !!! Stop." << endl;
                     return -1;
                 }
                 some_vars.fout << "Current Log file run out. Name of The NEXT Log File is  " << some_vars.CurrLogFile << endl;
                 some_vars.fout.flush();
             }   
             else           {     /** All free Log files run out. Take the first one.*/
                 some_vars.curr_reg_file=0;
                 ss << boost::format("register%1%.txt") % some_vars.curr_reg_file;
                 some_vars.CurrLogFile = some_vars.LogDir+"/"+ss.str();
                 
                 some_vars.fout.open(some_vars.CurrLogFile, ios::out); 
                 if (!some_vars.fout.is_open())     {
                     cout << "Can not open Log file for writing !!! Stop." << endl;
                     return 0;
                 }
                 some_vars.fout << "All free Log files run out. Take the first one: " << some_vars.CurrLogFile << endl;
                 some_vars.fout.flush();
             }   
             
         }/*ref. to  if( stat_buf.st_size > some_vars.reg_size) */
              
       return 0;    
       
   };/*end of SwitchCurrLogFile() */
   
   
   
    /*  

     *     Read config. file  for example "Logger.cfg" and form  std::map,
     *     If Logger does not find its config file, it uses default settings.
     *     in which key field is one of configuration parameters and value field
     *     is its value. Considered, that each configuration parameter has a unique name.
     */

    void ReadConfig(std::map <std::string, std::string> & cmdToInsert)

    {
       const std::string SPACE = " ";
       std::string textStr;
       std::string key;
       std::string  params;

       textStr.clear();
       key.clear();
       params.clear();

       std::ifstream fin(some_vars.confPath);
       if (fin.is_open())
       {
          ;
       }   
       else    {
          cout << "ERROR in openning config. file. Incorrect path." << some_vars.confPath << endl; 
       }

       if(!fin.fail())
       {
          while(!fin.eof())      {
              std::getline(fin, textStr);  
              std::string str = textStr;
              
             if( textStr.empty() )        {
                 /** Empty parameters is not treated. */
                 continue; 
              }              
              
              if(textStr.front() == '#')    {
                /** Comments is not treated. */
                 continue;                   
              }
              
               /** Only space and tab symbols in parameter line are not treated. */
              std::string::const_iterator it = textStr.begin();

              while (it != textStr.end() && ( (*it) == ' ' || (*it) == ' ' ) )        {
                      ++it;
              }
              if( !textStr.empty() && it == textStr.end() )       {
                  cout << "Ther is a line with spaces and tabs only symbols: " << textStr << "!!!" << endl; 
                  continue;
              }
              else  {
                  ;
              }            
            
              ////CONSTRUCTOR WITH DELIMITERS.
              typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

              boost::char_separator<char> sep(",:;| ");

              //  boost::char_separator<char> sep(",:;|.");
              tokenizer tokens(str, sep);
              for (tokenizer::iterator tok_iter = tokens.begin();
                 tok_iter != tokens.end(); ++tok_iter)                    {
                 if( tok_iter == tokens.begin()  )     {
                     key = *tok_iter;
                     boost::algorithm::trim(key);
                     ////cout << "key = " << key << endl;
                 }
                 else     {
                     params = params + SPACE+ (*tok_iter) + SPACE;
                     boost::algorithm::trim(params);
                     ////cout << "params = " << params << endl;
                 }

              }/*ref. to for(;;) */

              cmdToInsert.insert ( std::pair<std::string,std::string>(key, params) );
              textStr.clear();
              key.clear();
              params.clear();

          }/* ref. to while() */
          fin.close();
       } 
       else     {  
         std::cout << "file \"Logger.cfg\" not found. Applied default params." << std::endl;
       }

    };/*end of ReadConfig() */
   
   
   /*

    * Check if given string path is of a Directory

    */

   bool checkIfDirectory(std::string filePath)
   {
       try {
           // Create a Path object from given path string
           filesys::path pathObj(filePath);
           // Check if path exists and is of a directory file
           if (filesys::exists(pathObj) && filesys::is_directory(pathObj))
               return true;
       }
       catch (filesys::filesystem_error & e)
       {
           std::cerr << e.what() << std::endl;
       }
       return false;
       
   };/*end of checkIfDirectory() */
   
   
   /*
    *  Create directory tree recursively.
    *  parameters:
    *                   s -    end path
    *                   mode - attributes
    */
   int mkpath(std::string s,mode_t mode)
   {
       size_t pos=0;
       std::string dir;
       int mdret;
       
       if(s[s.size()-1]!='/'){
           // force trailing / so we can handle everything in loop
           s+='/';
       }

       
       while((pos=s.find_first_of('/',pos))!=std::string::npos){
           dir=s.substr(0,pos++);
           if(dir.size()==0) continue; // if leading / first time is 0 length
               boost::filesystem::path Dir(dir);
               int mkdirres=boost::filesystem::create_directory(Dir);
               if(mkdirres)     {
                   mdret=0;
               }
               else    {
                   mdret=-1;
               }    
       }/*ref. to  while((pos=s.find_first_of('/',pos))!=std::string::npos)*/

       return mdret;

   }; /*end of mkpath()  */
   
 
 
 
    /*
     *  This function configures settings taken them from
     *  configuration file at start up.
     *  Parameters:
     *    cpath - path to config.file, full or relative.
     */
 
    int Configure(string cpath)
    {
     /** lock both mutexes without deadlock */
     {  
       
       std::lock(head_mutex, tail_mutex);

       // make sure both already-locked mutexes are unlocked at the end of scope
       std::lock_guard<std::mutex> lock1(head_mutex, std::adopt_lock);
       std::lock_guard<std::mutex> lock2(tail_mutex, std::adopt_lock);
       
       
       some_vars.confPath=cpath;   /** path to config . file - absolute or relative. */
       boost::algorithm::trim( some_vars.confPath);
         
       ReadConfig(some_vars.cmdStr);
       
       // Now do assignment from cfg file.
     
       std::map<std::string, std::string>::iterator it = some_vars.cmdStr.find("BARRIER");

       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.barrier= it->second;  
           boost::algorithm::trim( some_vars.barrier);
             /** barrier level of logging as type int */
           if( some_vars.barrier == "DEBUG")       {
               some_vars.level_barrier =1;
           }
           else if( some_vars.barrier == "INFO")       {
               some_vars.level_barrier =2;
           }
           else if( some_vars.barrier == "WARN")       {
               some_vars.level_barrier =3;
           }
           else if( some_vars.barrier == "ERROR")       {
               some_vars.level_barrier =4;
           }
    
       }

                   
       it = some_vars.cmdStr.find("LOG_DIRECTORY");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.LogDir=it->second; 
           boost::algorithm::trim( some_vars.LogDir);
           bool dirresult = checkIfDirectory(some_vars.LogDir);
           if(dirresult==false)     {
               /** create new directory*/
               int mkdirretval;
             
               mkdirretval=mkpath(some_vars.LogDir,0755);
               if(mkdirretval == 0)     {
                   ;
               }
               else       {
                   cout << "Configure(): ERROR IN  creating new Directory " << some_vars.LogDir << endl;               
               }               
               
           }
       }    
       
       it = some_vars.cmdStr.find("UNIT_OF_MEASURE");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.unit_of_measure=it->second; 
           boost::algorithm::trim(some_vars.unit_of_measure);
        }                   
       
       it = some_vars.cmdStr.find("REG_SIZE");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           if( some_vars.unit_of_measure == "BYTES" )     {
               try
               {
                  some_vars.reg_size=std::stoi(it->second);  
               }
               catch(...)
               {}
           }
           else if (some_vars.unit_of_measure == "KBYTES" )      {
               try 
               {
                   some_vars.reg_size = std::stoi(it->second)*1024;
               }
               catch(...)
               {}
           } 
           else if (some_vars.unit_of_measure == "MBYTES")      {
               try 
               {
                   some_vars.reg_size = std::stoi(it->second)*1024*1024;
               }
               catch(...)
               {}
           }
           
       }                   
       
       it = some_vars.cmdStr.find("NUM_REGS");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           try 
           {
               some_vars.num_regs=std::stoi(it->second);  
           }
           catch(...)
           {}
       }                   

       it = some_vars.cmdStr.find("STDOUT");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.is_stdout=it->second;  
           boost::algorithm::trim( some_vars.is_stdout);
       }       
       
       
       it = some_vars.cmdStr.find("PATTERN");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.pattern=it->second;  
           boost::algorithm::trim( some_vars.pattern);
       }       
 
       it = some_vars.cmdStr.find("OVERSTOCKING");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           try   
           {
              some_vars.overstocking=std::stoi(it->second);  
           }
           catch(...)
           {
           }
       }                   
 
       
     }/** called  unlock both mutexes without deadlock */
       
       // Print content of some_vars for debugging purpose.

       std::cout << std::endl << std::endl;

       std::cout << "Configure(): Parameters read from config. file: " << std::endl;
       cout << "Configure(): PATH to config file is = " << some_vars.confPath << endl;

       cout << "some_vars.barrier = " << some_vars.barrier << endl;
       cout << "some_vars.level_barrier = " << some_vars.level_barrier << endl;
       cout << "some_vars.LogDir = " << some_vars.LogDir << endl;
       cout << "some_vars.unit_of_measure = " << some_vars.unit_of_measure << endl;
       cout << "some_vars.reg_size = " << some_vars.reg_size << endl;
       cout << "some_vars.num_regs = " << some_vars.num_regs << endl;
       cout << "some_vars.is_stdout = " << some_vars.is_stdout << endl;
       cout << "some_vars.pattern = " << some_vars.pattern << endl;
       cout << "some_vars.overstocking = " << some_vars.overstocking << endl;
       
      return 0;
       
    };/* end of Configure()*/        

 
 
 
    /*
     *  This function reconfigures settings taken them from
     *  configuration file.
     *  Parameters:
     *    cpath - path to config.file, full or relative.
     */
 
   int ReConfigure(string cpath)
   {
       
     /** lock both mutexes without deadlock */
     {  
       
       std::lock(head_mutex, tail_mutex);

       // make sure both already-locked mutexes are unlocked at the end of scope

       std::lock_guard<std::mutex> lock1(head_mutex, std::adopt_lock);

       std::lock_guard<std::mutex> lock2(tail_mutex, std::adopt_lock);

       
       some_vars.confPath=cpath;   /** path to config . file - absolute or relative. */
       boost::algorithm::trim( some_vars.confPath);
       
       ////cout << "ReConfigure(): PATH to config file is = " << some_vars.confPath << endl;
       
       
       ReadConfig(some_vars.cmdStr);
       
       // Now do assignment from cfg file.
     
       std::map<std::string, std::string>::iterator it = some_vars.cmdStr.find("BARRIER");

       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.barrier= it->second;  
           boost::algorithm::trim( some_vars.barrier);
             /** barrier level of logging as type int */
           if( some_vars.barrier == "DEBUG")       {
               some_vars.level_barrier =1;
           }
           else if( some_vars.barrier == "INFO")       {
               some_vars.level_barrier =2;
           }
           else if( some_vars.barrier == "WARN")       {
               some_vars.level_barrier =3;
           }
           else if( some_vars.barrier == "ERROR")       {
               some_vars.level_barrier =4;
           }
    
       }

                   
       it = some_vars.cmdStr.find("LOG_DIRECTORY");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.LogDir=it->second; 
           boost::algorithm::trim( some_vars.LogDir);
           bool dirresult = checkIfDirectory(some_vars.LogDir);
           if(dirresult==false)     {
               /** create new directory*/
               int mkdirretval;
             
               mkdirretval=mkpath(some_vars.LogDir,0755);
               if(mkdirretval == 0)     {
                   ;
               }
               else       {
                   // Look the size of current Log file. If it greater then wanted close it and give the next one.
                   // If all existing Log files are full, take the first one.
                   //some_vars.CurrLogFile
                   SwitchCurrLogFile();
 
                   cout << "ReConfigure(): ERROR IN  creating new Directory " << some_vars.LogDir << endl;               
               }               
               
           }
       }    
       
       it = some_vars.cmdStr.find("UNIT_OF_MEASURE");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.unit_of_measure=it->second; 
           boost::algorithm::trim(some_vars.unit_of_measure);
       }                   
       
       it = some_vars.cmdStr.find("REG_SIZE");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           if( some_vars.unit_of_measure == "BYTES" )     {
               try 
               {
                    some_vars.reg_size=std::stoi(it->second);  
               }
               catch(...)
               {}
           }
           else if (some_vars.unit_of_measure == "KBYTES" )      {
               try 
               {
                   some_vars.reg_size = std::stoi(it->second)*1024;
               }
               catch(...)
               {}
           } 
           else if (some_vars.unit_of_measure == "MBYTES")      {
              try 
              {
                  some_vars.reg_size = std::stoi(it->second)*1024*1024;
              }
              catch(...)
              {}
           }
           
       }                   
       
       it = some_vars.cmdStr.find("NUM_REGS");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           try 
           {
              some_vars.num_regs=std::stoi(it->second);  
           }
           catch(...)
           {}
       }                   

       it = some_vars.cmdStr.find("STDOUT");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.is_stdout=it->second;  
           boost::algorithm::trim( some_vars.is_stdout);
       }       
       
       
       it = some_vars.cmdStr.find("PATTERN");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           some_vars.pattern=it->second;  
           boost::algorithm::trim( some_vars.pattern);
       }    
       
       it = some_vars.cmdStr.find("OVERSTOCKING");
           
       if(it != some_vars.cmdStr.end() )     {
           /** found */
           try 
           {
               some_vars.overstocking=std::stoi(it->second);  
           }
           catch(...)
           {}
       }                   
       
       
     }/** called  unlock both mutexes without deadlock */
     

      //Do record in Log file about Reconfigure() call.
      //Do it pushing corresponding message to the threadsafe_queue.
      string ms = "Occured the ReConfigure() call with config. file " + cpath;
      push(ms);

      
       // Print content of some_vars for debugging purpose.
       std::cout << std::endl << std::endl;
       std::cout << "Reconfigure(): Parameters read from config. file: " << std::endl;
       cout << "ReConfigure(): PATH to config file is = " << some_vars.confPath << endl;

       cout << "some_vars.barrier = " << some_vars.barrier << endl;
       cout << "some_vars.level_barrier = " << some_vars.level_barrier << endl;
       cout << "some_vars.LogDir = " << some_vars.LogDir << endl;
       cout << "some_vars.unit_of_measure = " << some_vars.unit_of_measure << endl;
       cout << "some_vars.reg_size = " << some_vars.reg_size << endl;
       cout << "some_vars.num_regs = " << some_vars.num_regs << endl;
       cout << "some_vars.is_stdout = " << some_vars.is_stdout << endl;
       cout << "some_vars.pattern = " << some_vars.pattern << endl;
       cout << "some_vars.overstockingn = " << some_vars.overstocking << endl;
       
      return 0;
       
   };/*end of ReConfigure() */
   

   
  /*

   *   Log Thread function to do reading from the List

   *   and write it in Log file.

   */   

   void  LogThreadFunc( void) 

   {   

       

////  Advanced realization.

       

    std::shared_ptr<string> retObj;

    ////int tcount=0;  

    int tmpsize=0;

    int debcnt=0;

       

    while (some_vars.SignalFlag==false)    {

       std::unique_lock<std::mutex> head_lock(head_mutex);

       queueCondVar.wait(head_lock,[&]{ return (head.get() != get_tail() ) || some_vars.SignalFlag; });

       head_lock.unlock();

       debcnt++;
       

        //// OVERSTOCKING.

       tmpsize=size_threadsafe_queue.load();

       if( tmpsize > some_vars.overstocking)     {

          clean_queue();

          string tmpMsg = "OVERSTOCKING: cleaned size_threadsafe_queue = " + to_string(tmpsize);

          push(tmpMsg);

          cout << tmpMsg <<  endl;

       }/*ref. to if(size_threadsafe_queue > excess) */ 


  
    // For debuggung only.
#if 0    
//-------------    

       if( tcount == 0 )   {

          ////cout << "LogThreadFunc(): tcount = " << tcount << endl;

          tcount++;

       }

       else if (tcount == 1 )      {

          

           if( (debcnt%50)==0)      {

               cout << "LogThreadFunc(): tcount = " << tcount <<"  tmpsize = " << tmpsize << " debcnt = " << debcnt << endl;

           }

         

          tcount=0;

       }/*ref .to  else if (tcount == 1 ) */

//------------
#endif
     

       

         while(true)       {

             // Look the size of current Log file. If it greater then wanted close it and give the next one.

             // If all existing Log files are full, take the first one.

             //some_vars.CurrLogFile

             SwitchCurrLogFile();

             retObj=std::move( try_pop() );

             if(retObj!= nullptr)      {   

                if(some_vars.is_stdout == "YES")     {

                    cout << (*retObj) << endl;

                    cout.flush();

                }

                some_vars.fout << (*retObj) << endl;
                some_vars.fout.flush();
             }

             else    {

                ;
                break;

             }   

             

         }/* while(true) */


      // When we get to the place, it's mean that Destructor was called.

      // So, an object of threadsafe_queue quits its scope. 

      // So, there must not be any elements in the queue!

      if(some_vars.SignalFlag==true)      {

         //cout << "LogThreadFunc(): STOP #7 size_threadsafe_queue =  " << size_threadsafe_queue << endl; 

         some_vars.ThrEnd--;

         return; 

      }

         

    }/* ref. to while (some_vars.SignalFlag==false)  */

    
   };/* end of LogThreadFunc()*/    







  /*
   *  Start Log Thread function
   */  
   void RunLogging(void)
   {
      // Open the first Log file where to write.
       std::stringstream ss;
       ss << boost::format("register%1%.txt") % some_vars.curr_reg_file;
       some_vars.CurrLogFile = some_vars.LogDir+"/"+ss.str();
       
       some_vars.fout.open(some_vars.CurrLogFile, ios::out); 
       if (!some_vars.fout.is_open())     {
           cout << "Can not open Log file for writing !!! Stop." << endl;
           return;
       }

       auto tmp_log_thread = std::async(std::launch::async,&threadsafe_queue::LogThreadFunc,this);
       log_thread = move(tmp_log_thread);
       some_vars.ThrEnd++;
       cout << " -- started log_thread..." << endl;
       some_vars.fout << " -- started log_thread..."  << endl; 
       some_vars.fout.flush();
      
   };/* end of RunLogging()*/ 
   

}; /* class threadsafe_queue  */

  
  
  
/** Fictitious global func. Otherwise, the library not linked. */
void TemporaryFunction()
{
    
    threadsafe_queue<string> vvLogQueue("some_path");
    threadsafe_queue<string>* vvLq = &vvLogQueue;
    string vvstrThread = "Hello, This is thread ";
    string fl="some_file";
    string cla="some_class";
    string func="some_func";
    vvLq->PutToLog( fl, cla, func, LogLevel::DEBUG, vvstrThread);
    vvLq->ReConfigure("some_path");
     
    vvLq->RunLogging();
    
    vvLq->go_around_list();

};/*TemporaryFunction()*/
 
  
  
  
  
  
  
  
  
  
  
  
  
