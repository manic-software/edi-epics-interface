#include <qapplication.h>

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include <fstream>
using std::ofstream;

#include "server.hh"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

Server* server=NULL;
QApplication* app;

//catch sigquit for elegant exit
void handler(int signum)
{  
  cout << "exiting\n";
  delete server;
  app->quit();  
  cout << "bye\n";
}

int
main(int argc, char* argv[])
{  
  if (argc<2 || strcmp(argv[1], "-f"))
      daemon(1,1);
  //standard qt init
  app=new QApplication (argc, argv, false);  
  //conbined tcp and epics server
  server=new Server;  
  //indicate that if we are sent a sigquit we want to know
  signal(SIGQUIT, handler);  
  //start
  app->exec();  
}
