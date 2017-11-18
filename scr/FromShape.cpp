#include "MagickMeter.h"

struct ShapeEllipse 
{
	double x = 0;
	double y = 0;
	double radiusX = 0;
	double radiusY = 0;
	auto Create(void) { return Magick::DrawableEllipse(x, y, radiusX, radiusY == 0 ? radiusX : radiusY, 0, 360); }
};

struct ShapeRectangle
{
	double x = 0;
	double y = 0;
	double w = 0;
	double h = 0;
	double cornerX = 0;
	double cornerY = 0;
	auto Create(void)		{ return Magick::DrawableRectangle(x, y, x + w, y + h); }
	auto CreateRound(void)	{ return Magick::DrawableRoundRectangle(x, y, x + w, y + h, cornerX, cornerY == 0 ? cornerX : cornerY); }
};

BOOL CreateShape(ImgStruct * dst, std::vector<std::wstring> setting, ImgType shape, Measure * measure)
{
	std::vector<Magick::Drawable> drawList;
	std::vector<std::wstring> paraList;
	switch (shape)
	{
	case ELLIPSE:
	{
		std::wstring parameter = setting[0].substr(setting[0].find_first_not_of(L" \t\r\n", 7));
		paraList = SeparateList(parameter.c_str(), L",", 4);
		ShapeEllipse e;
		MathParser::CheckedParse(paraList[0], &e.x);
		MathParser::CheckedParse(paraList[1], &e.y);
		MathParser::CheckedParse(paraList[2], &e.radiusX);
		MathParser::CheckedParse(paraList[3], &e.radiusY);
		drawList.push_back(e.Create());
		break;
	}
	case RECTANGLE:
	{
		std::wstring parameter = setting[0].substr(setting[0].find_first_not_of(L" \t\r\n", 9));
		paraList = SeparateList(parameter.c_str(), L",", 6);
		ShapeRectangle r;
		MathParser::CheckedParse(paraList[0], &r.x);
		MathParser::CheckedParse(paraList[1], &r.y);
		MathParser::CheckedParse(paraList[2], &r.w);
		MathParser::CheckedParse(paraList[3], &r.h);
		MathParser::CheckedParse(paraList[4], &r.cornerX);
		MathParser::CheckedParse(paraList[5], &r.cornerY);
		if (r.cornerX == 0 && r.cornerY == 0)
			drawList.push_back(r.Create());
		else
			drawList.push_back(r.CreateRound());
		break;
	}
	}

	setting.erase(setting.begin());

	size_t width = 600;
	size_t height = 600;

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
			int tempW = _wtoi(valList[0].c_str());
			int tempH = _wtoi(valList[1].c_str());

			if (tempW <= 0 || tempH <= 0)
				RmLog(2, L"Invalid Width or Height value. Default canvas 600x600 is used.");
			else
			{
				width = tempW;
				height = tempH;
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLOR") == 0)
		{
			drawList.push_back(Magick::DrawableFillColor(GetColor(parameter)));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ANTIALIAS") == 0)
		{
			drawList.push_back(Magick::DrawableStrokeAntialias(_wtoi(parameter) == 1));
			isSetting = TRUE;
		}
		if (_wcsicmp(name, L"STROKECOLOR") == 0)
		{
			drawList.push_back(Magick::DrawableStrokeColor(GetColor(parameter)));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEWIDTH") == 0)
		{
			drawList.push_back(Magick::DrawableStrokeWidth(_wtof(parameter)));
			isSetting = TRUE;
		}

		if (isSetting)
			setting.erase(setting.begin() + i);
	}
	dst->contain = Magick::Image(Magick::Geometry(width, height), Magick::ColorRGB(1, 1, 1, 0));

	try
	{
		dst->contain.draw(drawList);
	}
	catch (std::exception &error_)
	{
		RmLogF(measure->rm, 1, L"%s", error2pws(error_));
		return FALSE;
	}

	for (auto &settingIt : setting)
	{
		std::wstring name, parameter;
		GetNamePara(settingIt, name, parameter);

		if (!ParseEffect(measure->rm, dst->contain, name, parameter))
			return FALSE;
	}

	return TRUE;
}