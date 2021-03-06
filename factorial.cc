#include <cstdlib>
#include <stdlib.h>
#include <cstdio>
#include <string.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <fstream>
#include <sys/stat.h>
#include <my_global.h>
#include <my_sys.h>

#include <new>
#include <vector>
#include <algorithm>

#if defined(MYSQL_SERVER)
#include <m_string.h>		/* To get my_stpcpy() */
#else
/* when compiled as standalone */
#include <string.h>
#define my_stpcpy(a,b) stpcpy(a,b)
#endif

#include <mysql.h>
#include <ctype.h>

#ifdef _WIN32
/* inet_aton needs winsock library */
#pragma comment(lib, "ws2_32")
#endif

#ifdef HAVE_DLOPEN

#if !defined(HAVE_GETHOSTBYADDR_R) || !defined(HAVE_SOLARIS_STYLE_GETHOST)
static native_mutex_t LOCK_hostname;
#endif

C_MODE_START;

my_bool factorial_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
 void factorial_deinit(UDF_INIT *initid);
longlong factorial(UDF_INIT *initid, UDF_ARGS *args, char *is_null,
		   char *error);

C_MODE_END;


my_bool factorial_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{
  if (args->arg_count != 1)
  {
    my_stpcpy(message,"This function takes 1 argument");
    return 1;
  }
	
  if (args->arg_type[0] != INT_RESULT)
  {
     strcpy(message,"factorial() requires an integer");
     return 1;
  }

  if (*((longlong*)args->args[0]) < 0)
  {
    my_stpcpy(message,"Negative number passed");
    return 1;
  }
  //Start Check if python installed
  int i;
  i = system("which python");
  if (i == 256)
  {
    my_stpcpy(message,"Could not find python, please check if it is installed and in your PATH env");
    return 1;
  }
	//END Check python installed
  extern char *opt_plugin_dir_ptr;
  std::string python_file("factorial.py");
  std::string pyfile(opt_plugin_dir_ptr);
  pyfile.append(python_file);
  std::fstream file(pyfile.c_str());
  struct stat buffer;
	/*Check if file exists*/
    if (stat (pyfile.c_str(), &buffer) != 0){
    my_stpcpy(message,"Cannot access 'factorial.py': No such file inside MySQL plugin directory.");
    return 1;
    }
  /*File exits but can't open.*/
  if (!file)
  {
    my_stpcpy(message,"Can't access 'factorial.py': Check file permissions.");
    return 1;
  }

  if (args->arg_count)
    args->arg_type[0]= INT_RESULT;		/* Force argument to int */

  if (!(initid->ptr=(char*) malloc(sizeof(longlong))))
  {
    my_stpcpy(message,"Couldn't allocate memory");
    return 1;
  }
  memset(initid->ptr, 0, sizeof(longlong));
 
  initid->const_item=0;
  return 0;
}

void factorial_deinit(UDF_INIT *initid)
{
  if (initid->ptr)
    free(initid->ptr);
}

longlong factorial(UDF_INIT *initid MY_ATTRIBUTE((unused)), UDF_ARGS *args,
                  char *is_null MY_ATTRIBUTE((unused)),
                  char *error MY_ATTRIBUTE((unused)))
{
  extern char *opt_plugin_dir_ptr;

  ulonglong val=0;
  
  if (args->arg_count)
  {
      
      val= *((longlong*) args->args[0]);
      //fprintf(stderr, "Value of args->args[0] -> %s\n", args->args[0]);
      //fprintf(stderr, "Value of args->args[0] -> %lld\n", val);
    
      
      //fprintf(stderr, "%s\n", opt_plugin_dir_ptr);
      std::string command("python");
      std::string plugin_dir(opt_plugin_dir_ptr);
      std::string python_file("factorial.py");
      
      //std::string command("python opt_plugin_dir_ptr/factorial.py");

      char str[10];
      sprintf(str, "%lld", val);
      command.append(" ");
      command.append(plugin_dir);
      command.append(python_file);
      command.append(" ");
      command.append(str);
    
      //fprintf(stderr, "Value of command.c_str() -> %s\n", command.c_str());

      std::string result = "";
      std::shared_ptr<FILE> pipe(popen(command.c_str(), "r"), pclose);
      while (!feof(pipe.get())) {
          if (fgets(initid->ptr, 128, pipe.get()) != NULL)
              result += initid->ptr;
      }
    
      //fprintf(stderr, "The Result -> %s\n", result.c_str());
    
      std::string::size_type sz;
      return std::stol(result, &sz);
    }
  }

  //return (longlong)initid->ptr;

#endif
