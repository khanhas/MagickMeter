#include "MagickMeter.h"
#include <filesystem>

BOOL CreateText(ImgStruct * dst, std::vector<std::wstring> setting, Measure * measure)
{
	std::wstring text = setting[0].substr(setting[0].find_first_not_of(L" \t\r\n", 4));
	setting.erase(setting.begin());

	double xPos = 0;
	double yPos = 0;
	size_t width = 600;
	size_t height = 600;
	std::vector<Magick::Drawable> drawList;

	BOOL isSysFont = TRUE;
	int fontWeight = 400;
	std::string fontFace = "Arial";
	MagickCore::StyleType fontStyle = MagickCore::NormalStyle;
	MagickCore::StretchType fontStretch = MagickCore::NormalStretch;
	std::string fontPath = "";
	drawList.push_back(Magick::DrawableGravity(MagickCore::NorthWestGravity));
	for (int i = setting.size() - 1; i >= 0; i--)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(setting[i], tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();
		
		BOOL isSetting = FALSE;
		if (_wcsicmp(name, L"CANVAS") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L",", 2);
			int tempW = MathParser::ParseI(valList[0]);
			int tempH = MathParser::ParseI(valList[1]);

			if (tempW <= 0 || tempH <= 0)
				RmLog(2, L"Invalid Width or Height value. Default canvas 600x600 is used.");
			else
			{
				width = tempW;
				height = tempH;
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"MOVE") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L",", 2);
			if (valList.size() > 0)
			{
				xPos = MathParser::ParseF(valList[0]);
				yPos = MathParser::ParseF(valList[1]);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ANTIALIAS") == 0)
		{
			drawList.push_back(Magick::DrawableTextAntialias(
				MathParser::ParseI(parameter) == 1
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLOR") == 0)
		{
			drawList.push_back(Magick::DrawableFillColor(
				GetColor(parameter)
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"FACE") == 0)
		{
			std::wstring para = parameter;
			if (para.find(L"@") == 0) //Font file in @Resource folder
			{
				isSysFont = FALSE;
				std::wstring fontDir = RmReplaceVariables(measure->rm, L"#@#");
				fontDir += L"Fonts\\" + para.substr(1);
				if (std::experimental::filesystem::exists(fontDir))
					fontFace = ws2s(fontDir);
			}
			else if (std::experimental::filesystem::exists(parameter)) //Direct path to font file
			{
				isSysFont = FALSE;
				fontFace = ws2s(parameter);
			}
			else //Installed font family name
				fontFace = ws2s(parameter);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"SIZE") == 0)
		{
			drawList.push_back(Magick::DrawablePointSize(
					MathParser::ParseF(parameter) * 100 / 75			//Rainmeter font size = 75% Magick font size
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"WEIGHT") == 0)
		{
			fontWeight = MathParser::ParseI(parameter);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STRETCH") == 0)
		{
			fontStretch = (Magick::StretchType)MathParser::ParseI(parameter);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STYLE") == 0)
		{
			if (_wcsicmp(parameter, L"NORMAL") == 0)
				fontStyle = Magick::NormalStyle;

			else if (_wcsicmp(parameter, L"ITALIC") == 0)
				fontStyle = Magick::ItalicStyle;

			else if (_wcsicmp(parameter, L"OBLIQUE") == 0)
				fontStyle = Magick::ObliqueStyle;
			else
			{
				RmLog(2, L"Invalid Text Style. Normal style is applied.");
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ANCHOR") == 0)
		{
			Magick::DrawableGravity align = MagickCore::NorthWestGravity;
			if (_wcsnicmp(parameter, L"LEFTBOTTOM", 10) == 0)
				align = MagickCore::SouthWestGravity;
			else if (_wcsnicmp(parameter, L"LEFTCENTER", 10) == 0)
				align = MagickCore::WestGravity;
			else if (_wcsnicmp(parameter, L"RIGHTBOTTOM", 11) == 0)
				align = MagickCore::SouthEastGravity;
			else if (_wcsnicmp(parameter, L"RIGHTCENTER", 11) == 0)
				align = MagickCore::EastGravity;
			else if (_wcsnicmp(parameter, L"CENTERBOTTOM", 12) == 0)
				align = MagickCore::SouthGravity;
			else if (_wcsnicmp(parameter, L"CENTERCENTER", 12) == 0)
				align = MagickCore::CenterGravity;
			else if (_wcsnicmp(parameter, L"LEFTTOP", 7) == 0 || _wcsnicmp(parameter, L"LEFT", 4) == 0)
				align = MagickCore::NorthWestGravity;
			else if (_wcsnicmp(parameter, L"RIGHTTOP", 8) == 0 || _wcsnicmp(parameter, L"RIGHT", 5) == 0)
				align = MagickCore::NorthEastGravity;
			else if (_wcsnicmp(parameter, L"CENTERTOP", 9) == 0 || _wcsnicmp(parameter, L"CENTER", 6) == 0)
				align = MagickCore::NorthGravity;
			else
				RmLog(2, L"Invalid Anchor value. Anchor LeftTop is applied");

			drawList.push_back(align);
			isSetting = TRUE;
		}

		if (isSetting)
			setting.erase(setting.begin() + i);
	}

	if (isSysFont)
	{
		drawList.push_back(Magick::DrawableFont(
			fontFace,
			fontStyle,
			fontWeight,
			fontStretch
		));
	}
	else
		drawList.push_back(Magick::DrawableFont(fontFace));

	drawList.push_back(Magick::DrawableText(xPos, yPos, ws2s(text)));
	dst->contain = Magick::Image(
		Magick::Geometry(width, height),
		Magick::Color("transparent")
	);
	dst->contain.draw(drawList);

	for (auto &settingIt : setting)
	{
		std::wstring name, parameter;
		GetNamePara(settingIt, name, parameter);

		if (!ParseEffect(measure->rm, dst->contain, name, parameter))
			return FALSE;
	}
	return TRUE;
}