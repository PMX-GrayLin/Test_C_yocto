
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <thread>
#include <unistd.h>

// add at ubuntu 18.04
#include <stdint.h>
#include <string.h> 

// yocto test
#include "json.h" 

#define MIN(a,b) (a>b) ? b : a
#define MAX_STRING_SIZE	2048

#define DEBUGX

#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
// #define xlog(...) printf(__VA_ARGS__)
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif
