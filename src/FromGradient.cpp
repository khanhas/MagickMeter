#include "MagickMeter.h"
#include <sstream>

BOOL CreateConicalGradient(Measure* measure, WSVector &config, ImgContainer &out);

BOOL Measure::CreateGradient(LPCWSTR gradType, WSVector &config, ImgContainer &out)
{
	MagickCore::GradientType type = MagickCore::UndefinedGradient;
    if (_wcsicmp(gradType, L"LINEAR") == 0)
    {
        type = MagickCore::LinearGradient;
    }
    else if (_wcsicmp(gradType, L"RADIAL") == 0)
    {
        type = MagickCore::RadialGradient;
    }
	else if (_wcsicmp(gradType, L"CONICAL") == 0)
	{
		return CreateConicalGradient(this, config, out);
	}
	else
	{
		RmLogF(rm, 2, L"%s is invalid Gradient type. Valid types: Linear, Radical, Conical.", config[0].c_str());
		return FALSE;
	}

	MagickCore::SpreadMethod spread = MagickCore::PadSpread;

    std::vector <MagickCore::StopInfo> colorMap;
    size_t totalColor = 0;

	Magick::Geometry customCanvas = Magick::Geometry(600, 600);

	for (auto &option : config)
	{
        if (option.empty())
            continue;

		std::wstring tempName, tempPara;
		Utils::GetNamePara(option, tempName, tempPara);
		ParseInternalVariable(tempPara, out);

		LPCWSTR name = tempName.c_str();

		BOOL isSetting = FALSE;

		if (_wcsicmp(name, L"CANVAS") == 0)
		{
			WSVector valList = Utils::SeparateParameter(tempPara, 2);
            const size_t width = MathParser::ParseSizeT(valList[0]);
            const size_t height = MathParser::ParseSizeT(valList[1]);

			if (width <= 0 || height <= 0)
			{
				RmLog(2, L"Invalid Width or Height value. Default canvas 600,600 is used.");
				customCanvas = Magick::Geometry(600, 600);
			}
			else
			{
				customCanvas = Magick::Geometry(width, height);
				customCanvas.aspect(true);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"SPREADMETHOD") == 0)
		{
			const int val = MathParser::ParseInt(tempPara);
            if (val >= 1 && val <= 3)
            {
                spread = static_cast<MagickCore::SpreadMethod>(val);
            }
			else
			{
				spread = MagickCore::PadSpread;
				RmLogF(rm, 2, L"%s is invalid SpreadMethod. Pad Spread is applied", tempPara);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTANGLE") == 0)
		{
            const double val = MathParser::ParseDouble(tempPara);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00537
			out.img.artifact("gradient:angle", std::to_string(val));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTEXTENT") == 0)
		{
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00560
			out.img.artifact("gradient:extent", Utils::WStringToString(tempPara));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTRADII") == 0)
		{
			WSVector valList = Utils::SeparateList(tempPara, L",", 2);
            const int radiiX = MathParser::ParseInt(valList[0]);
            const int radiiY = MathParser::ParseInt(valList[1]);
            std::ostringstream radii;
			radii << radiiX << "," << radiiY;
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00593
			out.img.artifact("gradient:radii", radii.str());
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTVECTOR") == 0)
		{
			WSVector valList = Utils::SeparateList(tempPara, L",", 4);
			const int X1 = MathParser::ParseInt(valList[0]);
			const int Y1 = MathParser::ParseInt(valList[1]);
            const int X2 = MathParser::ParseInt(valList[2]);
            const int Y2 = MathParser::ParseInt(valList[3]);
            std::ostringstream gradVector;
            gradVector << X1 << "," << Y1 << "," << X2 << "," << Y2;
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00522
			out.img.artifact("gradient:vector", gradVector.str());
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLORLIST") == 0)
		{
			std::wstring rawColorList = RmReadString(rm, tempPara.c_str(), L"");
			WSVector colorList = Utils::SeparateList(rawColorList, L"|", NULL);
			if (colorList.size() < 1)
            {
                RmLogF(rm, 2, L"%s is invalid ColorList.", tempPara);
                return FALSE;
            }

			for (auto &colorRaw : colorList)
			{
				WSVector colorStop = Utils::SeparateList(colorRaw, L";", 2);
				if (!colorStop[0].empty() && !colorStop[1].empty())
				{
                    MagickCore::StopInfo s{
                        Utils::ParseColor(colorStop[0]),
                        MathParser::ParseDouble(colorStop[1])
                    };
					colorMap.push_back(s);
				}
			}

			totalColor = colorMap.size();

			isSetting = TRUE;
		}

		if (isSetting)
			option.clear();
	}

	if (totalColor == 0)
    {
        RmLog(rm, 2, L"No valid color was found. Check ColorList again.");
        return FALSE;
    }

	out.img.scale(customCanvas);
    const auto drawSucceeced = MagickCore::GradientImage(
		out.img.image(),
		type, spread,
        &colorMap[0], // convert vector to array
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

bool CompareStop(MagickCore::StopInfo i, MagickCore::StopInfo j) noexcept {
    return (i.offset < j.offset);
}

BOOL CreateConicalGradient(Measure * measure, WSVector &config, ImgContainer &out)
{
	Magick::DrawableList drawList;
	Magick::Geometry customCanvas;
	std::vector <MagickCore::StopInfo> colorMap;

	for (auto &option : config)
	{
        if (option.empty())
            continue;

		std::wstring tempName, tempPara;
		Utils::GetNamePara(option, tempName, tempPara);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempPara.c_str();

		BOOL isSetting = FALSE;

		if (_wcsicmp(name, L"CANVAS") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
            const size_t width = MathParser::ParseSizeT(valList[0]);
            const size_t height = MathParser::ParseSizeT(valList[1]);

			if (width <= 0 || height <= 0)
			{
				RmLog(2, L"Invalid Width or Height value. Default canvas 600,600 is used.");
				customCanvas = Magick::Geometry(600, 600);
			}
			else
			{
				customCanvas = Magick::Geometry(width, height);
				customCanvas.aspect(true);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLORLIST") == 0)
		{
			std::wstring rawColorList = RmReadString(measure->rm, parameter, L"");
			WSVector colorList = Utils::SeparateList(rawColorList, L"|", NULL);
			if (colorList.size() > 0)
			{
				for (auto &colorRaw : colorList)
				{
					WSVector colorStop = Utils::SeparateList(colorRaw, L";", 2);
					if (!colorStop[0].empty() && !colorStop[1].empty())
					{
						const double offset = MathParser::ParseDouble(colorStop[1]);
						if (offset >= 0 && offset <= 1)
						{
                            MagickCore::StopInfo s{
                                Utils::ParseColor(colorStop[0]),
                                offset
                            };
							colorMap.push_back(s);
						}
					}
				}
			}
			else
			{
				RmLogF(measure->rm, 2, L"%s is invalid ColorList.", parameter);
				return FALSE;
			}
			isSetting = TRUE;
		}

		if (isSetting)
            option.clear();
	}

	const double maxSize = round(sqrt(customCanvas.height() * customCanvas.height() + customCanvas.width() * customCanvas.width()) / 2);
    const double centerX = (double)customCanvas.width() / 2;
    const double centerY = (double)customCanvas.height() / 2;

	Magick::PathMovetoAbs centerPoint(Magick::Coordinate(centerX, centerY));

	std::sort(colorMap.begin(), colorMap.end(), CompareStop);

    const size_t totalColor = colorMap.size();
	if (totalColor == 0)
	{
		RmLog(measure->rm, LOG_ERROR, L"No valid color was found. Check ColorList again.");
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

	out.img.scale(customCanvas);
	out.img.draw(drawList);
	out.img.blur(100, 100);
	return TRUE;
}