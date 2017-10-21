//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 4

//taken from https://stackoverflow.com/questions/3673226/how-to-print-time-in-format-2009-08-10-181754-811

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/time.h>

#define DEBUG 0

void getTime(char* buffer) {
	  int millisec;
	  struct tm* tm_info;
	  struct timeval tv;

	  gettimeofday(&tv, NULL);

	  millisec = (int) (tv.tv_usec/1000.0); // Round to nearest millisec

	  if (millisec>=1000) { // Allow for rounding up to nearest second
	    millisec -=1000;
	    tv.tv_sec++;
	  }
	  char mils[3];
	  sprintf(mils, "%03d", millisec);

	  tm_info = localtime(&tv.tv_sec);

	  strftime(buffer, 26, "%Y-%m-%d %H:%M:%S.", tm_info);
	  strcat(buffer,mils);
	if (DEBUG) printf("timestamp: %s\n", buffer);
}

long getUnixTime() {
	struct timeval  tv;
	gettimeofday(&tv, NULL);

	return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}



