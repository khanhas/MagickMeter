#include "MagickMeter.h"
#include <sstream>

BOOL Measure::CreateGradient(std::shared_ptr<ImgContainer> out)
{
	MagickCore::GradientType type = MagickCore::UndefinedGradient;

    if (out->config.at(0).Equal(L"LINEAR"))
    {
        type = MagickCore::LinearGradient;
    }
    else if (out->config.at(0).Equal(L"RADIAL"))
    {
        type = MagickCore::RadialGradient;
    }
	else if (out->config.at(0).Equal(L"CONICAL"))
	{
		return CreateConicalGradient(out);
	}
	else
	{
		RmLogF(rm, LOG_ERROR, L"%s is invalid Gradient type. Valid types: Linear, Radical, Conical.", out->config.at(0).para);
		return FALSE;
	}

	MagickCore::SpreadMethod spread = MagickCore::PadSpread;

    std::vector <MagickCore::StopInfo> colorMap;
    size_t totalColor = 0;

	Magick::Geometry customCanvas = Magick::Geometry{ 600, 600, 0, 0 };

	for (auto &option : out->config)
	{
        if (option.isApplied)
            continue;

		ReplaceInternalVariable(option.para, out);

		if (option.Match(L"CANVAS"))
		{
			WSVector valList = Utils::SeparateParameter(option.para, 2);
            const size_t width = MathParser::ParseSizeT(valList.at(0));
            const size_t height = MathParser::ParseSizeT(valList.at(1));

			if (width <= 0 || height <= 0)
			{
				RmLog(rm, 2, L"Invalid Width or Height value. Default canvas 600,600 is used.");
                customCanvas = Magick::Geometry{ 600, 600, 0, 0 };
			}
			else
			{
                customCanvas = Magick::Geometry{ width, height , 0, 0 };
				customCanvas.aspect(true);
			}
			option.isApplied = TRUE;
		}
		else if (option.Match(L"SPREADMETHOD"))
		{
			const int val = MathParser::ParseInt(option.para);
            if (val >= 1 && val <= 3)
            {
                spread = static_cast<MagickCore::SpreadMethod>(val);
            }
			else
			{
				spread = MagickCore::PadSpread;
				RmLogF(rm, 2, L"%s is invalid SpreadMethod. Pad Spread is applied", option.para.c_str());
			}
			option.isApplied = TRUE;
		}
		else if (option.Match(L"GRADIENTANGLE"))
		{
            const double val = MathParser::ParseDouble(option.para);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00537
			out->img.artifact("gradient:angle", std::to_string(val));
			option.isApplied = TRUE;
		}
		else if (option.Match(L"GRADIENTEXTENT"))
		{
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00560
			out->img.artifact("gradient:extent", Utils::WStringToString(option.para));
			option.isApplied = TRUE;
		}
		else if (option.Match(L"GRADIENTRADII"))
		{
			WSVector valList = Utils::SeparateList(option.para, L",", 2);
            const int radiiX = MathParser::ParseInt(valList.at(0));
            const int radiiY = MathParser::ParseInt(valList.at(1));
            std::ostringstream radii;
			radii << radiiX << "," << radiiY;
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00593
			out->img.artifact("gradient:radii", radii.str());
			option.isApplied = TRUE;
		}
		else if (option.Match(L"GRADIENTVECTOR"))
		{
			WSVector valList = Utils::SeparateList(option.para, L",", 4);
			const int X1 = MathParser::ParseInt(valList.at(0));
			const int Y1 = MathParser::ParseInt(valList.at(1));
            const int X2 = MathParser::ParseInt(valList.at(2));
            const int Y2 = MathParser::ParseInt(valList.at(3));
            std::ostringstream gradVector;
            gradVector << X1 << "," << Y1 << "," << X2 << "," << Y2;
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00522
			out->img.artifact("gradient:vector", gradVector.str());
			option.isApplied = TRUE;
		}
		else if (option.Match(L"COLORLIST"))
		{
			std::wstring rawColorList = RmReadString(rm, option.para.c_str(), L"");
			WSVector colorList = Utils::SeparateList(rawColorList, L"|", NULL);
			if (colorList.size() < 1)
            {
                RmLogF(rm, 2, L"%s is invalid ColorList.", option.para.c_str());
                return FALSE;
            }

			for (auto &colorRaw : colorList)
			{
				WSVector colorStop = Utils::SeparateList(colorRaw, L";", 2);
				if (!colorStop.at(0).empty() && !colorStop.at(1).empty())
				{
                    MagickCore::StopInfo s{
                        Utils::ParseColor(colorStop.at(0)),
                        MathParser::ParseDouble(colorStop.at(1))
                    };
					colorMap.push_back(s);
				}
			}

			totalColor = colorMap.size();

            option.isApplied = TRUE;
		}
	}

	if (totalColor == 0)
    {
        RmLog(rm, 2, L"No valid color was found. Check ColorList again.");
        return FALSE;
    }

    out->geometry = customCanvas;

	out->img.scale(customCanvas);

    const auto drawSucceeced = MagickCore::GradientImage(
		out->img.image(),
		type, spread,
        &colorMap.at(0), // convert vector to array
        totalColor,
		nullptr
	);

	if (drawSucceeced == MagickCore::MagickFalse)
	{
		RmLog(rm, 2, L"Could not draw Gradient");
		return FALSE;
	}

	return TRUE;
}

BOOL Measure::CreateConicalGradient(std::shared_ptr<ImgContainer> out)
{
	Magick::DrawableList drawList;
	Magick::Geometry customCanvas;
	std::vector <MagickCore::StopInfo> colorMap;

	for (auto &option : out->config)
	{
        if (option.isApplied)
            continue;

		if (option.Match(L"CANVAS"))
		{
			WSVector valList = Utils::SeparateParameter(option.para, 2);
            const size_t width = MathParser::ParseSizeT(valList.at(0));
            const size_t height = MathParser::ParseSizeT(valList.at(1));

			if (width <= 0 || height <= 0)
			{
				RmLog(2, L"Invalid Width or Height value. Default canvas 600,600 is used.");
                customCanvas = Magick::Geometry{ 600, 600, 0, 0 };
			}
			else
			{
                customCanvas = Magick::Geometry{ width, height, 0, 0 };
				customCanvas.aspect(true);
			}
			option.isApplied = TRUE;
		}
		else if (option.Match(L"COLORLIST"))
		{
			std::wstring rawColorList = RmReadString(rm, option.para.c_str(), L"");
			WSVector colorList = Utils::SeparateList(rawColorList, L"|", NULL);
			if (colorList.size() > 0)
			{
				for (auto &colorRaw : colorList)
				{
					WSVector colorStop = Utils::SeparateList(colorRaw, L";", 2);
					if (!colorStop.at(0).empty() && !colorStop.at(1).empty())
					{
						const double offset = MathParser::ParseDouble(colorStop.at(1));
						if (offset >= 0 && offset <= 1)
						{
                            MagickCore::StopInfo s{
                                Utils::ParseColor(colorStop.at(0)),
                                offset
                            };
							colorMap.push_back(s);
						}
					}
				}
			}
			else
			{
				RmLogF(rm, 2, L"%s is invalid ColorList.", option.para.c_str());
				return FALSE;
			}
			option.isApplied = TRUE;
		}
	}

	const double maxSize = round(sqrt(customCanvas.height() * customCanvas.height() + customCanvas.width() * customCanvas.width()) / 2);
    const double centerX = static_cast<double>(customCanvas.width()) / 2;
    const double centerY = static_cast<double>(customCanvas.height()) / 2;

	Magick::PathMovetoAbs centerPoint(Magick::Coordinate(centerX, centerY));

	std::sort(
        colorMap.begin(),
        colorMap.end(),
        [](MagickCore::StopInfo i, MagickCore::StopInfo j) noexcept {
            return (i.offset < j.offset);
        }
    );

    const size_t totalColor = colorMap.size();
	if (totalColor == 0)
	{
		RmLog(rm, LOG_ERROR, L"No valid color was found. Check ColorList again.");
		return FALSE;
	}

	for (size_t i = 0; i < totalColor; i++)
	{
		Magick::Color c1;
		double start = 0;
		if (i == 0)
		{
			start = colorMap[totalColor - 1].offset - 1;
			c1 = colorMap[totalColor - 1].color;
		}
		else
		{
            const size_t index = i - (size_t)1;
			start = colorMap[index].offset;
			c1 = colorMap[index].color;
		}

		Magick::Color c2 = colorMap[i].color;
        const double end = colorMap[i].offset;

        const double half = (end - start) / 2 + start;
		Magick::VPathList pie1;

		pie1.push_back(centerPoint);

		pie1.push_back(Magick::PathLinetoAbs(Magick::Coordinate(
			maxSize*sin(start * Magick2PI) + centerX,
			maxSize*cos(start * Magick2PI) + centerY
		)));

		pie1.push_back(Magick::PathArcAbs(Magick::PathArcArgs(
			maxSize, maxSize,
			0, false, false,
			maxSize*sin(half * Magick2PI) + centerX,
			maxSize*cos(half * Magick2PI) + centerY
		)));

		pie1.push_back(Magick::PathClosePath());

		drawList.push_back(Magick::DrawableFillColor(c1));
		drawList.push_back(Magick::DrawablePath(pie1));

		Magick::VPathList pie2;

		pie2.push_back(centerPoint);

		pie2.push_back(Magick::PathLinetoAbs(Magick::Coordinate(
			maxSize*sin(half * Magick2PI) + centerX,
			maxSize*cos(half * Magick2PI) + centerY
		)));

		pie2.push_back(Magick::PathArcAbs(Magick::PathArcArgs(
			maxSize, maxSize,
			0, false, false,
			maxSize*sin(end * Magick2PI) + centerX,
			maxSize*cos(end * Magick2PI) + centerY
		)));

		pie2.push_back(Magick::PathClosePath());

		drawList.push_back(Magick::DrawableFillColor(c2));
		drawList.push_back(Magick::DrawablePath(pie2));
	};

    out->geometry = customCanvas;
	out->img.scale(customCanvas);
	out->img.draw(drawList);
	out->img.blur(100, 100);
	return TRUE;
}