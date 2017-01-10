#ifndef __BAG_SPLITTER_H__
#define __BAG_SPLITTER_H__
#include "opencv2/opencv.hpp"

#ifdef _cplusplus
extern "c" {
#endif

//bool splitBags(std::vector<cv::Mat> vm, int nImgStitchDirection);
	bool bindImageAndBarCode(std::vector<cv::Mat> & vBags, std::vector<cv::Mat> vm, int nImgStitchDirection, std::vector<cv::Point> vp);

#ifdef _cplusplus
}
#endif

#endif
