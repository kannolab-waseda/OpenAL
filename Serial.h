#ifndef _Serial_h
#define _Serial_h

#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <vector>
#include <dirent.h>

#define BAUDRATE B9600           
#define MODEMDEVICE "/dev/cu.usbserial-A600afyl"
#define _POSIX_SOURCE 1

#define FALSE 0
#define TRUE 1

using namespace std;

class Serial {

public:
    int         fd, c, res;
    volatile int active;
	volatile int done;
    pthread_t   thread;
    struct      termios oldtio,newtio;
    char        buf[1024];
	int			rp, wp;
    char        device[128];
    vector<string> deviceList;
    
    void (*cb)(void *data, int size, void *user_data);
    void *ud;
    
    Serial();
    ~Serial();
    void threadStop();
    bool prepareSerial();
    void threadStart();
    void setDevice(const char *d);
    void setCallback(void (*callback)(void *data, int size, void *client), void *user_data);

	void writeData(void *data, int s);

private:
    static void *threadFunction(void *data);
    
};

#endif
