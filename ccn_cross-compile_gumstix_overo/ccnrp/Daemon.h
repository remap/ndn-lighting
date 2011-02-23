#ifndef _DAEMON_H
#define _DAEMON_H

class Daemon
{
private:
	static void log_message(char *filename, char *message);
	static void signal_handler(int sig);
public:
	static void daemonize();
};

#endif
