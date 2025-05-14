#pragma once

#include "global.hpp"
#include "aicamera.hpp"

void gst_test(int testCase);
void gst_test2(int testCase);

void stopPipeline();


#if defined(ENABLE_ARAVIS)

void aravisTest();

#endif // ENABLE_ARAVIS

