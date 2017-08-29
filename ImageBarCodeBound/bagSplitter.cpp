#include <sstream>
#include <direct.h>
#include <sys/stat.h>
#include <windows.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "bagSplitter.h"

using namespace cv;
using namespace rapidjson;

#ifdef _DEBUG
#define SAVE_IMAGE
#endif

const char szLabelDirection[] = "direction";
const char szLabelBarcodes[] = "barcodes";
const char szLabelBarcode[] = "barcode";
const char szLabelAbscissa[] = "abscissa";
const char szLabelOrdinate[] = "ordinate";
const char szLabelImages[] = "images";
const char szLabelImageName[] = "imageName";
const char szLabelImagePath[] = "imagePath";
const char szLabelStatus[] = "status";
const char szLabelCutBottomNoise[] = "cutImageBottomNoise";
const char szLabelBagIgnoreWidthLessThan[] = "bagIgnoreWidthLessThan";

const char szDstImgDir[] = "d:/sf";

struct IBCBConf {
	int nImgStitchDirection;
	BOOL bCutBottomNoise;
	int nBagIgnoreWidthLessThan;

	int nReserved;
};

bool DirectoryExists(const char* pzPath) {
	//LOG(INFO) << "DirectoryExists(): " << pzPath;
	if (pzPath == NULL)
		return false;
	size_t n = strlen(pzPath) - 1;
	size_t ind = n;
	while (ind > 0 && (pzPath[ind] == '/' || pzPath[ind] == '\\')) {
		ind--;
	}
	char* pszPathNoSlash;
	char szTemp[_MAX_PATH] = {};
	if (ind < n) {
		strncpy(szTemp, pzPath, ind + 1);
		pszPathNoSlash = szTemp;
	}
	else {
		pszPathNoSlash = (char*)pzPath;
	}
	struct _stat st;
	int nRet = _stat(pszPathNoSlash, &st);
	if (nRet == 0)
		if ((st.st_mode & _S_IFDIR) != 0)
			return true;
	return false;
}

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
	const int nGrayThreshOffset = -6;
	int nGrayThresh = getGrayThresh(mGray);
	Mat mBin;
	GaussianBlur(mGray, mGray, Size(5, 5), 0);
	threshold(mGray, mBin, nGrayThresh + nGrayThreshOffset, 255, CV_THRESH_BINARY);
#ifdef SAVE_IMAGE
	imwrite("bin.png", mBin);
#endif

	int erosion_type = MORPH_RECT;
	int erosion_size = 1;
	Mat element = getStructuringElement(erosion_type,
		Size(2 * erosion_size + 1, 2 * erosion_size + 1),
		Point(erosion_size, erosion_size));
	erode(mBin, mBin, element);
#ifdef SAVE_IMAGE
	imwrite("binErode.png", mBin);
#endif

	std::vector<std::vector<cv::Point> > contours;
	//cv::findContours(mBin.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	//cv::findContours(mBin.clone(), contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
	Mat mBinInv = 255 - mBin;
	cv::findContours(mBinInv, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	const int contourLenThresh = 50;
	const int contourLenThresh2 = 20;
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
		if (min(rc.height, rc.width) < contourLenThresh2)
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

struct PointComparer
{
	bool operator()(const Point & a, const Point & b) const
	{
		return (a.x < b.x) || (a.x == b.x && a.y < b.y);
	}
};

bool parseBarcodeAndImagePathJSON(IBCBConf & conf, std::vector<std::string > & vecImagePath, std::map<Point, std::string, PointComparer> & mapBarcodes, std::string strJSON) {
	Document document;
	//char * writable = new char[strJSON.size() + 1];
	//std::copy(strJSON.begin(), strJSON.end(), writable);
	//writable[strJSON.size()] = '\0'; // don't forget the terminating 0
	//if (document.Parse<0>(writable).HasParseError())
	if (document.Parse<0>(strJSON.c_str()).HasParseError()) {
		return false;
	}

	assert(document.HasMember(szLabelDirection));
	if (!document.HasMember(szLabelDirection))
	{
		return false;
	}
	assert(document[szLabelDirection].IsInt());
	if (! document[szLabelDirection].IsInt())
	{
		return false;
	}
	conf.nImgStitchDirection = document[szLabelDirection].GetInt();

	conf.bCutBottomNoise = 0;
	if (document.HasMember(szLabelCutBottomNoise))
	{
		assert(document[szLabelCutBottomNoise].IsInt());
		if (!document[szLabelCutBottomNoise].IsInt())
		{
			return false;
		}
		conf.bCutBottomNoise = document[szLabelCutBottomNoise].GetInt();
	}

	conf.nBagIgnoreWidthLessThan = 0;
	if (document.HasMember(szLabelBagIgnoreWidthLessThan))
	{
		assert(document[szLabelBagIgnoreWidthLessThan].IsInt());
		if (!document[szLabelBagIgnoreWidthLessThan].IsInt())
		{
			return false;
		}
		conf.nBagIgnoreWidthLessThan = document[szLabelBagIgnoreWidthLessThan].GetInt();
	}

	if (!document.HasMember(szLabelBarcodes))
	{
		return false;
	}
	if (!document[szLabelBarcodes].IsArray())
	{
		return false;
	}
	rapidjson::Value & barcodes = document[szLabelBarcodes];
	for (SizeType i = 0; i < barcodes.Size(); i++) {
		if (!barcodes[i].HasMember(szLabelBarcode))
		{
			return false;
		}
		if (!barcodes[i][szLabelBarcode].IsString())
		{
			return false;
		}
		const char * pszBarcode = barcodes[i][szLabelBarcode].GetString();
		
		if (!barcodes[i].HasMember(szLabelAbscissa))
		{
			return false;
		}
		if (!barcodes[i][szLabelAbscissa].IsInt())
		{
			return false;
		}
		int nX = barcodes[i][szLabelAbscissa].GetInt();

		if (!barcodes[i].HasMember(szLabelOrdinate))
		{
			return false;
		}
		if (!barcodes[i][szLabelOrdinate].IsInt())
		{
			return false;
		}
		int nY = barcodes[i][szLabelOrdinate].GetInt();
		Point pt = Point(nX, nY);

		if (mapBarcodes.count(pt) > 0)
		{
			return false;
		}
		mapBarcodes[pt] = pszBarcode;
	}

	rapidjson::Value & images = document[szLabelImages];
	for (SizeType i = 0; i < images.Size(); i++) {
		if (!images[i].HasMember(szLabelImagePath))
		{
			return false;
		}
		if (!images[i][szLabelImagePath].IsString())
		{
			return false;
		}
		const char * pszImagePath = images[i][szLabelImagePath].GetString();
		
		vecImagePath.push_back(pszImagePath);
	}

	return true;
}

bool splitImageBagAndBindBarCode(std::map<Point, cv::Mat, PointComparer> & mapBag,std::vector<cv::Mat> vm, int nImgStitchDirection, std::vector<cv::Point> vp, int nBagIgnoreWhenWidthLessThan) {
	Mat m = stitchImage(vm, nImgStitchDirection);
	if (m.empty()) {
		return false;
	}
#ifdef SAVE_IMAGE
	imwrite("stitch.png", m);
#endif

	std::vector<cv::Mat> vBags;
	bool bRet = splitBags(vBags, m, nImgStitchDirection);
#ifdef SAVE_IMAGE
	for (size_t idx = 0; idx < vBags.size(); idx++) {
		std::stringstream ss;
		ss << "bag" << idx << ".png";
		imwrite(ss.str(), vBags.at(idx));
	}
#endif
	if (nBagIgnoreWhenWidthLessThan > 0 && vBags.size() > vp.size()) {
		for (int idx = (int)vBags.size() - 1; idx >= 0; idx--) {
			if (vBags.at(idx).cols < nBagIgnoreWhenWidthLessThan) {
				vBags.erase(vBags.begin() + idx);
			}
		}
	}

	if (vBags.size() != vp.size()) {
		for (size_t idx = 0; idx < vp.size(); idx++) {
			mapBag[vp.at(idx)] = m;
		}

		return false;
	}

	std::sort(vp.begin(), vp.end(), [](cv::Point a, cv::Point b) -> bool {return a.x < b.x; });
	for (size_t idx = 0; idx < min(vBags.size(), vp.size()); idx++) {
		mapBag[vp.at(idx)] = vBags.at(idx);
	}

	return bRet;
}

rapidjson::Value CreateResult(rapidjson::Document & document) {
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
	rapidjson::Value root(rapidjson::kObjectType);
	root.AddMember(szLabelStatus, 0, allocator);
	rapidjson::Value imageArray(rapidjson::kArrayType);
	root.AddMember(szLabelBarcodes, imageArray, allocator);

	return root;
}

std::string GetJsonString(rapidjson::Value &root)
{
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	root.Accept(writer);
	std::string reststring = buffer.GetString();
	return reststring;
}

std::string outputBarcodeAndImgPathJSON(std::map<std::string, std::pair<std::string, std::string>> & mapBarcodeAndImgPath) {
	rapidjson::Document document;
	rapidjson::Value root = CreateResult(document);
	rapidjson::Value & barcodeArray = root[szLabelBarcodes];
	rapidjson::Document::AllocatorType & allocator = document.GetAllocator();
	for (std::map<std::string, std::pair<std::string, std::string>>::iterator iter = mapBarcodeAndImgPath.begin(); iter != mapBarcodeAndImgPath.end(); ++iter)
	{
		rapidjson::Value object(rapidjson::kObjectType);
		rapidjson::Value vBarcode(kStringType);
		std::string strBarcode = iter->first;
		vBarcode.SetString(strBarcode.c_str(), allocator);
		object.AddMember(szLabelBarcode, vBarcode, allocator);

		rapidjson::Value vImageTitle(kStringType);
		vImageTitle.SetString(iter->second.first.c_str(), allocator);
		object.AddMember(szLabelImageName, vImageTitle, allocator);
		rapidjson::Value vImagePath(kStringType);
		vImagePath.SetString(iter->second.second.c_str(), allocator);
		object.AddMember(szLabelImagePath, vImagePath, allocator);

		barcodeArray.PushBack(object, allocator);
	}

	return GetJsonString(root);
}

int freeJSONMemory(char ** ppszJSONOut)
{
	if (NULL == ppszJSONOut)
	{
		return -1;
	}
	delete[] ppszJSONOut[0];

	return 0;
}


bool findRowForeground(Mat & mOrg, int row, int colStart, int colEnd)
{
	Mat mGray;
	cvtColor(mOrg, mGray, CV_BGR2GRAY);
#ifdef SAVE_IMAGE
	imwrite("gray.png", mGray);
#endif

	int objThickThresh = 10;
	int nObjLen = 0;
	bool bForeground = false;
	for (int i = colStart; i < colEnd; i++) {
		if (mGray.at<unsigned char>(row, i) < 200) {
			nObjLen++;
			if (nObjLen >= objThickThresh) {
				bForeground = true;
				break;
			}
		}
		else {
			nObjLen = 0;
		}
	}

	return bForeground;
}

// to remove the bottom garbage.
cv::Mat cutBottomEdge(cv::Mat mOrg) {
	int nGrayThresh = 10;
	int nBagMinWidthThresh = 10;

	cv::Mat m = mOrg(Rect(mOrg.cols / 3, 0, mOrg.cols / 3, mOrg.rows));
	int rowEnd = m.rows - 1;
	bool bForeground = false;
	for (int i = 0; i < m.cols; i ++)
	{
		if (m.at<Vec3b>(rowEnd, i) != Vec3b(255, 255, 255))
		{
			bForeground = true;
			break;
		}
	}

	if (bForeground)
	{
		bForeground = false;
		while (rowEnd > m.rows - 10 && !findRowForeground(m, rowEnd - 1, 0, m.cols)) {
			rowEnd--;
		}
		if (rowEnd == m.rows - 10)
		{
			bForeground = false;
		}
		return bForeground ? mOrg : mOrg(Rect(0, 0, mOrg.cols, mOrg.rows - 60));
	}

	return mOrg;
}

int bindImageAndBarCode(const char * pszJSONIn, char ** ppszJSONOut) {
	if (NULL == pszJSONIn)
	{
		return -16;
	}
	if (NULL == ppszJSONOut)
	{
		return -17;
	}

	IBCBConf conf = {};
	std::vector<std::string > vecImagePath;
	std::map<Point, std::string, PointComparer> mapBarcodes;
	if (!parseBarcodeAndImagePathJSON(conf, vecImagePath, mapBarcodes, pszJSONIn))
	{
		return -1;
	}
	std::vector<cv::Mat> vm;
	for (size_t idx = 0; idx < vecImagePath.size(); idx++)
	{
		cv::Mat m = imread(vecImagePath.at(idx));
		if (m.empty())
		{
			return -2;
		}

		if (conf.bCutBottomNoise) {
			m = cutBottomEdge(m);
		}
		vm.push_back(m);
	}

	std::vector<cv::Point> vp;
	for (std::map<Point, std::string, PointComparer>::iterator iter = mapBarcodes.begin(); iter != mapBarcodes.end(); ++iter)
	{
		vp.push_back(iter->first);
	}
	std::map<Point, cv::Mat, PointComparer> mapBag;
	bool bRet = splitImageBagAndBindBarCode(mapBag, vm, conf.nImgStitchDirection, vp, conf.nBagIgnoreWidthLessThan);
	std::map<std::string, std::pair<std::string, std::string>> mapBarcodeAndImgPath;
	_mkdir(szDstImgDir);
	if (! DirectoryExists(szDstImgDir))
	{
		return -3;
	}

	if (!bRet)
	{
		if (0 == vecImagePath.size())
		{
			return -10;
		}
		else if (1 == vecImagePath.size())
		{
			std::string imgFullPath = vecImagePath.at(0);
			std::string imgName = imgFullPath.substr(imgFullPath.find_last_of("/\\") + 1);

			for (std::map<Point, cv::Mat, PointComparer>::iterator iter = mapBag.begin(); iter != mapBag.end(); ++iter)
			{
				Point pt = iter->first;
				std::string strBarcode = mapBarcodes[pt];
				mapBarcodeAndImgPath[strBarcode] = std::pair<std::string, std::string>(imgName, vecImagePath.at(0));
			}
		} 
		else
		{
			SYSTEMTIME	curr_tm = {};
			GetLocalTime(&curr_tm);
			char szDate[_MAX_PATH] = {};
			sprintf_s(szDate, _MAX_PATH, "%d-%02d-%02d", curr_tm.wYear, curr_tm.wMonth, curr_tm.wDay);

			char szImgName[_MAX_PATH] = {};
			sprintf_s(szImgName, _MAX_PATH, "sf_%d-%02d-%02d_%02d%02d%02d%03d_%d.png", curr_tm.wYear, curr_tm.wMonth, curr_tm.wDay, curr_tm.wHour, curr_tm.wMinute, curr_tm.wSecond, curr_tm.wMilliseconds, rand());
			char szImgFullPath[256] = {};
			sprintf_s(szImgFullPath, 256, "%s\\%s", szDstImgDir, szDate);
			_mkdir(szImgFullPath);
			if (!DirectoryExists(szDstImgDir))
			{
				return -4;
			}
			sprintf_s(szImgFullPath, 256, "%s\\%s\\%s", szDstImgDir, szDate, szImgName);
			cv::Mat m = mapBag.begin()->second;
			if (!imwrite(szImgFullPath, m))
			{
				return -5;
			}

			for (std::map<Point, cv::Mat, PointComparer>::iterator iter = mapBag.begin(); iter != mapBag.end(); ++iter)
			{
				Point pt = iter->first;
				std::string strBarcode = mapBarcodes[pt];
				mapBarcodeAndImgPath[strBarcode] = std::pair<std::string, std::string>(szImgName, szImgFullPath);
			}
		}
	}
	else
	{
		for (std::map<Point, cv::Mat, PointComparer>::iterator iter = mapBag.begin(); iter != mapBag.end(); ++iter)
		{
			Point pt = iter->first;
			std::string strBarcode = mapBarcodes[pt];


			SYSTEMTIME	curr_tm = {};
			GetLocalTime(&curr_tm);
			char szDate[_MAX_PATH] = {};
			sprintf_s(szDate, _MAX_PATH, "%d-%02d-%02d", curr_tm.wYear, curr_tm.wMonth, curr_tm.wDay);

			char szImgName[_MAX_PATH] = {};
			sprintf_s(szImgName, _MAX_PATH, "sf_%d-%02d-%02d_%02d%02d%02d%03d_%d.png", curr_tm.wYear, curr_tm.wMonth, curr_tm.wDay, curr_tm.wHour, curr_tm.wMinute, curr_tm.wSecond, curr_tm.wMilliseconds, rand());
			char szImgFullPath[256] = {};
			sprintf_s(szImgFullPath, 256, "%s\\%s", szDstImgDir, szDate);
			_mkdir(szImgFullPath);
			if (!DirectoryExists(szDstImgDir))
			{
				return -4;
			}
			sprintf_s(szImgFullPath, 256, "%s\\%s\\%s", szDstImgDir, szDate, szImgName);
			cv::Mat m = iter->second;
			if (!imwrite(szImgFullPath, m))
			{
				return -5;
			}

			mapBarcodeAndImgPath[strBarcode] = std::pair<std::string, std::string>(szImgName, szImgFullPath);
		}
	}

	std::string strJSONOut = outputBarcodeAndImgPathJSON(mapBarcodeAndImgPath);
	ppszJSONOut[0] = new char [strJSONOut.length() + 1]();
	memcpy(ppszJSONOut[0], strJSONOut.c_str(), strJSONOut.length());

	return 0;
}