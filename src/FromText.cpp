#include "MagickMeter.h"
#include <filesystem>

BOOL CreateText(ImgStruct * dst, WSVector setting, Measure * measure)
{
	std::wstring text = setting[0];

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
	drawList.push_back(Magick::DrawablePointSize(40)); //40 * 75% = 30 (default font size)

	int strokeAlign = 0;
	double strokeWidth = 0;
	Magick::Color strokeColor = Magick::Color("black");
	//Default color
	drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));

	for (int i = 0; i < setting.size(); i++)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(setting[i], tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();
		
		BOOL isSetting = FALSE;
		if (_wcsicmp(name, L"CANVAS") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
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
			WSVector valList = SeparateList(parameter, L",", 2);
			if (valList.size() > 0)
			{
				xPos = MathParser::ParseF(valList[0]);
				yPos = MathParser::ParseF(valList[1]);
			}
			drawList.push_back(Magick::DrawableTranslation(xPos, yPos));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ROTATE") == 0)
		{
			drawList.push_back(Magick::DrawableRotation(
				MathParser::ParseF(parameter)
			));
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
		else if (_wcsicmp(name, L"BACKGROUNDCOLOR") == 0)
		{
			drawList.push_back(Magick::DrawableTextUnderColor(
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
				std::wstring fontDir = RmReplaceVariables(measure->rm, L"#@#Fonts\\");
				fontDir += para.substr(1);
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
		else if (_wcsicmp(name, L"LINESPACING") == 0)
		{
			drawList.push_back(Magick::DrawableTextInterlineSpacing(
				MathParser::ParseF(parameter)
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"WORDSPACING") == 0)
		{
			drawList.push_back(Magick::DrawableTextInterwordSpacing(
				MathParser::ParseF(parameter)
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"TEXTKERNING") == 0)
		{
			drawList.push_back(Magick::DrawableTextKerning(
				MathParser::ParseF(parameter)
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ALIGN") == 0)
		{

			Magick::DrawableTextAlignment align = MagickCore::LeftAlign;
			if (_wcsnicmp(parameter, L"LEFT", 4) == 0)
				align = MagickCore::LeftAlign;
			else if (_wcsnicmp(parameter, L"RIGHT", 5) == 0)
				align = MagickCore::RightAlign;
			else if (_wcsnicmp(parameter, L"CENTER", 6) == 0)
				align = MagickCore::CenterAlign;
			else
				RmLog(2, L"Invalid Align value. Anchor Left is applied");

			drawList.push_back(align);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"DECORATION") == 0)
		{
			Magick::DrawableTextDecoration decor = MagickCore::NoDecoration;
			if (_wcsicmp(parameter, L"UNDERLINE") == 0)
				decor = MagickCore::UnderlineDecoration;
			else if (_wcsicmp(parameter, L"OVERLINE") == 0)
				decor = MagickCore::OverlineDecoration;
			else if (_wcsicmp(parameter, L"STRIKETHROUGH") == 0)
				decor = MagickCore::LineThroughDecoration;
			else
				RmLog(2, L"Invalid Decoration value. No decoration is applied");

			drawList.push_back(decor);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"DIRECTION") == 0)
		{
			Magick::DrawableTextDirection dir = MagickCore::LeftToRightDirection;
			if (_wcsicmp(parameter, L"LEFTTORIGHT") == 0)
				dir = MagickCore::LeftToRightDirection;
			else if (_wcsicmp(parameter, L"RIGHTTOLEFT") == 0)
				dir = MagickCore::RightToLeftDirection;
			else
				RmLog(2, L"Invalid Direction value. Left to Right direction is applied");

			drawList.push_back(dir);
			isSetting = TRUE;
		}
		if (_wcsicmp(name, L"STROKECOLOR") == 0)
		{
			strokeColor = GetColor(parameter);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEWIDTH") == 0)
		{
			strokeWidth = MathParser::ParseF(parameter);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEALIGN") == 0)
		{
			if (_wcsicmp(parameter, L"CENTER") == 0)
				strokeAlign = 0;

			else if (_wcsicmp(parameter, L"OUTSIDE") == 0)
				strokeAlign = 1;

			else if (_wcsicmp(parameter, L"INSIDE") == 0)
				strokeAlign = 2;
			else
			{
				strokeAlign = 0;
				RmLog(2, L"Invalid StrokeAlign value. Center align is applied.");
			}

			isSetting = TRUE;
		}
		if (isSetting)
			setting[i] = L"";
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

	drawList.push_back(Magick::DrawableText(0, 0, ws2s(text)));

	try
	{
		dst->contain = Magick::Image(Magick::Geometry(width, height), Magick::Color("transparent"));
		if (strokeWidth == 0)
		{
			drawList.push_back(Magick::DrawableStrokeColor(Magick::Color("transparent")));
			dst->contain.draw(drawList);
		}
		else if (strokeWidth != 0 && strokeAlign != 0)
		{
			Magick::Image temp = dst->contain;
			std::vector<Magick::Drawable> tempList = drawList;

			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth * 2));
			drawList.push_back(Magick::DrawableStrokeColor(strokeColor));
			dst->contain.draw(drawList);

			tempList.push_back(Magick::DrawableStrokeWidth(0));
			tempList.push_back(Magick::DrawableStrokeColor(Magick::Color("transparent")));
			temp.draw(tempList);
			tempList.clear();

			if (strokeAlign == 1)
				dst->contain.composite(temp, 0, 0, MagickCore::OverCompositeOp);
			else if (strokeAlign == 2)
				dst->contain.composite(temp, 0, 0, MagickCore::CopyAlphaCompositeOp);
		}
		else
		{
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth));
			drawList.push_back(Magick::DrawableStrokeColor(strokeColor));
			dst->contain.draw(drawList);
		}

		drawList.clear();
	}
	catch (Magick::Exception &error_)
	{
		error2pws(error_);
		return FALSE;
	}

	for (auto &settingIt : setting)
	{
		if (settingIt.empty()) continue;
		std::wstring name, parameter;
		GetNamePara(settingIt, name, parameter);

		if (!ParseEffect(measure->rm, dst->contain, name, parameter))
			return FALSE;
	}
	return TRUE;
}