#include <stdlib.h>
#include <direct.h>
#include <windows.h>
#include "opencv2/opencv.hpp"
#include "bagSplitter.h"

using namespace std;

bool IsImageFileExt(const char * pszExt)
{
	if (_stricmp(pszExt, "jpg") == 0 ||
		_stricmp(pszExt, "jpeg") == 0 ||
		_stricmp(pszExt, "png") == 0 ||
		_stricmp(pszExt, "bmp") == 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#ifdef _WIN32

void SearchImageFiles(vector<string> & fileList, const char * pszImagePath, bool bFullPath = false) {
	//LOG_FUNCTION;
	if (NULL == pszImagePath)
	{
		return;
	}
	char szSearchPath[_MAX_PATH];
	strcpy(szSearchPath, pszImagePath);

	strcat(szSearchPath, "/*.*");

	HANDLE hFind;
	WIN32_FIND_DATAA ffd;

	hFind = FindFirstFileA(szSearchPath, &ffd);
	DWORD errorcode = 0;
	if (hFind == INVALID_HANDLE_VALUE) {
		printf("Can not find any file\n");
		return;
	}

	while (true)
	{
		if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			int nLen = strlen(ffd.cFileName);
			if (nLen >= 5)
			{
				char * pszExt = strrchr(ffd.cFileName, '.');

				if (pszExt != NULL && IsImageFileExt(pszExt + 1))
				{
					if (bFullPath)
					{
						string strFile = pszImagePath;
						strFile += "/";
						strFile += ffd.cFileName;
						fileList.push_back(strFile);
					}
					else
					{
						fileList.push_back(string(ffd.cFileName));
					}
				}
			}
		}

		if (FindNextFileA(hFind, &ffd) == FALSE)
			break;
	}

	FindClose(hFind);
}
#else
void SearchImageFiles(vector<string> & fileList, const char * pszImagePath, bool bFullPath = false)
{
	LOG_FUNCTION;
	char szSearchPath[_MAX_PATH];
	strcpy(szSearchPath, pszImagePath);

	DIR *dir = opendir(szSearchPath);
	if (dir == NULL)  {
		printf("Can not find any file\n");
		return;
	}

	while (true)
	{
		struct dirent *s_dir = readdir(dir);
		if (s_dir == NULL)
			break;

		if ((strcmp(s_dir->d_name, ".") == 0) || (strcmp(s_dir->d_name, "..") == 0))
			continue;

		char currfile[_MAX_PATH];
		sprintf(currfile, "%s/%s", szSearchPath, s_dir->d_name);
		struct stat file_stat;
		stat(currfile, &file_stat);
		if (!S_ISDIR(file_stat.st_mode))
		{
			int nLen = strlen(s_dir->d_name);
			if (nLen >= 5)
			{
				char * pszExt = strrchr(s_dir->d_name, '.');

				if (pszExt != NULL && IsImageFileExt(pszExt + 1))
				{
					if (bFullPath)
					{
						string strFile = pszImagePath;
						strFile += "/";
						strFile += s_dir->d_name;
						fileList.push_back(strFile);
					}
					else
					{
						fileList.push_back(string(s_dir->d_name));
					}

					printf("%s %s\n", currfile, s_dir->d_name);
				}
			}
		}
	}

	closedir(dir);
}
#endif

string replacePathSeparator(const char * pszFilePath)
{
	string strFile = pszFilePath;
	std::replace(strFile.begin(), strFile.end(), '/', '_');
	std::replace(strFile.begin(), strFile.end(), '\\', '_');
	std::replace(strFile.begin(), strFile.end(), ':', '_');
	return strFile;
}

// nImgBagDirection:
// 0: move to right from left, which means the right bag is ahead of the left bag.
void batchProcess(const char * pszImageDir, const char * pszDstImageDir, int nImgBagDirection)
{
	_mkdir(pszDstImageDir);

	vector<string> fileList;
	SearchImageFiles(fileList, pszImageDir, true);
	for (vector<string>::iterator it = fileList.begin(); it != fileList.end(); it++) {
		printf(">>> processing %s\n", it->c_str());
		cv::Mat m = cv::imread(*it);
		//m = m(cv::Rect(0, 0, 1730, 1048));

		std::vector<cv::Mat> vm;
		vm.push_back(m);
		std::vector<cv::Point> vp;
		std::vector<cv::Mat> vBags;
		bool bRet = bindImageAndBarCode(vBags, vm, nImgBagDirection, vp);
		if (! bRet)
		{
			continue;
		}

		string strFile = replacePathSeparator(it->c_str());
		strFile = "/" + strFile;
		strFile = pszDstImageDir + strFile;
		for (size_t idx = 0; idx < vBags.size(); idx++) {
			std::stringstream ss;
			ss << strFile <<"-bag" << idx << ".png";
			imwrite(ss.str(), vBags.at(idx));
		}
	}
}

enum BagOrder
{
	RightAhead,
	BottmAhead,
	LeftAhead,
	TopAhead,
};

int main(int argc, char * argv[]) {
	if (argc > 1)
	{
		batchProcess(argv[1], "./bags", RightAhead);
	}
	std::vector<cv::Mat> vm;
	cv::Mat m = cv::imread("D:\\SinoCloud\\ImageBarCodeBound\\tsw_2016-12-08_072024929_23655.jpg");
	vm.push_back(m);
	//cv::Mat m1 = cv::imread("D:\\SinoCloud\\ImageBarCodeBound\\tsw_2016-12-08_071701298_17421.jpg");
	//vm.push_back(m1);
	//cv::Mat m2 = cv::imread("D:\\SinoCloud\\ImageBarCodeBound\\tsw_2016-12-08_071751416_25547.jpg");
	//vm.push_back(m2);

	cv::Point;
	std::vector<cv::Point> vp;
	std::vector<cv::Mat> vBags;
	//bool bRet = bindImageAndBarCode(vBags, vm, 2, vp);


	const char * pszBarLogFile = "../barcode_res.log";
	//const char * pcszImageDir = "D:\\SinoCloud\\ImageBarCodeBound\\";
	//const char * pcszImageDir = "F:/images/scene";
	const char * pcszImageDir = "F:/images/À„∑®≤‚ ‘Õº";


	//const char * pszImageDir = argv[1];
	const char * pszImageDir = pcszImageDir;
	const char * pszDstImageDir = "../res";
	batchProcess(pszImageDir, pszDstImageDir, RightAhead);
	printf("job finished.");
}
