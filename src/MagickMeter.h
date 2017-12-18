#pragma once
#include <locale>
#include <codecvt>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <windows.h>
#include <Strsafe.h>
#include <Shlwapi.h>
#include <filesystem>
#include "Magick++.h"
#include "..\..\API\RainmeterAPI.h"

typedef std::vector<std::wstring> WSVector;
#define INVISIBLE Magick::Color("transparent")
#define ONEPIXEL Magick::Image(Magick::Geometry(1,1), INVISIBLE)

struct ImgStruct
{
	Magick::Image	contain = ONEPIXEL;
	int				index;
	BOOL			isDelete = FALSE;
	BOOL			isIgnore = FALSE;
	size_t			W = 0;
	size_t			H = 0;
	ssize_t			X = 0;
	ssize_t			Y = 0;
	std::vector<Magick::Color> colorList;
};

struct Measure
{
	void* skin;
	void* rm;
	std::string outputA;
	std::wstring outputW;
	std::vector<ImgStruct> imgList;
	Magick::Image finalImg;
};

std::string ws2s(const std::wstring& wstr);
std::wstring s2ws(const std::string& str);
WSVector SeparateList(std::wstring rawString, wchar_t* separator, int maxElement, wchar_t* defValue = L"0");
WSVector SeparateParameter(std::wstring rawPara, int maxPara, std::wstring defValue = L"0");
Magick::Color GetColor(std::wstring rawString);
std::wstring TrimString(std::wstring bloatedString);
int NameToIndex(std::wstring name);
void GetNamePara(std::wstring input, std::wstring& name, std::wstring& para);
void error2pws(Magick::Exception error);
void ParseExtend(void * rm, WSVector &parentVector, std::wstring parentName, BOOL isRecursion = FALSE);
void ParseInternalVariable(Measure * measure, std::wstring &rawSetting, ImgStruct &srcImg);

typedef enum 
{
	NOTYPE,
	NORMAL,
	TEXT,
	ELLIPSE,
	RECTANGLE,
	POLYGON,
	PATH,
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

	double ParseF(std::wstring input);
	int ParseI(std::wstring input);
	BOOL ParseB(std::wstring input);
};

#if (MAGICKCORE_QUANTUM_DEPTH == 8)
#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef MagickFloatType Quantum;
#define QuantumRange  255.0
#else
typedef unsigned char Quantum;
#define QuantumRange  ((Quantum) 255)
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 16)
#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef MagickFloatType Quantum;
#define QuantumRange  65535.0f
#else
typedef unsigned short Quantum;
#define QuantumRange  ((Quantum) 65535)
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 32)
#if defined(MAGICKCORE_HDRI_SUPPORT)
typedef MagickDoubleType Quantum;
#define QuantumRange  4294967295.0
#else
typedef unsigned int Quantum;
#define QuantumRange  ((Quantum) 4294967295)
#endif
#elif (MAGICKCORE_QUANTUM_DEPTH == 64)
#define MAGICKCORE_HDRI_SUPPORT  1
typedef MagickDoubleType Quantum;
#define QuantumRange  18446744073709551615.0
#else
#error "MAGICKCORE_QUANTUM_DEPTH must be one of 8, 16, 32, or 64"
#endif