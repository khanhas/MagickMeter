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

struct Measure
{
	void* skin;
	void* rm;
	std::wstring name;
	std::wstring outputFile;
	BOOL isGIF = FALSE;
	std::vector<Magick::Image> gifList;
	int GIFSeq = 0;
};

std::string ws2s(const std::wstring& wstr);
std::wstring s2ws(const std::string& str);
std::vector<std::wstring> SeparateList(LPCWSTR rawString, wchar_t* separtor, int maxElement, wchar_t* defValue = L"0");
Magick::ColorRGB GetColor(LPCWSTR rawString);
std::wstring TrimString(std::wstring bloatedString);
int NameToIndex(std::wstring name);
BOOL ParseEffect(void * rm, Magick::Image &img, std::wstring name, std::wstring para);
void GetNamePara(std::wstring input, std::wstring& name, std::wstring& para);
std::wstring error2pws(std::exception &error);

typedef enum 
{
	NOTYPE,
	NORMAL,
	TEXT,
	ELLIPSE,
	RECTANGLE,
	PATH,
	LINE,
	CURVE,
	ARC,		//possibly
	COMBINE,
	REFERENCE
} ImgType ;

struct ImgStruct
{
	Magick::Image	contain;
	ImgType			type		= NOTYPE;
	BOOL			isDelete	= FALSE;
};

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