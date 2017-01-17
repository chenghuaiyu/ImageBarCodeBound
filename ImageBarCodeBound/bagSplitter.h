#ifndef __BAG_SPLITTER_H__
#define __BAG_SPLITTER_H__
#include "opencv2/opencv.hpp"

#ifdef _cplusplus
extern "c" {
#endif

	int bindImageAndBarCode(const char * pszJSONIn, char ** ppszJSONOut);
	//int bindImageAndBarCode(const char * pszJSONIn, std::string & strJSONOut);
	int freeJSONMemory(char ** ppszJSONOut);

#ifdef _cplusplus
}
#endif

#endif
