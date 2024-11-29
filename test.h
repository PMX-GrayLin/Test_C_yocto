
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

#include <vector>
#include <algorithm>
#include <thread>
#include <string> 

// yocto test
#include "json.h" 

#define DEBUGX

#ifndef DEBUGX
#define xlog(...) ((void)0)
#else
#define xlog(fmt, ...) printf("%s:%d, " fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#endif
