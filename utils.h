/*
 * utils.h
 *
 *  Created on: 10 Nov 2014
 *      Author: marcin
 */

#ifndef UTILS_H_
#define UTILS_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>
#include <fcntl.h>

typedef struct StructTime {
        int hour;
        int minute;
}	StructTime;

StructTime* currentTime();


int readFile(const char* path, char** fileContent, struct stat* fileStat);


#endif /* UTILS_H_ */
