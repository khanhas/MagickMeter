#pragma once
#include <locale>
#include <codecvt>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <Strsafe.h>
#include <Shlwapi.h>
#include "Magick++.h"
#include "..\..\API\RainmeterAPI.h"

typedef std::vector<std::wstring> WSVector;
#define INVISIBLE Magick::Color("transparent")

struct ImgStruct
{
	Magick::Image	contain;
	BOOL			isDelete = FALSE;
	int				width = contain.columns();
	int				height = contain.rows();
};

struct Measure
{
	void* skin;
	void* rm;
	std::wstring name;
	std::wstring outputFile;
	std::vector<ImgStruct *> imgList;
	Magick::Image finalImg;
	BOOL isGIF = FALSE;
	std::vector<Magick::Image> gifList;
	int GIFSeq = 0;
	/*HWND magickHwnd;
	WNDCLASS winClass = { 0 };
	HBITMAP thisBM;*/
};

std::string ws2s(const std::wstring& wstr);
std::wstring s2ws(const std::string& str);
WSVector SeparateList(std::wstring rawString, wchar_t* separtor, int maxElement, wchar_t* defValue = L"0");
Magick::Color GetColor(std::wstring rawString);
std::wstring TrimString(std::wstring bloatedString);
int NameToIndex(std::wstring name);
BOOL ParseEffect(void * rm, Magick::Image &img, std::wstring name, std::wstring para);
void GetNamePara(std::wstring input, std::wstring& name, std::wstring& para);
void error2pws(Magick::Exception error);
void ParseExtend(void * rm, WSVector &parentVector, std::wstring parentName, BOOL isRecursion = FALSE);

typedef enum 
{
	NOTYPE,
	NORMAL,
	TEXT,
	ELLIPSE,
	RECTANGLE,
	PATH,		//possibly
	LINE,		//possibly
	CURVE,		//possibly
	ARC,		//possibly
	COMBINE,
	CLONE,
	GRADIENT
} ImgType ;

namespace MathParser
{
	typedef bool(*GetValueFunc)(const WCHAR* str, int len, double* value, void* context);

	const WCHAR* Check(const WCHAR* formula);
	const WCHAR* CheckedParse(std::wstring input, double* result);
	const WCHAR* Parse(
		const WCHAR* formula, double* result,
		GetValueFunc getValue = nullptr, void* getValueContext = nullptr);

	bool IsDelimiter(WCHAR ch);

	int ParseI(std::wstring input);
	double ParseF(std::wstring input);
};