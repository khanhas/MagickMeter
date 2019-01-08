#include "MagickMeter.h"
#include <sstream>
#include <wctype.h>

bool Utils::IsEqual(std::wstring a, std::wstring b)
{
    if (a.length() == b.length())
    {
        return std::equal(
            a.begin(),
            a.end(),
            b.begin(),
            [](wchar_t c, wchar_t d) noexcept -> bool {
                return towupper(c) == towupper(d);
            }
        );
    }

    return false;
}

Config Utils::GetNamePara(std::wstring input)
{
	const size_t firstSpace = input.find_first_of(L" \t\r\n");
	if (firstSpace == std::wstring::npos)
	{
        return Config{
            input,
            L"",
            TRUE
        };
	}

    return Config{
        input.substr(0, firstSpace),
        input.substr(input.find_first_not_of(L" \t\r\n", firstSpace + 1)),
        FALSE
    };
}

const int Utils::NameToIndex(std::wstring name)
{
    if (_wcsnicmp(name.c_str(), L"IMAGE", 5) == 0)
	{
		if (name.length() == 5)
			return 0;

		const int index = _wtoi(name.substr(5).c_str()) - 1;
		if (index >= 0)
			return index;
	}

	return -1;
}

std::wstring Utils::TrimString(std::wstring bloatedString)
{
	const size_t start = bloatedString.find_first_not_of(L" \t\r\n", 0);
	if (start != std::wstring::npos)
	{
		std::wstring trimmedString = bloatedString.substr(start, bloatedString.length());
		trimmedString.erase(trimmedString.find_last_not_of(L" \t\r\n") + 1);
		return trimmedString;
	}
	return L"";
}

std::vector<std::wstring> Utils::SeparateList(
	std::wstring rawString,
    LPCWSTR separator,
    int minElementCount,
    LPCWSTR defValue)
{
	std::vector<std::wstring> vectorList;

	if (rawString.empty())
		return vectorList;

	size_t start = 0;
	size_t end = rawString.find(separator);

	while (end != std::wstring::npos)
	{
        const auto prevChar = rawString.at(end - 1);
        // Ignore escaped separator
        if (prevChar == L'\\') {
            rawString.erase(end - 1, 1);
            end = rawString.find(separator, end);
            continue;
        }

        auto element = rawString.substr(start, end - start);
        element = TrimString(element);

        if (!element.empty())
        {
            vectorList.push_back(element);
        }

		start = end + 1;
		end = rawString.find(separator, start);
	}

	if (start < rawString.length())
	{
		auto element = rawString.substr(start, rawString.length() - start);
		element = TrimString(element);

		if (!element.empty())
		{
			vectorList.push_back(element);
		}
	}

	if (minElementCount)
	{
		while (vectorList.size() < minElementCount)
		{
			vectorList.push_back(defValue);
		}
	}

	return vectorList;
}

std::vector<std::wstring> Utils::SeparateParameter(
	std::wstring raw,
	int minParaCount,
    LPCWSTR defValue)
{
	std::vector<std::wstring> resultList;
	size_t s = 0;
	size_t e = raw.find(L",");

	while (e != std::wstring::npos)
	{
		std::wstring t = raw.substr(s, e - s);
		LPCWSTR tP = t.c_str();
        int b = 0;
        if (tP != nullptr)
        {
            while (*tP)
            {
                if (*tP == L'(')
                    ++b;
                else if (*tP == L')')
                    --b;
                ++tP;
            }
        }

        if (b != 0)
        {
            e = raw.find(L",", e + 1);
        }
		else
		{
			resultList.push_back(t);
			s = e + 1;
			e = raw.find(L",", s);
		}
	}
	if (s != raw.length())
		resultList.push_back(raw.substr(s, raw.length() - s));

	while (resultList.size() < minParaCount)
		resultList.push_back(defValue);

	return resultList;
}

std::vector<Config> Utils::ParseConfig(const std::wstring raw)
{
    WSVector rawConfig = Utils::SeparateList(raw, L"|", NULL);
    std::vector<Config> config;
    std::transform(
        rawConfig.begin(),
        rawConfig.end(),
        std::back_inserter(config),
        Utils::GetNamePara
    );

    return config;
}

Magick::Color Utils::ParseColor(std::wstring raw)
{
	if (raw.empty())
		return Magick::Color("black");

	Magick::Color result;
	if (_wcsnicmp(raw.c_str(), L"HSL(", 4) == 0)
	{
		const size_t end = raw.find_last_of(L")");

		if (end != std::wstring::npos)
		{
			raw = raw.substr(4, end - 4);
			WSVector hsl = SeparateList(raw, L",", 3, L"0");
			Magick::ColorRGB temp = Magick::ColorHSL(
				MathParser::ParseDouble(hsl[0]) / 360,
				MathParser::ParseDouble(hsl[1]) / 100,
				MathParser::ParseDouble(hsl[2]) / 100
			);

			if (hsl.size() > 3 && !hsl[3].empty())
				temp.alpha(MathParser::ParseDouble(hsl[3]) / 100.0);

			if (temp.isValid()) result = temp;
		}
	}
	else
	{
		std::vector<double> rgba;
		size_t start = 0;
		const size_t end = raw.find(L",");

		if (end == std::wstring::npos) //RRGGBBAA
		{
			if (raw.find_first_not_of(L"0123456789ABCDEFabcdef") == std::wstring::npos) //Find non hex character
			{
				size_t remain = raw.length();
				while (remain >= 2)
				{
					int val = std::stoi(raw.substr(start, 2), nullptr, 16);
					rgba.push_back((double)val / 255);
					start += 2;
					remain -= 2;
				}
			}
		}
		else //R,G,B,A
		{
			for (auto &color : SeparateList(raw, L",", NULL))
			{
				rgba.push_back(MathParser::ParseDouble(color) / 255);
			}
		}
		if (rgba.size() == 3)
			result = Magick::ColorRGB(rgba[0], rgba[1], rgba[2]);
		else if (rgba.size() > 3)
			result = Magick::ColorRGB(rgba[0], rgba[1], rgba[2], rgba[3]);
	}

	if (result.isValid())
		return result;

	return Magick::Color("black");
}

std::wstring Utils::ColorToWString(Magick::Color c)
{
	Magick::ColorRGB rgb = c;
	std::wostringstream s;
    s << static_cast<int>(round(rgb.red() * 255)) << L","
        << static_cast<int>(round(rgb.green() * 255)) << L","
        << static_cast<int>(round(rgb.blue() * 255));
	return s.str();
}

std::wstring Utils::StringToWString(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string Utils::WStringToString(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

void Utils::SetGeometryMode(int raw, Magick::Geometry &out)
{
    switch (raw)
    {
    case 1:
        out.aspect(true);
        break;
    case 2:
        out.fillArea(true);
        break;
    case 3:
        out.greater(true);
        break;
    case 4:
        out.less(true);
        break;
    }
}