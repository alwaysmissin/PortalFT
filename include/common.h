#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>

#include <macro.h>

#define MAXLINE 128
#define MAXTHREAD 8

void init_log(const char *log_file);
#endif