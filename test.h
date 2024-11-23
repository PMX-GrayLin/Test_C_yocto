
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
#define xlog
#else
#define xlog printf
#endif
