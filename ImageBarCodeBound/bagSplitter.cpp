#include "bagSplitter.h"
#include <sstream>
using namespace cv;

#ifdef _DEBUG
#define SAVE_IMAGE
#endif

cv::Mat stitchImageRight(std::vector<cv::Mat> vm) {
	Mat m;
	if (0 == vm.size())
	{
		return m;
	}

	m = vm.at(0);
	for (int i = 1; i < vm.size(); i++)
	{
		Mat mNext = vm.at(i);
		assert(m.type() == mNext.type());
		assert(m.rows == mNext.rows);
		if (m.type() != mNext.type() || m.rows != mNext.rows)
		{
			return m;
		}
		int nCols = m.cols + mNext.cols;
		Mat mMerge = Mat::zeros(m.rows, nCols, m.type());
		m.copyTo(mMerge(Rect(0, 0, m.cols, m.rows)));
		mNext.copyTo(mMerge(Rect(m.cols, 0, mNext.cols, mNext.rows)));
		m = mMerge;
	}
	return m;
}

cv::Mat stitchImageLeft(std::vector<cv::Mat> vm) {

	Mat m;
	if (0 == vm.size())
	{
		return m;
	}

	m = vm.at(0);
	for (int i = 1; i < vm.size(); i++)
	{
		Mat mNext = vm.at(i);
		assert(m.type() == mNext.type());
		assert(m.rows == mNext.rows);
		if (m.type() != mNext.type() || m.rows != mNext.rows)
		{
			return m;
		}
		int nCols = m.cols + mNext.cols;
		Mat mMerge = Mat::zeros(m.rows, nCols, m.type());
		mNext.copyTo(mMerge(Rect(0, 0, mNext.cols, mNext.rows)));
		m.copyTo(mMerge(Rect(mNext.cols, 0, m.cols, m.rows)));
		m = mMerge;
	}

	return m;
}

cv::Mat stitchImage(std::vector<cv::Mat> vm, int nImgStitchDirection) {
	Mat m;
	switch (nImgStitchDirection)
	{
	case 0:
		return stitchImageLeft(vm);
		break;
	case 1:
			break;
	case 2 :
		return stitchImageRight(vm);
		break;
	case 3:
		break;
	default:
		break;
	}

	return m;
}

int getGrayThresh(Mat &grayImage)
{
	int nGrayThresh = 0;
	int nCounts[256] = {};
	for (int col = 0; col < grayImage.cols; col++) {
		nCounts[grayImage.at<unsigned char>(0, col)] ++;
		nCounts[grayImage.at<unsigned char>(1, col)] ++;
		nCounts[grayImage.at<unsigned char>(2, col)] ++;
	}

	int nMaxCount = nCounts[0];
	for (int i = 1; i < 256; i++) {
		if (nCounts[i] > nMaxCount) {
			nMaxCount = nCounts[i];
			nGrayThresh = i;
		}
	}
	//nGrayThresh -= min(10, nGrayThresh / 2);
	return nGrayThresh;
}

bool myPairCompare(std::pair<int, int> pi, std::pair<int, int> pj)
{
	return (pi.first < pj.first);
}

bool myTupleCompare(std::tuple<Mat, Rect, Point, int, int> ti, std::tuple<Mat, Rect, Point, int, int> tj)
{
	//int first = std::get<3>(ti);
	return (std::get<3>(ti) < std::get<3>(tj));
}

bool splitBags(std::vector<cv::Mat>& vecBags, cv::Mat m, int nImgMovDirection) {
	Mat mGray;
	cvtColor(m, mGray, CV_RGB2GRAY);
#ifdef SAVE_IMAGE
	imwrite("gray.png", mGray);
#endif
	const int nGrayThreshOffset = -5;
	int nGrayThresh = getGrayThresh(mGray);
	Mat mBin;
	GaussianBlur(mGray, mGray, Size(5, 5), 0);
	threshold(mGray, mBin, nGrayThresh + nGrayThreshOffset, 255, CV_THRESH_BINARY);
#ifdef SAVE_IMAGE
	imwrite("bin.png", mBin);
#endif

	std::vector<std::vector<cv::Point> > contours;
	//cv::findContours(mBin.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	//cv::findContours(mBin.clone(), contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
	Mat mBinInv = 255 - mBin;
	cv::findContours(mBinInv, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	const int contourLenThresh = 50;
	const int contourAreaThresh = 400;
	std::vector<std::vector<cv::Point> > contourFiltered;
	std::vector<cv::Rect> vecBagRect;
	std::vector<cv::Point> vecBagCenterPnt;
	std::vector<std::pair<int, int>> vecPairCenter;
	std::vector<std::tuple<Mat, Rect, Point, int, int>> vecTuple;
	int ind = 0;
	for (size_t idx = 0; idx < contours.size(); idx++) {
		std::vector<cv::Point> contour = contours.at(idx);
		Rect rc = boundingRect(contour);
		if (max(rc.height, rc.width) < contourLenThresh)
		{
			continue;
		}
		double area = contourArea(contour);
		if (area < contourAreaThresh)
		{
			continue;
		}

#ifdef SAVE_IMAGE
		//imwrite("roi.png", m(rc));
#endif

		Moments moment = moments(contour, true);
		int cX = moment.m10 / moment.m00;
		int cY = moment.m01 / moment.m00;
		//vecBags.push_back(m(rc));
		//vecBagRect.push_back(rc);
		//vecBagCenterPnt.push_back(Point(cX, cY));
		int c = nImgMovDirection % 2 == 0 ? cX : cY;
		vecPairCenter.push_back(std::pair<int, int>(c, vecPairCenter.size()));
		//contourFiltered.push_back(contour);
		
		std::tuple<Mat, Rect, Point, int, int> t(m(rc), rc, Point(cX, cY), c, idx);
		vecTuple.push_back(t);
		ind++;
	}

	//std::sort(vecPairCenter.begin(), vecPairCenter.end(), myPairCompare);
	std::sort(vecTuple.begin(), vecTuple.end(), myTupleCompare);
	for (size_t idx = 0; idx < vecTuple.size(); idx++) {
		vecBags.push_back(std::get<0>(vecTuple.at(idx)));
		vecBagRect.push_back(std::get<1>(vecTuple.at(idx)));
		vecBagCenterPnt.push_back(std::get<2>(vecTuple.at(idx)));
	}


#ifdef SAVE_IMAGE
	//Draw the contours
	cv::Mat mContours(mBin.size(), CV_8UC3, cv::Scalar(0, 0, 0));
	cv::Scalar colors[3];
	colors[0] = cv::Scalar(255, 0, 0);
	colors[1] = cv::Scalar(0, 255, 0);
	colors[2] = cv::Scalar(0, 0, 255);
	//for (size_t idx = 0; idx < contourFiltered.size(); idx++) {
	//	std::vector<cv::Point> contour = contourFiltered.at(idx);
	//	Rect rc = boundingRect(contour);
	//	if (max(rc.height, rc.width) < contourLenThresh)
	//	{
	//		continue;
	//	}
	//	double area = contourArea(contour);
	//	if (area < contourAreaThresh)
	//	{
	//		continue;
	//	}

	//	cv::drawContours(mContours, contourFiltered, idx, colors[idx % 3]);

	//	Moments moment = moments(contour, true);
	//	int cX = moment.m10 / moment.m00;
	//	int cY = moment.m01 / moment.m00;
	//	circle(mContours, Point(cX, cY), 10, Scalar(255, 255, 255), 2);
	//	std::stringstream ss;
	//	ss << "cent" << idx;
	//	putText(mContours, ss.str(), Point(cX - 20, cY - 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255));
	//}

	for (size_t idx = 0; idx < vecTuple.size(); idx++) {
		cv::drawContours(mContours, contours, std::get<4>(vecTuple.at(idx)), colors[idx % 3]);
		Point ptCenter = std::get<2>(vecTuple.at(idx));
		circle(mContours, ptCenter, 10, Scalar(255, 255, 255), 2);
		std::stringstream ss;
		ss << "cent" << idx;
		putText(mContours, ss.str(), Point(ptCenter.x - 20, ptCenter.y - 20), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(255, 255, 255));

	}
	imwrite("contours.png", mContours);
#endif

	return true;
}




bool bindImageAndBarCode(std::vector<cv::Mat> & vBags, std::vector<cv::Mat> vm, int nImgStitchDirection, std::vector<cv::Point> vp) {
	Mat m = stitchImage(vm, nImgStitchDirection);
#ifdef SAVE_IMAGE
	imwrite("stitch.png", m);
#endif
//	m = stitchImage(vm, 0);
//#ifdef SAVE_IMAGE
//	imwrite("stitch2.png", m);
//#endif

	bool bRet = splitBags(vBags, m, nImgStitchDirection);
#ifdef SAVE_IMAGE
	for (size_t idx = 0; idx < vBags.size(); idx++) {
		std::stringstream ss;
		ss << "bag" << idx << ".png";
		imwrite(ss.str(), vBags.at(idx));
	}
#endif

	return bRet;
	//return true;
}