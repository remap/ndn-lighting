#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "Daemon.h"
#include "CCNListener.h"

void _usage()
{
  fprintf(stdout, "ccnrp [ -d ] [ -p <port> ] [ -h ]\n");
}

int _daemonize()
{
  int iRet = -1;
  pid_t tPid = -1;

  //fprintf(stderr, "Daemonizing...\n");
  if ((tPid = fork()) < 0)
  {
    fprintf(stderr, "Unable to fork: %s\n", strerror(errno));
  }
  else if (tPid > 0)
  {
    iRet = tPid;
  }
  else if (setsid() < 0)
  {
    fprintf(stderr, "Unable to create new process group: %s\n", strerror(errno));
  }
  else
  {
    close(0);
    close(1);
    close(2);
    int iFD = open("/dev/null", O_RDWR);
    if (iFD < 0)
    {
      fprintf(stderr, "Unable to open /dev/null: %s\n", strerror(errno));
    }
    else
    {
      int iRetVal = dup(iFD);
      iRetVal = dup(iFD);
      iRetVal = 0;

      if (chdir("/") < 0)
      {
        fprintf(stderr, "Unable to chdir to /: %s\n", strerror(errno));
      }
      else
      {
        iRet = 0;
      }
    }
  }

  return iRet;
}

int main(int argc, char *argv[])
{
  int iRet = 0;

  int iPort = CCNRP_DEFAULT_PORT;

  bool bDaemon = false;
  int c = 0;
  while ((c = getopt(argc, argv, "p:dh")) != -1)
  {
    switch (c)
    {
      case 'p':
        iPort = (int) strtol(optarg, NULL, 10);
        if (0 == iPort)
        {
          fprintf(stderr, "Unable to convert %s to port number, exiting.\n", optarg);
          exit(1);
        }
        break;
      case 'h':
        _usage();
        exit(0);
        break;
      case 'd':
        // Turn into daemon
        bDaemon = true;
        break;
      case '?':
      default:
        fprintf(stderr, "Unable to parse command line.\n");
        _usage();
        exit(1);
        break;
    }
  }

  bool bRun = true;
  if (bDaemon)
  {
    bRun = false;
    int iPID = _daemonize();
    if (0 > iPID)
    {
      fprintf(stderr, "Unable to daemonize.  Exiting...\n");
      iRet = 1;
    }
    else if (0 < iPID)
    {
      fprintf(stdout, "ccnrp started w/ PID %d\n", iPID);
    }
    else
    {
      bRun = true;
    }
  }

  if (bRun)
  {
	  Listener oListener(iPort);
	  oListener.listen();	
  }

  return iRet;
}
