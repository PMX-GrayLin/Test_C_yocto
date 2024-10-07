#include "test.h"

using namespace std; 

uint32_t crc_table[256];


int main(int argc, char *argv[])
{
	xlog("%s:%d, argc:%d \n\r", __func__, __LINE__, argc);
	for (int i = 0; i < argc; ++i) {
		xlog("%s:%d, argv[%d]:%s \n\r", __func__, __LINE__, i, argv[i]);
	}

	if( argc < 2 )
	{
		xlog("%s:%d, input more than 1 params... \n\r", __func__, __LINE__);
		getchar();
		return -1;
	}

	xlog("%s:%d, ===================================== \n\r", __func__, __LINE__);
	// cases -------------------------------------------------
	if ( !strcmp(argv[1], "test") ) {

		xlog("%s:%d, test... \n\r", __func__, __LINE__);

	} 
}
