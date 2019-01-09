#pragma once
#include <windows.h>
#include <locale>
#include <codecvt>
#include <vector>
#include <filesystem>
#include <algorithm>
#include "Magick++.h"
#include "..\API\RainmeterAPI.h"

#define INVISIBLE Magick::Color("transparent")
#define ONEPIXEL Magick::Image(Magick::Geometry(1,1), INVISIBLE)
constexpr auto MagickPI2 = 1.57079632679489661923132169163975144209858469968755;
constexpr auto MagickPI  = 3.14159265358979323846264338327950288419716939937510;
constexpr auto Magick2PI = 6.28318530717958647692528676655900576839433879875020;

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
void GenColor(Magick::Image img, size_t totalColor, std::vector<Magick::Color> &outContainer);

class Config {
public:
    std::wstring name;
    std::wstring para;
    BOOL isApplied;
    // Match name
    BOOL Match(LPCWSTR input);
    // Match para
    BOOL Equal(LPCWSTR input);
    WSVector ToList(int minItems = 0, LPCWSTR defVal = L"0");
    Magick::Color ToColor();
    BOOL ToBool();
    std::string ToString();
    double ToDouble();
    int ToInt();
    size_t ToSizeT();
    ssize_t ToSSizeT();
};

class ImgContainer
{
public:
    ImgContainer(INT _index, std::vector<Config> _config) :
        index(_index),
        config(_config),
        img(ONEPIXEL)
    {};

	Magick::Image	    img;
    std::vector<Config> config;
	INT			        index;
	BOOL			    isCombined = FALSE;
	BOOL			    isIgnored = FALSE;
    Magick::Geometry    geometry;
    std::vector<Magick::Color> colorList;
};

class Measure
{
public:
    Measure::~Measure();
    static int refCount;
	void* skin = nullptr;
	void* rm = nullptr;
	std::string outputA;
	std::wstring outputW;
	std::vector<std::shared_ptr<ImgContainer>> imgList;

    void Compose();
    BOOL Measure::GetImage(std::shared_ptr<ImgContainer> curImg);

	BOOL CreateFromFile(std::shared_ptr<ImgContainer> out);
	BOOL CreateText(std::shared_ptr<ImgContainer> out);
	BOOL CreateShape(ImgType shapeType, std::shared_ptr<ImgContainer> out);
	BOOL CreateCombine(std::shared_ptr<ImgContainer> out);
	BOOL CreateGradient(std::shared_ptr<ImgContainer> out);
    BOOL CreateConicalGradient(std::shared_ptr<ImgContainer> out);

	BOOL ApplyEffect(std::shared_ptr<ImgContainer> img, Config &option);
	void ReplaceInternalVariable(std::wstring &rawSetting, std::shared_ptr<ImgContainer> srcImg);
	void InsertExtend(
		std::vector<Config> &parentVector,
		std::wstring parentName,
		BOOL isRecursion = FALSE) const;

	void LogError(Magick::Exception error) const noexcept;
};

namespace MathParser
{
	double ParseDouble(std::wstring input);
	int ParseInt(std::wstring input);
	BOOL ParseBool(std::wstring input);
    size_t ParseSizeT(std::wstring input);
    ssize_t ParseSSizeT(std::wstring input);
};

namespace Utils
{
    bool IsEqual(std::wstring a, std::wstring b);
	std::string WStringToString(const std::wstring& wstr);
	std::wstring StringToWString(const std::string& str);
	std::vector<std::wstring> SeparateList(
		std::wstring raw,
        LPCWSTR separator,
		int minElementCount,
		LPCWSTR defValue = L"0");
	std::vector<std::wstring> SeparateParameter(
		std::wstring raw,
		int minParaCount,
        LPCWSTR defValue = L"0");
	Magick::Color ParseColor(std::wstring raw);
	std::wstring ColorToWString(Magick::Color c);
	std::wstring TrimString(std::wstring bloatedString);
	const int NameToIndex(std::wstring name);
	Config GetNamePara(std::wstring input);
    std::vector<Config> ParseConfig(std::wstring raw);
    void SetGeometryMode(int raw, Magick::Geometry &out);

	static auto ParseNumber = [](auto var, const WCHAR* value, auto* func) -> decltype(var)
	{
		if (_wcsnicmp(value, L"*", 1) == 0) return var;
		return static_cast<decltype(var)>(func(value));
	};

	static void ParseBool(bool &var, std::wstring value)
	{
		if (_wcsnicmp(value.c_str(), L"*", 1) != 0)
			var = MathParser::ParseBool(value) ? true : false;
	};

	static auto ParseNumber2 = [](const WCHAR* value, auto defVal, auto* func) -> decltype(defVal)
	{
		if (_wcsnicmp(value, L"*", 1) == 0) return defVal;
		return static_cast<decltype(defVal)>(func(value));
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