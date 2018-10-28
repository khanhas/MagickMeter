#include "MagickMeter.h"

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

BOOL Measure::CreateText(std::wstring text, WSVector &config, ImgContainer &out)
{
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
	size_t clipW = 0;
	size_t clipH = 0;
	size_t clipLine = 0;
	for (auto &option : config)
	{
        if (option.empty())
            continue;

        std::wstring tempName;
        std::wstring tempPara;
		Utils::GetNamePara(option, tempName, tempPara);
		ParseInternalVariable(tempPara, out);

		LPCWSTR name = tempName.c_str();
        LPCWSTR parameter = tempPara.c_str();

		BOOL isValidOption = FALSE;
		if (_wcsicmp(name, L"CANVAS") == 0)
		{
			WSVector valList = Utils::SeparateList(parameter, L",", 2);
            const size_t width = MathParser::ParseSizeT(valList[0]);
            const size_t height = MathParser::ParseSizeT(valList[1]);

			if (width <= 0 || height <= 0)
				RmLog(rm, LOG_WARNING, L"Invalid Width or Height value. Default canvas is used.");
			else
			{
				customCanvas = Magick::Geometry(width, height);
				customCanvas.aspect(true);
			}
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"OFFSET") == 0)
		{
			WSVector valList = Utils::SeparateList(tempPara, L",", 2);
			xPos = MathParser::ParseDouble(valList[0]);
			yPos = MathParser::ParseDouble(valList[1]);
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"ANTIALIAS") == 0)
		{
			tempImg.textAntiAlias(MathParser::ParseBool(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"COLOR") == 0)
		{
			tempImg.fillColor(Utils::ParseColor(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"BACKGROUNDCOLOR") == 0)
		{
			tempImg.textUnderColor(Utils::ParseColor(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"FACE") == 0)
		{
            // Font file in @Resource folder
			if (tempPara.find(L"@") == 0)
			{
				std::wstring fontDir = RmReplaceVariables(rm, L"#@#Fonts\\");
				fontDir += tempPara.substr(1);
				if (std::filesystem::exists(fontDir))
					tempImg.font(Utils::WStringToString(fontDir));
			}
            // Direct path to font file
			else if (std::filesystem::exists(parameter))
			{
				tempImg.font(Utils::WStringToString(tempPara));
			}
            // Installed font family name
			else
			{
				tempImg.fontFamily(Utils::WStringToString(tempPara));
			}
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"SIZE") == 0)
		{
            // Rainmeter font size = 75% ours.
			tempImg.fontPointsize(MathParser::ParseDouble(tempPara) * 100 / 75);
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"WEIGHT") == 0)
		{
			tempImg.fontWeight(MathParser::ParseInt(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"STYLE") == 0)
		{
			if (_wcsicmp(parameter, L"NORMAL") == 0)
				tempImg.fontStyle(Magick::NormalStyle);

			else if (_wcsicmp(parameter, L"ITALIC") == 0)
				tempImg.fontStyle(Magick::ItalicStyle);

			else if (_wcsicmp(parameter, L"OBLIQUE") == 0)
				tempImg.fontStyle(Magick::ObliqueStyle);
			else
			{
				tempImg.fontStyle(Magick::NormalStyle);
				RmLog(rm, LOG_WARNING, L"Invalid Text Style. Normal style is applied.");
			}
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"CASE") == 0)
		{
            WCHAR* textAddress = &text[0];
            const int textLen = static_cast<int>(text.length());
			if (_wcsicmp(parameter, L"UPPER") == 0)
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, textAddress, textLen, textAddress, textLen);
			else if (_wcsicmp(parameter, L"LOWER") == 0)
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, textAddress, textLen, textAddress, textLen);
			else if (_wcsicmp(parameter, L"PROPER") == 0)
				LCMapString(LOCALE_USER_DEFAULT, LCMAP_TITLECASE, textAddress, textLen, textAddress, textLen);
			else if (_wcsicmp(parameter, L"NONE") == 0)
			{
			}
			else
				RmLogF(rm, LOG_WARNING, L"%s is invalid Text Case.", parameter);

			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"LINESPACING") == 0)
		{
			tempImg.textInterlineSpacing(MathParser::ParseDouble(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"WORDSPACING") == 0)
		{
			tempImg.textInterwordSpacing(MathParser::ParseDouble(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"TEXTKERNING") == 0)
		{
			tempImg.textKerning(MathParser::ParseDouble(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"ALIGN") == 0)
		{
			if (_wcsnicmp(parameter, L"LEFTCENTER", 10) == 0)
				align = LEFTCENTER;
			else if (_wcsnicmp(parameter, L"LEFTBOTTOM", 10) == 0)
				align = LEFTBOTTOM;
			else if (_wcsnicmp(parameter, L"RIGHTCENTER", 11) == 0)
				align = RIGHTCENTER;
			else if (_wcsnicmp(parameter, L"RIGHTBOTTOM", 11) == 0)
				align = RIGHTBOTTOM;
			else if (_wcsnicmp(parameter, L"CENTERCENTER", 11) == 0)
				align = CENTERCENTER;
			else if (_wcsnicmp(parameter, L"CENTERBOTTOM", 12) == 0)
				align = CENTERBOTTOM;
			else if (_wcsnicmp(parameter, L"LEFTTOP", 7) == 0 || _wcsnicmp(parameter, L"LEFT", 4) == 0)
				align = LEFTTOP;
			else if (_wcsnicmp(parameter, L"RIGHTTOP", 8) == 0 || _wcsnicmp(parameter, L"RIGHT", 5) == 0)
				align = RIGHTTOP;
			else if (_wcsnicmp(parameter, L"CENTERTOP", 9) == 0 || _wcsnicmp(parameter, L"CENTER", 6) == 0)
				align = CENTERTOP;
			else
				RmLog(rm, LOG_WARNING, L"Invalid Align value. Anchor Left is applied");

			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"SKEW") == 0)
		{
			WSVector valList = Utils::SeparateList(tempPara, L",", 2);
			skewX = MathParser::ParseDouble(valList[0]);
			tempImg.transformSkewX(skewX);

			skewY = MathParser::ParseDouble(valList[1]);
			tempImg.transformSkewY(skewY);
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"DIRECTION") == 0)
		{
			if (_wcsicmp(parameter, L"LEFTTORIGHT") == 0)
            {
                tempImg.textDirection(Magick::LeftToRightDirection);

            }
            else if (_wcsicmp(parameter, L"RIGHTTOLEFT") == 0)
            {
                tempImg.textDirection(Magick::RightToLeftDirection);
            }
            else
            {
                tempImg.textDirection(Magick::LeftToRightDirection);
                RmLog(rm, LOG_WARNING, L"Invalid Direction value. Left to Right direction is applied");
            }

			isValidOption = TRUE;
		}
		if (_wcsicmp(name, L"STROKECOLOR") == 0)
		{
			tempImg.strokeColor(Utils::ParseColor(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEWIDTH") == 0)
		{
			strokeWidth = MathParser::ParseDouble(tempPara);
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"DENSITY") == 0)
		{
			tempImg.density(MathParser::ParseDouble(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"CLIPSTRINGW") == 0)
		{
			clipW = (size_t)abs(MathParser::ParseInt(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"CLIPSTRINGH") == 0)
		{
			clipH = (size_t)abs(MathParser::ParseInt(tempPara));
			isValidOption = TRUE;
		}
		else if (_wcsicmp(name, L"CLIPSTRINGLINE") == 0)
		{
			clipLine = (size_t)abs(MathParser::ParseInt(tempPara));
			isValidOption = TRUE;
		}

		if (isValidOption)
			option.clear();
	}

	try
	{
		Magick::TypeMetric metric;

		tempImg.fontTypeMetricsMultiline(Utils::WStringToString(text), &metric);

		if (clipW != 0)
		{
			if (metric.textWidth() > clipW)
			{
				WSVector words = Utils::SeparateList(text, L" ", NULL);

				WSVector lines;
				size_t offset = 1;
                size_t startLine = 0;
				while (startLine + offset <= words.size())
				{
					std::wstring curLine = L"";
					for (size_t j = startLine; j < startLine + offset; j++)
						curLine += words[j] + L" ";

					tempImg.fontTypeMetricsMultiline(Utils::WStringToString(curLine), &metric);
					if ((size_t)ceil(metric.textWidth()) < clipW)
						offset++;
					else
					{
						std::wstring addLine = L"";
						if (offset == 1)
						{
							addLine = words[startLine];
							if (metric.textWidth() > clipW)
							{
								std::wstring w = L"";
								std::wstring oldw = L"";
								for (int c = 0; c < addLine.length(); c++)
								{
									oldw = w;
									w += addLine[c];
									tempImg.fontTypeMetricsMultiline(Utils::WStringToString(w + L"-"), &metric);
									if (metric.textWidth() > clipW)
									{
										w = L"";
										for (int rC = c; rC < addLine.length(); rC++)
											w += words[startLine][rC];

										words[startLine] = w;
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
                                addLine += words[j] + L" ";
                        }

						lines.push_back(addLine);
						startLine += offset - 1;
						offset = 1;
					}
				}
				if (startLine < words.size())
				{
					std::wstring addLine = L"";
					for (UINT i = startLine; i < words.size(); i++)
						addLine += words[i] + L" ";
					lines.push_back(addLine);
				}
				if (lines.size() >= 1)
					text = lines[0];
				for (size_t i = 1; i < lines.size(); i++)
					text += L"\n" + lines[i];
			}

		}

		if (clipH != 0)
		{
			WSVector lineList = Utils::SeparateList(text, L"\n", NULL);
			const size_t listSize = lineList.size();
			if (listSize > 1)
			{
				std::wstring curText = lineList[0];
				size_t lastLine = 0;
				for (size_t i = 1; i < listSize; i++)
				{
					tempImg.fontTypeMetricsMultiline(Utils::WStringToString(curText + L"\n" + lineList[i]), &metric);
					if (metric.textHeight() <= clipH)
					{
						curText += L"\n" + lineList[i];
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
                            curText[i] = L'.';
                            curText[i - 1] = L'.';
                            curText[i - 2] = L'.';
                            if (i != curText.length() - 1)
                                curText.pop_back();

                            tempImg.fontTypeMetricsMultiline(Utils::WStringToString(curText), &metric);

                            if (metric.textWidth() <= clipW)
                                break;
                        }
                    }
					text = curText;
				}
			}
		}

		if (clipLine != 0)
		{
			WSVector lineList = Utils::SeparateList(text, L"\n", NULL);
			const size_t listSize = lineList.size();
			if (listSize > 1 && clipLine < listSize)
			{
				std::wstring curText = lineList[0];
				for (size_t i = 1; i < clipLine; i++)
					curText += L"\n" + lineList[i];

				curText += L"...";
                if (clipW != 0)
                {
                    for (size_t i = curText.length() - 1; i >= 2; i--)
                    {
                        if (i != curText.length() - 1)
                        {
                            curText.pop_back();
                            curText[i] = L'.';
                            curText[i - 1] = L'.';
                            curText[i - 2] = L'.';
                        }

                        tempImg.fontTypeMetricsMultiline(Utils::WStringToString(curText), &metric);
                        if (metric.textWidth() <= clipW)
                            break;
                    }
                }
				text = curText;
			}
		}

		tempImg.fontTypeMetricsMultiline(Utils::WStringToString(text), &metric);

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

        tempImg.scale(tempSize);
		tempImg.annotate(Utils::WStringToString(text), tempSize, grav);
		tempImg.crop(tempImg.boundingBox());

		out.W = tempImg.columns();
		out.H = tempImg.rows();

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

		out.X = (ssize_t)round(xPos);
		out.Y = (ssize_t)round(yPos);

		if (!customCanvas.isValid())
		{
			customCanvas = Magick::Geometry(
				out.X + out.W,
				out.Y + out.H
			);
			customCanvas.aspect(true);
		}

        out.img.scale(customCanvas);

        out.img.composite(tempImg, out.X, out.Y, Magick::OverCompositeOp);
	}
	catch (Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}

	return TRUE;
}