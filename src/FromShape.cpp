#include "MagickMeter.h"

struct ShapeEllipse
{
	double x = 0;
	double y = 0;
	double radiusX = 0;
	double radiusY = 0;
	auto Create(void) 
	{ 
		radiusY = radiusY == 0 ? radiusX : radiusY;
		return Magick::DrawableEllipse(x, y, radiusX, radiusY, 0, 360);
	}
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

BOOL CreateShape(ImgStruct &dst, WSVector &setting, ImgType shape, Measure * measure)
{
	std::wstring parameter = setting[0];
	setting.erase(setting.begin());

	std::vector<Magick::Drawable> drawList;
	Magick::Drawable mainShape;
	WSVector paraList;

	double width = 0;
	double height = 0;
	Magick::Geometry customCanvas;
	switch (shape)
	{
	case ELLIPSE:
	{
		paraList = SeparateList(parameter, L",", 4);
		ShapeEllipse e;
		e.x = MathParser::ParseF(paraList[0]);
		e.y = MathParser::ParseF(paraList[1]);
		e.radiusX = MathParser::ParseF(paraList[2]);
		e.radiusY = MathParser::ParseF(paraList[3]);
		mainShape = e.Create();
		width = e.x + abs(e.radiusX);
		height = e.y + abs(e.radiusY);

		break;
	}
	case RECTANGLE:
	{
		paraList = SeparateList(parameter, L",", 6);
		ShapeRectangle r;
		r.x = MathParser::ParseF(paraList[0]);
		r.y = MathParser::ParseF(paraList[1]);
		r.w = MathParser::ParseF(paraList[2]);
		r.h = MathParser::ParseF(paraList[3]);
		r.cornerX = MathParser::ParseF(paraList[4]);
		r.cornerY = MathParser::ParseF(paraList[5]);
		if (r.cornerX == 0 && r.cornerY == 0)
			mainShape =  r.Create();
		else
			mainShape = r.CreateRound();

		if (r.w > 0)
			width = r.w + r.x;
		else
			width = r.x;

		if (r.h > 0)
			height = r.h + r.y;
		else
			height = r.y;

		break;
	}
	}

	int strokeAlign = 0;
	double strokeWidth = 0;

	//Default color
	drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));
	Magick::Drawable strokeColor = Magick::DrawableStrokeColor(Magick::Color("black"));

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
				RmLog(2, L"Invalid Width or Height value. Default canvas is used.");
			else
			{
				customCanvas = Magick::Geometry((size_t)tempW, (size_t)tempH);
				customCanvas.aspect(true);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"MOVE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			drawList.push_back(Magick::DrawableTranslation(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ROTATE") == 0)
		{
			drawList.push_back(Magick::DrawableRotation(
				MathParser::ParseF(parameter)
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLOR") == 0)
		{
			drawList.push_back(Magick::DrawableFillColor(GetColor(parameter)));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ANTIALIAS") == 0)
		{
			drawList.push_back(Magick::DrawableStrokeAntialias(MathParser::ParseI(parameter) == 1));
			isSetting = TRUE;
		}
		if (_wcsicmp(name, L"STROKECOLOR") == 0)
		{
			strokeColor = Magick::DrawableStrokeColor(GetColor(parameter));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEWIDTH") == 0)
		{
			strokeWidth = MathParser::ParseF(parameter);
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth));
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
				RmLog(2, L"Invalid StrokeAlign value. Center align is applied.");
			}
			isSetting = TRUE;
		}
		if (isSetting)
			setting[i] = L"";
	}

	try
	{
		if (customCanvas.isValid())
		{
			dst.contain.size(customCanvas);
		}
		else
			dst.contain.size(Magick::Geometry((size_t)(width + strokeWidth), (size_t)(height + strokeWidth)));
		drawList.push_back(mainShape);
		if (strokeWidth == 0)
		{
			drawList.push_back(Magick::DrawableStrokeColor(INVISIBLE));
			dst.contain.draw(drawList);
		}
		else if (strokeWidth != 0 && strokeAlign != 0)
		{
			drawList.push_back(strokeColor);
			Magick::Image temp = dst.contain;
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth * 2));
			dst.contain.draw(drawList);

			drawList.push_back(Magick::DrawableStrokeWidth(0));
			temp.draw(drawList);
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
			drawList.push_back(strokeColor);
			dst.contain.draw(drawList);
		}
	}
	catch (Magick::Exception &error_)
	{
		error2pws(error_);
		return FALSE;
	}

	return TRUE;
}