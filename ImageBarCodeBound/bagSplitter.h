#ifndef __BAG_SPLITTER_H__
#define __BAG_SPLITTER_H__
#include "opencv2/opencv.hpp"

#ifdef _cplusplus
extern "c" {
#endif

	int bindImageAndBarCode(std::string strJSONIn, std::string & strJSONOut);
#ifdef _cplusplus
}
#endif

#endif
