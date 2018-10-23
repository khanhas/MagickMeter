#pragma once
#include <locale>
#include <codecvt>
#include <vector>
#include <iomanip>
#include <windows.h>
#include <Strsafe.h>
#include <Shlwapi.h>
#include <filesystem>
#include <algorithm>
#include "Magick++.h"
#include "..\API\RainmeterAPI.h"

#define INVISIBLE Magick::Color("transparent")
#define ONEPIXEL Magick::Image(Magick::Geometry(1,1), INVISIBLE)
#define MagickPI2	1.57079632679489661923132169163975144209858469968755
#define MagickPI	3.14159265358979323846264338327950288419716939937510
#define Magick2PI	6.28318530717958647692528676655900576839433879875020

typedef std::vector<std::wstring> WSVector;
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
} ImgType;

ImgType GetType(std::wstring input) noexcept;
std::vector<Magick::Color> GenColor(Magick::Image img, size_t totalColor);

class ImgContainer
{
public:
    ImgContainer(int _index);
	Magick::Image	img;
	int				index = 0;
	BOOL			isCombined = FALSE;
	BOOL			isIgnored = FALSE;
	size_t			W = 0;
	size_t			H = 0;
	ssize_t			X = 0;
	ssize_t			Y = 0;
    std::vector<Magick::Color> colorList;
};

class Measure
{
public:
    Measure::~Measure();
	void* skin = nullptr;
	void* rm = nullptr;
	std::string outputA;
	std::wstring outputW;
	std::vector<ImgContainer> imgList;

    void Compose();
    BOOL GetImage(std::wstring imageName, BOOL isPush);

	BOOL CreateFromFile(LPCWSTR baseFile, WSVector &config, ImgContainer &out);
	BOOL CreateText(std::wstring text, WSVector &config, ImgContainer &out);
	BOOL CreateShape(ImgType shapeType, LPCWSTR shapeParas, WSVector &config, ImgContainer &out);
	BOOL CreateCombine(LPCWSTR baseImage, WSVector &config, ImgContainer &out);
	BOOL CreateGradient(LPCWSTR gradType, WSVector &config, ImgContainer &out);

	BOOL ParseEffect(ImgContainer &img, std::wstring name, std::wstring para);
	void ParseInternalVariable(std::wstring &rawSetting, const ImgContainer &srcImg) const;
	void ParseExtend(
		WSVector &parentVector,
		std::wstring parentName,
		BOOL isRecursion = FALSE) const;

	void LogError(Magick::Exception error) const noexcept;
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

	double ParseDouble(std::wstring input);
	int ParseInt(std::wstring input);
	BOOL ParseBool(std::wstring input);
    size_t ParseSizeT(std::wstring input);
    ssize_t ParseSSizeT(std::wstring input);
};

namespace Utils
{
	std::string WStringToString(const std::wstring& wstr);
	std::wstring StringToWString(const std::string& str);
	std::vector<std::wstring> SeparateList(
		std::wstring raw,
        LPCWSTR separator,
		int maxElement,
		LPCWSTR defValue = L"0");
	std::vector<std::wstring> SeparateParameter(
		std::wstring raw,
		int maxPara,
        LPCWSTR defValue = L"0");
	Magick::Color ParseColor(std::wstring raw);
	std::wstring ColorToWString(Magick::Color c);
	std::wstring TrimString(std::wstring bloatedString);
	const int NameToIndex(std::wstring name);
	void GetNamePara(
		std::wstring input,
		std::wstring& name,
		std::wstring& para);
    void SetGeometryMode(int raw, Magick::Geometry &out);

	static auto ParseNumber = [](auto var, const WCHAR* value, auto* func) -> decltype(var)
	{
		if (_wcsnicmp(value, L"*", 1) == 0) return var;
		return (decltype(var))func(value);
	};

	static void ParseBool(bool &var, std::wstring value)
	{
		if (_wcsnicmp(value.c_str(), L"*", 1) != 0)
			var = static_cast<bool>(MathParser::ParseBool(value));
	};

	static auto ParseNumber2 = [](const WCHAR* value, auto defVal, auto* func) -> decltype(defVal)
	{
		if (_wcsnicmp(value, L"*", 1) == 0) return defVal;
		return (decltype(defVal))func(value);
	};
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