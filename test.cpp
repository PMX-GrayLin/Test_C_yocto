#include "test.h"
#include "test_gst.h"
#include "test_ocv.h"

// using namespace std; 

int main(int argc, char *argv[]) {
  xlog("");

  for (int i = 0; i < argc; ++i) {
    xlog("argv[%d]:%s", i, argv[i]);
  }

  if (argc < 2) {
    xlog("to input more than 1 params...");
    return -1;
  }

  if (!strcmp(argv[1], "test")) {
    xlog("");
  } else if (!strcmp(argv[1], "gst")) {
	int testCase = (argv[2] == nullptr) ? 0 : 1;
    gst_test(testCase);
  } else if (!strcmp(argv[1], "ocv")) {
	int testCase = (argv[2] == nullptr) ? 0 : 1;
    ocv_test(testCase);
  }

  return 0;
}
