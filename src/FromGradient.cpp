#include "MagickMeter.h"

MagickCore::PixelInfo GetPixelInfo(std::wstring rawString);

BOOL CreateGradient(ImgStruct * dst, WSVector setting, Measure * measure)
{
	dst->contain = Magick::Image(Magick::Geometry(600,600), INVISIBLE);

	MagickCore::GradientType type;
	if (_wcsicmp(setting[0].c_str(), L"LINEAR") == 0)
		type = MagickCore::LinearGradient;
	else if (_wcsicmp(setting[0].c_str(), L"RADIAL") == 0)
		type = MagickCore::RadialGradient;
	else
	{
		RmLogF(measure->rm, 2, L"%s is invalid GradientType.", setting[0].c_str());
		return FALSE;
	}
	setting.erase(setting.begin());
	MagickCore::SpreadMethod spread = MagickCore::PadSpread;

	MagickCore::StopInfo * stopColors = { 0 };
	size_t totalColor = 0;
	for (int i = 0; i < setting.size(); i++)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(setting[i], tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();

		BOOL isSetting = FALSE;

		if (_wcsicmp(name, L"SPREADMETHOD") == 0)
		{
			int val = MathParser::ParseI(parameter);
			if (val >= 1 && val <= 3)
				spread = (MagickCore::SpreadMethod)val;
			else
			{
				spread = MagickCore::PadSpread;
				RmLogF(measure->rm, 2, L"%s is invalid SpreadMethod. Pad Spread is applied", parameter);
			}
		}
		else if (_wcsicmp(name, L"GRADIENTANGLE") == 0)
		{
			double val = MathParser::ParseF(parameter);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00537
			MagickCore::SetImageArtifact(dst->contain.image(), "gradient:angle", std::to_string(val).c_str());
		}
		else if (_wcsicmp(name, L"GRADIENTRADII") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			int radiiX = MathParser::ParseI(valList[0]);
			int radiiY = MathParser::ParseI(valList[1]);
			std::string radii = std::to_string(radiiX) + "," + std::to_string(radiiY);
			//https://www.imagemagick.org/api/MagickCore/paint_8c_source.html#l00593
			MagickCore::SetImageArtifact(dst->contain.image(), "gradient:radii", radii.c_str());
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
			MagickCore::SetImageArtifact(dst->contain.image(), "gradient:vector", gradVector.c_str());
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
		}
		setting[i] = L"";
	}
	if (totalColor != 0)
		MagickCore::GradientImage(dst->contain.image(), type, spread, stopColors, totalColor, nullptr);
	else
	{
		RmLog(2, L"No valid color was found. Check ColorList again.");
		return FALSE;
	}
	return TRUE;
}

MagickCore::PixelInfo GetPixelInfo(std::wstring rawString)
{
	Magick::Color from = GetColor(rawString);
	MagickCore::PixelInfo to;
	to.blue = from.quantumBlue();
	to.green = from.quantumGreen();
	to.red = from.quantumRed();
	to.alpha = from.quantumAlpha();
	return to;
}