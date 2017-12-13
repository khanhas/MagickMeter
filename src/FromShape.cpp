#include "MagickMeter.h"

struct ShapeEllipse
{
	double x = 0;
	double y = 0;
	double radiusX = 0;
	double radiusY = 0;
	double start = 0;
	double end = 360;
	auto Create(void) { return Magick::DrawableEllipse(x, y, radiusX, radiusY, start, end); }
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
	auto CreateRound(void)	{ return Magick::DrawableRoundRectangle(x, y, x + w, y + h, cornerX, cornerY); }
};

auto ParseNumber = [](auto var, const WCHAR* value, auto* func) -> decltype(var)
{
	if (_wcsnicmp(value, L"*", 1) == 0) return var;
	return (decltype(var))func(value);
};

void ParseBool(bool &var, std::wstring value)
{
	if (_wcsnicmp(value.c_str(), L"*", 1) != 0)
		var = (bool)MathParser::ParseB(value);
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
		paraList = SeparateParameter(parameter, 6, L"");
		ShapeEllipse e;
		e.x = MathParser::ParseF(paraList[0]);
		e.y = MathParser::ParseF(paraList[1]);
		e.radiusX = MathParser::ParseF(paraList[2]);

		if (!paraList[3].empty())
			e.radiusY = MathParser::ParseF(paraList[3]);
		
		if (e.radiusY == 0)
			e.radiusY = e.radiusX;

		if (!paraList[4].empty()) e.start = MathParser::ParseF(paraList[4]);

		if (!paraList[5].empty()) e.end = MathParser::ParseF(paraList[5]);

		mainShape = e.Create();
		width = e.x + abs(e.radiusX);
		height = e.y + abs(e.radiusY);

		break;
	}
	case RECTANGLE:
	{
		paraList = SeparateParameter(parameter, 6, L"");
		ShapeRectangle r;
		r.x = MathParser::ParseF(paraList[0]);
		r.y = MathParser::ParseF(paraList[1]);
		r.w = MathParser::ParseF(paraList[2]);
		r.h = MathParser::ParseF(paraList[3]);

		if (!paraList[4].empty()) r.cornerX = MathParser::ParseF(paraList[4]);

		if (!paraList[5].empty()) r.cornerY = MathParser::ParseF(paraList[5]);

		if (r.cornerY == 0)
			r.cornerY = r.cornerX;

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
	case PATH:
	{
		parameter = RmReadString(measure->rm, parameter.c_str(), L"");
		if (!parameter.empty())
		{
			WSVector pathList = SeparateList(parameter, L"|", NULL);

			Magick::VPathList p;
			double curX = 0.0;
			double curY = 0.0;

			//Start point
			WSVector s = SeparateParameter(pathList[0], NULL);
			if (s.size() >= 2)
			{
				curX = MathParser::ParseF(s[0]);
				curY = MathParser::ParseF(s[1]);
				p.push_back(Magick::PathMovetoAbs(Magick::Coordinate(
					curX,
					curY
				)));
			}
			else
			{
				RmLogF(measure->rm, 2, L"%s is invalid start point.", pathList[0].c_str());
				return FALSE;
			}
			
			for (auto &path : pathList)
			{
				std::wstring tempName, tempParameter;
				GetNamePara(path, tempName, tempParameter);
				LPCWSTR name = tempName.c_str();
				LPCWSTR parameter = tempParameter.c_str();

				if (_wcsicmp(name, L"LINETO") == 0)
				{
					WSVector l = SeparateParameter(parameter, NULL);
					if (l.size() >= 2)
					{
						curX = MathParser::ParseF(l[0]);
						curY = MathParser::ParseF(l[1]);
						p.push_back(Magick::PathLinetoAbs(Magick::Coordinate(
							curX,
							curY
						)));
					}
				}
				else if (_wcsicmp(name, L"ARCTO") == 0)
				{
					WSVector l = SeparateParameter(parameter, NULL);
					size_t pSize = l.size();
					if (pSize >= 2)
					{
						double x = MathParser::ParseF(l[0].c_str());
						double y = MathParser::ParseF(l[1].c_str());
						double dx = x - curX;
						double dy = y - curY;
						double xRadius = std::sqrt(dx * dx + dy * dy) / 2.0;
						double angle = 0.0;
						bool sweep = false;
						bool size = false;

						if (pSize >= 3) xRadius = MathParser::ParseF(l[2].c_str());

						double yRadius = xRadius;

						if (pSize >= 4) yRadius = ParseNumber(yRadius, l[3].c_str(), MathParser::ParseF);
						if (pSize >= 5) angle = ParseNumber(angle, l[4].c_str(), MathParser::ParseF);
						if (pSize >= 6) ParseBool(sweep, l[5].c_str());
						if (pSize >= 7) ParseBool(size, l[6].c_str());

						p.push_back(Magick::PathArcAbs(Magick::PathArcArgs(
							xRadius, yRadius,
							angle, size, !sweep,
							x, y
						)));
						
						curX = x;
						curY = y;
					}
				}
				else if (_wcsicmp(name, L"CURVETO") == 0)
				{
					WSVector l = SeparateParameter(parameter, NULL);
					size_t pSize = l.size();
					if (pSize < 4)
					{
						RmLogF(measure->rm, 2, L"%s %s is invalid path segment.", name, parameter);
						continue;
					}
					double x = MathParser::ParseF(l[0].c_str());
					double y = MathParser::ParseF(l[1].c_str());
					double cx1 = MathParser::ParseF(l[2].c_str());
					double cy1 = MathParser::ParseF(l[3].c_str());
					if (pSize < 6)
						p.push_back(Magick::PathQuadraticCurvetoAbs(Magick::PathQuadraticCurvetoArgs(
							cx1, cy1, x, y
						)));
					else
					{
						double cx2 = MathParser::ParseF(l[4].c_str());
						double cy2 = MathParser::ParseF(l[5].c_str());
						p.push_back(Magick::PathCurvetoAbs(Magick::PathCurvetoArgs(
							cx1, cy1, cx2, cy2, x, y
						)));
					}
					curX = x;
					curY = y;
				}
				else if (_wcsicmp(name, L"CLOSEPATH") == 0)
				{
					if (MathParser::ParseB(parameter))
						p.push_back(Magick::PathClosePath());
				}
			}
			mainShape = Magick::DrawablePath(p);
			
		}
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
		else if (_wcsicmp(name, L"OFFSET") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
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
		else if(_wcsicmp(name, L"FILLPATTERN") == 0)
		{
			int index = NameToIndex(parameter);
			if (index >= 0 && index < measure->imgList.size())
				dst.contain.fillPattern(measure->imgList[index].contain);
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
		else if (_wcsicmp(name, L"STROKELINEJOIN") == 0)
		{
			Magick::LineJoin lj;
			if (_wcsicmp(parameter, L"MITER") == 0)
				lj = MagickCore::MiterJoin;
			else if (_wcsicmp(parameter, L"ROUND") == 0)
				lj = MagickCore::RoundJoin;
			else if (_wcsicmp(parameter, L"BEVEL") == 0)
				lj = MagickCore::BevelJoin;
			else
			{
				lj = MagickCore::MiterJoin;
				RmLog(2, L"Invalid StrokeLineJoin value. Miter line join is applied.");
			}
			drawList.push_back(Magick::DrawableStrokeLineJoin(lj));
		}
		else if (_wcsicmp(name, L"STROKELINECAP") == 0)
		{
			Magick::LineCap lc;
			if (_wcsicmp(parameter, L"FLAT") == 0)
				lc = MagickCore::ButtCap;
			else if (_wcsicmp(parameter, L"ROUND") == 0)
				lc = MagickCore::RoundCap;
			else if (_wcsicmp(parameter, L"SQUARE") == 0)
				lc = MagickCore::SquareCap;
			else
			{
				lc = MagickCore::ButtCap;
				RmLog(2, L"Invalid StrokeLineCap value. Flat line cap is applied.");
			}
			drawList.push_back(Magick::DrawableStrokeLineCap(lc));
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
		{
			Magick::Geometry newSize((size_t)(width + strokeWidth), (size_t)(height + strokeWidth));
			newSize.aspect(true);
			dst.contain.scale(newSize);
		}

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