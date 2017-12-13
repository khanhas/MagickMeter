#include "MagickMeter.h"

#define Magick2PI	6.28318530717958647692528676655900576839433879875020
BOOL CreateConicalGradient(ImgStruct &dst, WSVector &setting, Measure * measure);

BOOL CreateGradient(ImgStruct &dst, WSVector &setting, Measure * measure)
{

	MagickCore::GradientType type;
	if (_wcsicmp(setting[0].c_str(), L"LINEAR") == 0)
		type = MagickCore::LinearGradient;
	else if (_wcsicmp(setting[0].c_str(), L"RADIAL") == 0)
		type = MagickCore::RadialGradient;
	else if (_wcsicmp(setting[0].c_str(), L"CONICAL") == 0)
	{
		return CreateConicalGradient(dst, setting, measure);
	}
	else
	{
		RmLogF(measure->rm, 2, L"%s is invalid Gradient type.", setting[0].c_str());
		return FALSE;
	}
	setting.erase(setting.begin());
	MagickCore::SpreadMethod spread = MagickCore::PadSpread;

	MagickCore::StopInfo * stopColors = { 0 };
	size_t totalColor = 0;

	Magick::Geometry customCanvas = Magick::Geometry(600, 600);

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
			{
				RmLog(2, L"Invalid Width or Height value. Default canvas 600,600 is used.");
				customCanvas = Magick::Geometry(600, 600);
			}
			else
			{
				customCanvas = Magick::Geometry((size_t)tempW, (size_t)tempH);
				customCanvas.aspect(true);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"SPREADMETHOD") == 0)
		{
			int val = MathParser::ParseI(parameter);
			if (val >= 1 && val <= 3)
				spread = (MagickCore::SpreadMethod)val;
			else
			{
				spread = MagickCore::PadSpread;
				RmLogF(measure->rm, 2, L"%s is invalid SpreadMethod. Pad Spread is applied", parameter);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTANGLE") == 0)
		{
			double val = MathParser::ParseF(parameter);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00537
			dst.contain.artifact("gradient:angle", std::to_string(val).c_str());
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTEXTENT") == 0)
		{
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00560
			dst.contain.artifact("gradient:extent", ws2s(parameter).c_str());
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTRADII") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			int radiiX = MathParser::ParseI(valList[0]);
			int radiiY = MathParser::ParseI(valList[1]);
			std::string radii = std::to_string(radiiX) + "," + std::to_string(radiiY);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00593
			dst.contain.artifact("gradient:radii", radii.c_str());
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"GRADIENTVECTOR") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 4);
			int X1 = MathParser::ParseI(valList[0]);
			int Y1 = MathParser::ParseI(valList[1]);
			int X2 = MathParser::ParseI(valList[2]);
			int Y2 = MathParser::ParseI(valList[3]);
			std::string gradVector = std::to_string(X1) + "," + std::to_string(Y1) + "," + std::to_string(X2) + "," + std::to_string(Y2);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00522
			dst.contain.artifact("gradient:vector", gradVector.c_str());
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLORLIST") == 0)
		{
			WSVector colorList = SeparateList(RmReadString(measure->rm, parameter, L""), L"|", NULL);
			if (colorList.size() > 0)
			{
				std::vector <MagickCore::StopInfo> colorMap;
				for (auto &colorRaw : colorList)
				{
					WSVector colorStop = SeparateList(colorRaw, L";", 2);
					MagickCore::StopInfo s;
					if (!colorStop[0].empty() && !colorStop[1].empty())
					{
						s.color = GetColor(colorStop[0]);
						s.offset = MathParser::ParseF(colorStop[1]);
						colorMap.push_back(s);
					}
				}

				stopColors = &colorMap[0]; //convert vector to array	
				totalColor = colorMap.size();
			}
			else
			{
				RmLogF(measure->rm, 2, L"%s is invalid ColorList.", parameter);
				return FALSE;
			}
			isSetting = TRUE;
		}

		if (isSetting)
			setting[i] = L"";
	}


	if (totalColor != 0)
	{
		dst.contain.scale(customCanvas);
		MagickCore::MagickBooleanType drawSucceeced = MagickCore::GradientImage(
			dst.contain.image(),
			type, spread,
			stopColors, totalColor,
			nullptr
		);

		stopColors = nullptr;
		if (drawSucceeced == MagickCore::MagickBooleanType::MagickFalse)
		{
			RmLog(2, L"Could not draw Gradient");
			return FALSE;
		}
	}
	else
	{
		RmLog(measure->rm, 2, L"No valid color was found. Check ColorList again.");
		return FALSE;
	}

	return TRUE;
}

bool CompareStop(MagickCore::StopInfo i, MagickCore::StopInfo j) { return (i.offset < j.offset); }

BOOL CreateConicalGradient(ImgStruct &dst, WSVector &setting, Measure * measure)
{
	setting.erase(setting.begin());
	Magick::DrawableList drawList;
	Magick::Geometry customCanvas;
	std::vector <MagickCore::StopInfo> colorMap;

	for (int i = 0; i < setting.size(); i++)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(setting[i], tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();

		BOOL isSetting = FALSE;

		if (_wcsicmp(name, L"CANVAS") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			int tempW = MathParser::ParseI(valList[0]);
			int tempH = MathParser::ParseI(valList[1]);

			if (tempW <= 0 || tempH <= 0)
			{
				RmLog(2, L"Invalid Width or Height value. Default canvas 600,600 is used.");
				customCanvas = Magick::Geometry(600, 600);
			}
			else
			{
				customCanvas = Magick::Geometry((size_t)tempW, (size_t)tempH);
				customCanvas.aspect(true);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLORLIST") == 0)
		{
			WSVector colorList = SeparateList(RmReadString(measure->rm, parameter, L""), L"|", NULL);
			if (colorList.size() > 0)
			{
				for (auto &colorRaw : colorList)
				{
					WSVector colorStop = SeparateList(colorRaw, L";", 2);
					MagickCore::StopInfo s;
					if (!colorStop[0].empty() && !colorStop[1].empty())
					{
						double o = MathParser::ParseF(colorStop[1]);
						if (o >= 0 && o <= 1)
						{
							s.color = GetColor(colorStop[0]);
							s.offset = o;
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
			setting[i] = L"";
	}

	double maxSize = round(sqrt(customCanvas.height() * customCanvas.height() + customCanvas.width() * customCanvas.width()) / 2);
	double centerX = (double)customCanvas.width() / 2;
	double centerY = (double)customCanvas.height() / 2;

	Magick::PathMovetoAbs centerPoint(Magick::Coordinate(centerX, centerY));

	std::sort(colorMap.begin(), colorMap.end(), CompareStop);

	size_t totalColor = colorMap.size();
	if (totalColor == 0)
	{
		RmLog(measure->rm, 2, L"No valid color was found. Check ColorList again.");
		return FALSE;
	}

	for (int i = 0; i < totalColor; i++)
	{
		Magick::Color c1;
		double start;
		if (i == 0)
		{
			start = colorMap[totalColor - 1].offset - 1;
			c1 = colorMap[totalColor - 1].color;
		}
		else
		{
			start = colorMap[i - 1].offset;
			c1 = colorMap[i - 1].color;
		}

		Magick::Color c2  = colorMap[i].color;
		double end = colorMap[i].offset;

		double half = (end - start) / 2 + start;
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

	dst.contain.scale(customCanvas);
	dst.contain.draw(drawList);
	dst.contain.blur(100, 100);
	return TRUE;
}