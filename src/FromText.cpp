#include "MagickMeter.h"
#include <filesystem>

BOOL CreateText(ImgStruct &dst, WSVector &setting, Measure * measure)
{
	std::string text = ws2s(setting[0]);

	setting.erase(setting.begin());
	
	double xPos = 0;
	double yPos = 0;

	size_t width = 600;
	size_t height = 600;

	dst.contain = Magick::Image(Magick::Geometry(600, 600), INVISIBLE);

	double skewX = 0;
	double skewY = 0;

	dst.contain.fontPointsize(40); //40 * 75% = 30 (default font size)

	int align = 7;

	int strokeAlign = 0;
	double strokeWidth = 0;
	double angle = 0;
	Magick::Color strokeColor = Magick::Color("black");
	//Default color
	dst.contain.fillColor(Magick::Color("white"));
	dst.contain.textGravity(Magick::NorthWestGravity);

	for (int i = 0; i < setting.size(); i++)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(setting[i], tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();

		BOOL isSetting = FALSE;
		if (_wcsicmp(name, L"MOVE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			xPos = MathParser::ParseF(valList[0]);
			yPos = MathParser::ParseF(valList[1]);
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ANTIALIAS") == 0)
		{
			dst.contain.textAntiAlias(MathParser::ParseB(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLOR") == 0)
		{
			dst.contain.fillColor(GetColor(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"BACKGROUNDCOLOR") == 0)
		{
			dst.contain.textUnderColor(GetColor(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"FACE") == 0)
		{
			std::wstring para = parameter;
			if (para.find(L"@") == 0) //Font file in @Resource folder
			{
				std::wstring fontDir = RmReplaceVariables(measure->rm, L"#@#Fonts\\");
				fontDir += para.substr(1);
				if (std::experimental::filesystem::exists(fontDir))
					dst.contain.font(ws2s(fontDir));
			}
			else if (std::experimental::filesystem::exists(parameter)) //Direct path to font file
			{
				dst.contain.font(ws2s(parameter));
			}
			else //Installed font family name
			{
				dst.contain.fontFamily(ws2s(parameter));
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"SIZE") == 0)
		{
			dst.contain.fontPointsize(MathParser::ParseF(parameter) * 100 / 75); //Rm font size = 75% our.
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"WEIGHT") == 0)
		{
			dst.contain.fontWeight(MathParser::ParseI(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STYLE") == 0)
		{
			if (_wcsicmp(parameter, L"NORMAL") == 0)
				dst.contain.fontStyle(Magick::NormalStyle);

			else if (_wcsicmp(parameter, L"ITALIC") == 0)
				dst.contain.fontStyle(Magick::ItalicStyle);

			else if (_wcsicmp(parameter, L"OBLIQUE") == 0)
				dst.contain.fontStyle(Magick::ObliqueStyle);
			else
			{
				dst.contain.fontStyle(Magick::NormalStyle);
				RmLog(2, L"Invalid Text Style. Normal style is applied.");
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"LINESPACING") == 0)
		{
			dst.contain.textInterlineSpacing(MathParser::ParseF(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"WORDSPACING") == 0)
		{
			dst.contain.textInterwordSpacing(MathParser::ParseF(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"TEXTKERNING") == 0)
		{
			dst.contain.textKerning(MathParser::ParseF(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ALIGN") == 0)
		{
			if (_wcsnicmp(parameter, L"LEFTCENTER", 10) == 0)
				align = 1;
			else if (_wcsnicmp(parameter, L"LEFTBOTTOM", 10) == 0)
				align = 2;
			else if (_wcsnicmp(parameter, L"RIGHTCENTER", 11) == 0)
				align = 3;
			else if (_wcsnicmp(parameter, L"RIGHTBOTTOM", 11) == 0)
				align = 4;
			else if (_wcsnicmp(parameter, L"CENTERCENTER", 11) == 0)
				align = 5;
			else if (_wcsnicmp(parameter, L"CENTERBOTTOM", 12) == 0)
				align = 6;
			else if (_wcsnicmp(parameter, L"LEFTTOP", 7) == 0 || _wcsnicmp(parameter, L"LEFT", 4) == 0)
				align = 7;
			else if (_wcsnicmp(parameter, L"RIGHTTOP", 8) == 0 || _wcsnicmp(parameter, L"RIGHT", 5) == 0)
				align = 8;
			else if (_wcsnicmp(parameter, L"CENTERTOP", 9) == 0 || _wcsnicmp(parameter, L"CENTER", 6) == 0)
				align = 9;
			else
				RmLog(2, L"Invalid Align value. Anchor Left is applied");

			isSetting = TRUE;
		}
		//TODO: Figure out how to widen text zone.
		else if (_wcsicmp(name, L"SKEW") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			skewX = MathParser::ParseF(valList[0]);
			dst.contain.transformSkewX(skewX);

			skewY = MathParser::ParseF(valList[1]);
			dst.contain.transformSkewY(skewY);
			isSetting = TRUE;
		}
		/*else if (_wcsicmp(name, L"DECORATION") == 0)
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
		}*/
		else if (_wcsicmp(name, L"DIRECTION") == 0)
		{
			Magick::DirectionType dir = MagickCore::LeftToRightDirection;
			if (_wcsicmp(parameter, L"LEFTTORIGHT") == 0)
				dir = MagickCore::LeftToRightDirection;
			else if (_wcsicmp(parameter, L"RIGHTTOLEFT") == 0)
				dir = MagickCore::RightToLeftDirection;
			else
				RmLog(2, L"Invalid Direction value. Left to Right direction is applied");

			dst.contain.textDirection(dir);
			isSetting = TRUE;
		}
		if (_wcsicmp(name, L"STROKECOLOR") == 0)
		{
			dst.contain.strokeColor(GetColor(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEWIDTH") == 0)
		{
			strokeWidth = MathParser::ParseF(parameter);
			isSetting = TRUE;
		}
		/*else if (_wcsicmp(name, L"STROKEALIGN") == 0)
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
		}*/
		if (isSetting)
			setting[i] = L"";
	}
	
	try
	{
		Magick::TypeMetric metric;
		dst.contain.fontTypeMetricsMultiline(text, &metric);
		switch (align)
		{
		case 1:
			yPos -= metric.textHeight() / 2;
			break;
		case 2:
			yPos -= metric.textHeight();
			break;
		case 3:
			xPos -= metric.textWidth();
			yPos -= metric.textHeight() / 2;
			break;
		case 4:
			xPos -= metric.textWidth();
			yPos -= metric.textHeight();
			break;
		case 5:
			xPos -= metric.textWidth() / 2;
			yPos -= metric.textHeight() / 2;
			break;
		case 6:
			xPos -= metric.textWidth() / 2;
			yPos -= metric.textHeight();
			break;
		case 8:
			xPos -= metric.textWidth();
			break;
		case 9:
			xPos -= metric.textWidth() / 2;
			break;
		}
		dst.contain.strokeWidth(strokeWidth);
		Magick::Geometry newSize(
			(size_t)(metric.textWidth()  + strokeWidth / 2 + xPos),
			(size_t)(metric.textHeight() + strokeWidth / 2 + yPos)
		);

		newSize.aspect(true);
		dst.contain.resize(newSize);
		
		dst.contain.annotate(text, Magick::Geometry(0, 0, (ssize_t)xPos, (ssize_t)yPos));
	}
	catch (Magick::Exception &error_)
	{
		error2pws(error_);
		return FALSE;
	}

	return TRUE;
}


/*STROKE ALIGN
Magick::TypeMetric metrics;
if (strokeWidth == 0)
{
	dst.contain.fontTypeMetricsMultiline(ws2s(text), &metrics);
	Magick::Geometry newSize(
		(size_t)(metrics.textWidth() + 200),
		(size_t)(metrics.textHeight() + 200)
	);
	newSize.aspect(true);
	dst.contain.resize(newSize);

	dst.contain.annotate(ws2s(text), Magick::Geometry(0, 0, 200, 200));
}
else if (strokeWidth != 0 && strokeAlign != 0)
{
	dst.contain.strokeWidth(strokeWidth * 2);
	dst.contain.strokeColor(strokeColor);

	dst.contain.fontTypeMetricsMultiline(ws2s(text), &metrics);

	Magick::Geometry newSize(
		(size_t)(metrics.textWidth()),
		(size_t)(metrics.textHeight())
	);
	newSize.aspect(true);
	dst.contain.resize(newSize);

	dst.contain.annotate(ws2s(text), newSize, Magick::NorthGravity);
	Magick::Image temp = dst.contain;
	temp.strokeColor(INVISIBLE);
	temp.annotate(ws2s(text), newSize, Magick::NorthGravity);

	if (strokeAlign == 1)
	{
		dst.contain.composite(temp, 0, 0, MagickCore::OverCompositeOp);
	}
	else if (strokeAlign == 2)
	{
		dst.contain.composite(temp, 0, 0, MagickCore::CopyAlphaCompositeOp);
	}
}
else
{
	dst.contain.strokeWidth(strokeWidth);
	dst.contain.strokeColor(strokeColor);
	dst.contain.fontTypeMetricsMultiline(ws2s(text), &metrics);

	Magick::Geometry newSize(
		(size_t)(metrics.textWidth()),
		(size_t)(metrics.textHeight())
	);
	newSize.aspect(true);
	dst.contain.resize(newSize);

	dst.contain.annotate(ws2s(text), newSize, Magick::NorthGravity);

}*/