#include "MagickMeter.h"
#include <sstream>

Magick::TypeMetric getMetric(Magick::Image &img, const std::wstring &text);

const enum TextAlign {
    INVALIDALIGN,

    LEFTCENTER,
    LEFTBOTTOM,

    RIGHTCENTER,
    RIGHTBOTTOM,

    CENTERCENTER,
    CENTERBOTTOM,

    LEFTTOP,
    RIGHTTOP,
    CENTERTOP,
};

BOOL Measure::CreateText(std::shared_ptr<ImgContainer> out)
{
    std::wstring text = out->config.at(0).para;
    Magick::Image tempImg = ONEPIXEL;

    tempImg.fontPointsize(40); //40 * 75% = 30 (default font size)
    tempImg.fillColor(Magick::Color("white"));
    TextAlign align = LEFTTOP;

	double xPos = 0;
	double yPos = 0;
	double skewX = 0;
	double skewY = 0;
	double strokeWidth = 0;
    Magick::Color strokeColor = Magick::Color("black");

	Magick::Geometry customCanvas;
	double clipW = 0.0;
	double clipH = 0.0;
	size_t clipLine = 0;
	for (auto &option : out->config)
	{
        if (option.isApplied)
            continue;

		ReplaceInternalVariable(option.para, out);

        option.isApplied = TRUE;

		if (option.Match(L"CANVAS"))
		{
			auto valList = option.ToList(2);
            const size_t width = MathParser::ParseSizeT(valList.at(0));
            const size_t height = MathParser::ParseSizeT(valList.at(1));

			if (width <= 0 || height <= 0)
				RmLog(rm, LOG_WARNING, L"Invalid Width or Height value. Default canvas is used.");
			else
			{
				customCanvas = Magick::Geometry(width, height);
				customCanvas.aspect(true);
			}
		}
		else if (option.Match(L"OFFSET"))
		{
			auto valList = option.ToList(2);
			xPos = MathParser::ParseDouble(valList.at(0));
			yPos = MathParser::ParseDouble(valList.at(1));
		}
		else if (option.Match(L"ANTIALIAS"))
		{
			tempImg.textAntiAlias(option.ToBool());
		}
		else if (option.Match(L"COLOR"))
		{
			tempImg.fillColor(option.ToColor());
		}
		else if (option.Match(L"BACKGROUNDCOLOR"))
		{
			tempImg.textUnderColor(option.ToColor());
		}
		else if (option.Match(L"FACE"))
		{
            // Font file in @Resource folder
			if (option.para.find(L"@") == 0)
			{
				std::wstring fontDir = RmReplaceVariables(rm, L"#@#Fonts\\");
				fontDir += option.para.substr(1);
				if (std::filesystem::exists(fontDir))
					tempImg.font(Utils::WStringToString(fontDir));
			}
            // Direct path to font file
			else if (std::filesystem::exists(option.para.c_str()))
			{
				tempImg.font(option.ToString());
			}
            // Installed font family name
			else
			{
				tempImg.fontFamily(option.ToString());
			}
		}
		else if (option.Match(L"SIZE"))
		{
            // Rainmeter font size = 75% ours.
			tempImg.fontPointsize(option.ToDouble() * 100 / 75);
		}
		else if (option.Match(L"WEIGHT"))
		{
			tempImg.fontWeight(option.ToSizeT());
		}
		else if (option.Match(L"STYLE"))
		{
			if (option.Equal(L"NORMAL"))
				tempImg.fontStyle(Magick::NormalStyle);

			else if (option.Equal(L"ITALIC"))
				tempImg.fontStyle(Magick::ItalicStyle);

			else if (option.Equal(L"OBLIQUE"))
				tempImg.fontStyle(Magick::ObliqueStyle);
			else
			{
				tempImg.fontStyle(Magick::NormalStyle);
				RmLog(rm, LOG_WARNING, L"Invalid Text Style. Normal style is applied.");
			}
		}
		else if (option.Match(L"CASE"))
		{
            WCHAR* textAddress = &text.at(0);
            const int textLen = static_cast<int>(text.length());
			if (option.Equal(L"UPPER"))
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, textAddress, textLen, textAddress, textLen);
			else if (option.Equal(L"LOWER"))
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, textAddress, textLen, textAddress, textLen);
			else if (option.Equal(L"PROPER"))
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_TITLECASE, textAddress, textLen, textAddress, textLen);
			else if (option.Equal(L"NONE"))
			{
			}
			else
				RmLogF(rm, LOG_WARNING, L"%s is invalid Text Case.", option.para.c_str());

		}
		else if (option.Match(L"LINESPACING"))
		{
			tempImg.textInterlineSpacing(option.ToDouble());
		}
		else if (option.Match(L"WORDSPACING"))
		{
			tempImg.textInterwordSpacing(option.ToDouble());
		}
		else if (option.Match(L"TEXTKERNING"))
		{
			tempImg.textKerning(option.ToDouble());
		}
		else if (option.Match(L"ALIGN"))
		{
			if (option.Equal(L"LEFTCENTER"))
				align = LEFTCENTER;
			else if (option.Equal(L"LEFTBOTTOM"))
				align = LEFTBOTTOM;
			else if (option.Equal(L"RIGHTCENTER"))
				align = RIGHTCENTER;
			else if (option.Equal(L"RIGHTBOTTOM"))
				align = RIGHTBOTTOM;
			else if (option.Equal(L"CENTERCENTER"))
				align = CENTERCENTER;
			else if (option.Equal(L"CENTERBOTTOM"))
				align = CENTERBOTTOM;
			else if (option.Equal(L"LEFTTOP") || option.Equal(L"LEFT"))
				align = LEFTTOP;
			else if (option.Equal(L"RIGHTTOP") || option.Equal(L"RIGHT"))
				align = RIGHTTOP;
			else if (option.Equal(L"CENTERTOP") || option.Equal(L"CENTER"))
				align = CENTERTOP;
			else
				RmLog(rm, LOG_WARNING, L"Invalid Align value. Anchor Left is applied");

		}
		else if (option.Match(L"SKEW"))
		{
			auto valList = option.ToList(2);
			skewX = MathParser::ParseDouble(valList.at(0));
			tempImg.transformSkewX(skewX);

			skewY = MathParser::ParseDouble(valList.at(1));
			tempImg.transformSkewY(skewY);
		}
		else if (option.Match(L"DIRECTION"))
		{
			if (option.Equal(L"LEFTTORIGHT"))
            {
                tempImg.textDirection(Magick::LeftToRightDirection);
            }
            else if (option.Equal(L"RIGHTTOLEFT"))
            {
                tempImg.textDirection(Magick::RightToLeftDirection);
            }
            else
            {
                tempImg.textDirection(Magick::LeftToRightDirection);
                RmLog(rm, LOG_WARNING, L"Invalid Direction value. Left to Right direction is applied");
            }
		}
		if (option.Match(L"STROKECOLOR"))
		{
			tempImg.strokeColor(option.ToColor());
		}
		else if (option.Match(L"STROKEWIDTH"))
		{
			strokeWidth = option.ToDouble();
		}
		else if (option.Match(L"DENSITY"))
		{
			tempImg.density(option.ToDouble());
		}
		else if (option.Match(L"CLIPSTRINGW"))
		{
			clipW = abs(option.ToDouble());
		}
		else if (option.Match(L"CLIPSTRINGH"))
		{
			clipH = abs(option.ToDouble());
		}
		else if (option.Match(L"CLIPSTRINGLINE"))
		{
			clipLine = option.ToSizeT();
        }
        else
        {
            option.isApplied = FALSE;
        }
	}

    Magick::TypeMetric metric = getMetric(tempImg, text);

    if (clipW > 0 && metric.textWidth() > clipW)
    {
        WSVector words = Utils::SeparateList(text, L" ", NULL);

        WSVector lines;
        size_t offset = 1;
        size_t startLine = 0;
        while (startLine + offset <= words.size())
        {
            std::wstringstream curLine;
            for (size_t j = startLine; j < startLine + offset; j++)
            {
                if (j != startLine)
                    curLine << L" ";

                curLine << words.at(j);
            }

            metric = getMetric(tempImg, curLine.str());
            const double curLineWidth = ceil(metric.textWidth());
            if (curLineWidth < clipW)
            {
                offset++;
                continue;
            }

            std::wstring addLine = L"";
            if (offset == 1)
            {
                addLine = words.at(startLine);
                if (metric.textWidth() > clipW)
                {
                    std::wstring w = L"";
                    std::wstring oldw = L"";
                    for (int c = 0; c < addLine.length(); c++)
                    {
                        oldw = w;
                        w += addLine.at(c);

                        metric = getMetric(tempImg, w + L"-");
                        if (metric.textWidth() > clipW)
                        {
                            w = L"";
                            for (int rC = c; rC < addLine.length(); rC++)
                                w += words.at(startLine).at(rC);

                            words.at(startLine) = w;
                            addLine = oldw + L"-";
                            break;
                        }
                    }
                }
                else
                {
                    offset++;
                }
            }
            else
            {
                for (UINT j = startLine; j < startLine + offset - 1; j++)
                    addLine += words.at(j) + L" ";
            }

            lines.push_back(addLine);
            startLine += offset - 1;
            offset = 1;
        }
        if (startLine < words.size())
        {
            std::wstring addLine = L"";
            for (UINT i = startLine; i < words.size(); i++)
                addLine += words.at(i) + L" ";
            lines.push_back(addLine);
        }

        std::wstringstream curTextStream;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (i != 0)
                curTextStream << L"\n";
            curTextStream << lines.at(i);
        }

        text = curTextStream.str();
    }

    if (clipH > 0)
    {
        WSVector lineList = Utils::SeparateList(text, L"\n", NULL);
        const size_t listSize = lineList.size();
        if (listSize > 1)
        {
            std::wstring curText = lineList.at(0);
            size_t lastLine = 0;
            for (size_t i = 1; i < listSize; i++)
            {
                metric = getMetric(tempImg, curText + L"\n" + lineList.at(i));

                if (metric.textHeight() <= clipH)
                {
                    curText += L"\n" + lineList.at(i);
                    lastLine = i;
                }
                else
                {
                    break;
                }
            }

            if (lastLine != (listSize - 1))
            {
                curText += L"...";
                if (clipW != 0)
                {
                    for (size_t i = curText.length() - 1; i >= 2; i--)
                    {
                        curText.at(i) = L'.';
                        curText.at(i - 1) = L'.';
                        curText.at(i - 2) = L'.';
                        if (i != curText.length() - 1)
                            curText.pop_back();

                        metric = getMetric(tempImg, curText);
                        if (metric.textWidth() <= clipW)
                            break;
                    }
                }
                text = curText;
            }
        }
    }

    if (clipLine > 0)
    {
        WSVector lineList = Utils::SeparateList(text, L"\n", NULL);
        if (clipLine < lineList.size())
        {
            std::wstringstream curTextStream;
            for (size_t i = 0; i < clipLine; ++i)
            {
                if (i != 0)
                    curTextStream << L"\n";
                curTextStream << lineList.at(i);
            }

            curTextStream << L"...";

            auto curText = curTextStream.str();

            if (clipW > 0)
            {
                const size_t lastIndex = curText.length() - 1;
                for (size_t i = lastIndex; i >= 2; i--)
                {
                    if (i != lastIndex)
                    {
                        curText.pop_back();
                        curText.at(i - 2) = L'.';
                    }

                    metric = getMetric(tempImg, curText);
                    if (metric.textWidth() <= clipW)
                        break;
                }
            }
            text = curText;
        }
    }

    metric = getMetric(tempImg, text);

    tempImg.strokeWidth(strokeWidth);

    // Some fonts have glyphs that overflows normal zone
    // and fontTypeMetric doesn't detect that.
    // So we have to offset a textHeight() amount.
    Magick::Geometry tempSize(
        (size_t)ceil(metric.textWidth() + strokeWidth / 2 + metric.textHeight() * 2),
        (size_t)ceil(metric.textHeight() + strokeWidth / 2 + metric.textHeight() * 2),
        (ssize_t)ceil(metric.textHeight()),
        (ssize_t)ceil(metric.textHeight())
    );
    tempSize.aspect(true);

    Magick::GravityType grav = Magick::NorthWestGravity;
    switch (align)
    {
    case RIGHTCENTER:
    case RIGHTBOTTOM:
    case RIGHTTOP:
        grav = Magick::NorthEastGravity;
        break;
    case CENTERCENTER:
    case CENTERBOTTOM:
    case CENTERTOP:
        grav = Magick::NorthGravity;
        break;
    }

	try
	{
        tempImg.scale(tempSize);
		tempImg.annotate(Utils::WStringToString(text), tempSize, grav);
		tempImg.crop(tempImg.boundingBox());

		switch (align)
		{
		case LEFTCENTER:
			yPos -= tempImg.rows() / 2;
			break;
		case LEFTBOTTOM:
			yPos -= tempImg.rows();
			break;
		case RIGHTCENTER:
			xPos -= tempImg.columns();
			yPos -= tempImg.rows() / 2;
			break;
		case RIGHTBOTTOM:
			xPos -= tempImg.columns();
			yPos -= tempImg.rows();
			break;
		case CENTERCENTER:
			xPos -= tempImg.columns() / 2;
			yPos -= tempImg.rows() / 2;
			break;
		case CENTERBOTTOM:
			xPos -= tempImg.columns() / 2;
			yPos -= tempImg.rows();
			break;
		case RIGHTTOP:
			xPos -= tempImg.columns();
			break;
		case CENTERTOP:
			xPos -= tempImg.columns() / 2;
			break;
		}

        out->geometry = Magick::Geometry{
            tempImg.columns(),
            tempImg.rows(),
            static_cast<ssize_t>(round(xPos)),
            static_cast<ssize_t>(round(yPos))
        };

		if (!customCanvas.isValid())
		{
            ssize_t canvasWidth = out->geometry.xOff() + out->geometry.width();
            ssize_t canvasHeight = out->geometry.yOff() + out->geometry.height();
            if (canvasWidth > 0 || canvasHeight > 0)
            {
                if (canvasWidth < 0) canvasWidth = 0;
                if (canvasHeight < 0) canvasHeight = 0;
                customCanvas = Magick::Geometry{
                    static_cast<size_t>(canvasWidth),
                    static_cast<size_t>(canvasHeight)
                };
                customCanvas.aspect(true);
            }
            else
            {
                return TRUE;
            }
		}

        out->img.scale(customCanvas);

        out->img.composite(
            tempImg,
            out->geometry.xOff(),
            out->geometry.yOff(),
            Magick::OverCompositeOp
        );
	}
	catch (Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}

	return TRUE;
}

Magick::TypeMetric getMetric(Magick::Image &img, const std::wstring &text)
{
    Magick::TypeMetric metric;
    img.fontTypeMetricsMultiline(Utils::WStringToString(text), &metric);
    return metric;
}