#include "MagickMeter.h"


double GetLength(double dx, double dy);
Magick::Coordinate GetProportionPoint(Magick::Coordinate point, double segment,
	double length, double dx, double dy);
void DrawRoundedCorner(Magick::VPathList &drawList, Magick::Coordinate &angularPoint,
	Magick::Coordinate &p1, Magick::Coordinate &p2, double radius, bool start);

struct ShapeEllipse
{
	double x = 0;
	double y = 0;
	double radiusX = 0;
	double radiusY = 0;
	double start = 0;
	double end = 360;
	auto Create(void) { 
		//Because of Antialias, ellipse exceeds its actual size 1 pixel.
		return Magick::DrawableEllipse(x, y, radiusX - (radiusX == 0 ? 0 : 0.5), radiusY - (radiusY == 0 ? 0 : 0.5), start, end);
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
	auto CreateRound(void)	{ return Magick::DrawableRoundRectangle(x, y, x + w, y + h, cornerX, cornerY); }
};

class PathCubicSmoothCurvetoAbs : public Magick::VPathBase
{
public:
	// Draw a single curve
	PathCubicSmoothCurvetoAbs(const Magick::PathQuadraticCurvetoArgs &args_)
	: _args(1, args_) {}
	// Draw multiple curves
	PathCubicSmoothCurvetoAbs(const Magick::PathQuadraticCurvetoArgsList &args_)
	: _args(args_) {}

	// Copy constructor
	PathCubicSmoothCurvetoAbs(const PathCubicSmoothCurvetoAbs& original_)
	: VPathBase(original_), _args(original_._args) {}

	/*virtual*/ ~PathCubicSmoothCurvetoAbs(void) {};

	// Operator to invoke equivalent draw API call
	/*virtual*/ void operator()(MagickCore::DrawingWand *context_) const
	{
		for (Magick::PathQuadraticCurvetoArgsList::const_iterator p = _args.begin();
			p != _args.end(); p++)
		{
			DrawPathCurveToSmoothAbsolute(context_, p->x1(), p->y1(), p->x(), p->y());
		}
	}

	// Return polymorphic copy of object
	/*virtual*/
	VPathBase* copy() const
	{
		return new PathCubicSmoothCurvetoAbs(*this);
	}

private:
	Magick::PathQuadraticCurvetoArgsList _args;
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

	double strokeWidth = 0;

	Magick::Geometry customCanvas;
	switch (shape)
	{
	case ELLIPSE:
	{
		paraList = SeparateParameter(parameter, NULL);
		size_t pSize = paraList.size();
		if (pSize < 3)
		{
			RmLog(2, L"Ellipse: Not enough parameter.");
			return FALSE;
		}
		ShapeEllipse e;
		e.x = MathParser::ParseF(paraList[0]);
		e.y = MathParser::ParseF(paraList[1]);
		e.radiusX = MathParser::ParseF(paraList[2]);

		e.radiusY = e.radiusX;
		if (pSize > 3) e.radiusY = MathParser::ParseF(paraList[3]);

		if (pSize > 4) e.start = MathParser::ParseF(paraList[4]);

		if (pSize > 5) e.end = MathParser::ParseF(paraList[5]);

		mainShape = e.Create();
		drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));

		width = e.x + abs(e.radiusX);
		height = e.y + abs(e.radiusY);

		dst.X = (ssize_t)(e.x - abs(e.radiusX));
		dst.Y = (ssize_t)(e.y - abs(e.radiusY));
		dst.W = (size_t)(e.radiusX * 2.0);
		dst.H = (size_t)(e.radiusY * 2.0);

		break;
	}
	case RECTANGLE:
	{
		paraList = SeparateParameter(parameter, NULL);
		size_t pSize = paraList.size();
		if (pSize < 4)
		{
			RmLog(2, L"Rectangle: Not enough parameter.");
			return FALSE;
		}

		ShapeRectangle r;
		r.x = MathParser::ParseF(paraList[0]);
		r.y = MathParser::ParseF(paraList[1]);
		r.w = MathParser::ParseF(paraList[2]);
		r.h = MathParser::ParseF(paraList[3]);

		if (pSize > 4) r.cornerX = MathParser::ParseF(paraList[4]);

		r.cornerY = r.cornerX;
		if (pSize > 5) r.cornerY = MathParser::ParseF(paraList[5]);

		if (r.cornerX == 0 && r.cornerY == 0)
			mainShape = r.Create();
		else
			mainShape = r.CreateRound();

		drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));

		if (r.w > 0)
		{
			width = r.w + r.x;
			dst.X = (ssize_t)r.x;
		}
		else
		{
			width = r.x;
			dst.X = (ssize_t)(r.x + r.w);
		}
		dst.W = (size_t)abs(r.w);

		if (r.h > 0)
		{
			height = r.h + r.y;
			dst.Y = (ssize_t)r.y;
		}
		else
		{
			height = r.y;
			dst.Y = (ssize_t)(r.y + r.h);
		}
		dst.H = (size_t)abs(r.h);
		break;
	}
	case POLYGON:
	{
		paraList = SeparateParameter(parameter, NULL);
		size_t listSize = paraList.size();
		if (listSize < 4)
		{
			RmLogF(measure->rm, 2, L"Polygon %s: Not enough parameter", parameter.c_str());
			return FALSE;
		}
		double origX = MathParser::ParseF(paraList[0]);
		double origY = MathParser::ParseF(paraList[1]);
		double side = round(MathParser::ParseF(paraList[2]));
		if (side < 3)
		{
			RmLogF(measure->rm, 2, L"Polygon %s: Invalide number of sides.", parameter.c_str());
			return FALSE;
		}
		double radiusX = abs(MathParser::ParseF(paraList[3]));
		double radiusY = radiusX;
		if (listSize > 4) radiusY = abs(MathParser::ParseF(paraList[4]));
		double roundCorner = 0;
		if (listSize > 5) roundCorner = abs(MathParser::ParseF(paraList[5]));
		double startAngle = 0;
		if (listSize > 6) startAngle = MathParser::ParseF(paraList[6]) * MagickPI / 180;

		Magick::VPathList p;
		if (roundCorner != 0)
		{
			Magick::Coordinate c1(
				origX + radiusX * cos(MagickPI2 + startAngle),
				origY - radiusY * sin(MagickPI2 + startAngle)
			);
			double fac = 1 / side;
			for (int i = 1; i <= side; i++)
			{
				double a = (double)i * fac;
				Magick::Coordinate angPoint(
					origX + radiusX * cos(a * Magick2PI + MagickPI2 + startAngle),
					origY - radiusY * sin(a * Magick2PI + MagickPI2 + startAngle)
				);
				a = ((double)i + 1.0) * fac;
				Magick::Coordinate c2(
					origX + radiusX * cos(a * Magick2PI + MagickPI2 + startAngle),
					origY - radiusY * sin(a * Magick2PI + MagickPI2 + startAngle)
				);
				DrawRoundedCorner(p, angPoint, c1, c2, roundCorner, i == 1);
				c1 = angPoint;
			}
		}
		else
		{
			for (int i = 0; i < side; i++)
			{
				double a = (double)i / side;
				double curX = origX + radiusX * cos(a * Magick2PI + MagickPI2 + startAngle);
				double curY = origY - radiusY * sin(a * Magick2PI + MagickPI2 + startAngle);
				RmLogF(measure->rm, 2, L"%f %f", curX, curY);
				if (i == 0)
					p.push_back(Magick::PathMovetoAbs(Magick::Coordinate(
						curX, curY
					)));
				else
					p.push_back(Magick::PathLinetoAbs(Magick::Coordinate(
						curX, curY
					)));
			}
		}
		p.push_back(Magick::PathClosePath());
		mainShape = Magick::DrawablePath(p);
		drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));
		width = origX + radiusX;
		height = origY + radiusY;
		dst.X = (ssize_t)round(origX - radiusX);
		dst.Y = (ssize_t)round(origY - radiusY);
		dst.W = (size_t)ceil(radiusX * 2);
		dst.H = (size_t)ceil(radiusY * 2);
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
			
			double imgX1 = curX;
			double imgY1 = curY;
			double imgX2 = curX;
			double imgY2 = curY;

			for (auto &path : pathList)
			{
				std::wstring tempName, parameter;
				GetNamePara(path, tempName, parameter);
				ParseInternalVariable(measure, parameter, dst);

				LPCWSTR name = tempName.c_str();

				if (_wcsicmp(name, L"LINETO") == 0)
				{
					WSVector l = SeparateParameter(parameter, NULL);
					if (l.size() > 1)
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

						if (pSize > 2) xRadius = MathParser::ParseF(l[2].c_str());

						double yRadius = xRadius;

						if (pSize > 3) yRadius = ParseNumber(yRadius, l[3].c_str(), MathParser::ParseF);
						if (pSize > 4) angle = ParseNumber(angle, l[4].c_str(), MathParser::ParseF);
						if (pSize > 5) ParseBool(sweep, l[5].c_str());
						if (pSize > 6) ParseBool(size, l[6].c_str());

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
						RmLogF(measure->rm, 2, L"%s %s is invalid CurveTo segment.", name, parameter);
						continue;
					}
					double x = MathParser::ParseF(l[0].c_str());
					double y = MathParser::ParseF(l[1].c_str());
					double cx1 = MathParser::ParseF(l[2].c_str());
					double cy1 = MathParser::ParseF(l[3].c_str());
					if (pSize < 6)
					{
						p.push_back(Magick::PathQuadraticCurvetoAbs(Magick::PathQuadraticCurvetoArgs(
							cx1, cy1, x, y
						)));
					}
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
				else if (_wcsicmp(name, L"SMOOTHCURVETO") == 0)
				{
					WSVector l = SeparateParameter(parameter, NULL);
					size_t pSize = l.size();
					if (pSize < 2)
					{
						RmLogF(measure->rm, 2, L"%s %s is invalid path segment.", name, parameter);
						continue;
					}
					double x = MathParser::ParseF(l[0].c_str());
					double y = MathParser::ParseF(l[1].c_str());

					if (pSize > 3)
					{
						double	cx = MathParser::ParseF(l[2].c_str());
						double	cy = MathParser::ParseF(l[3].c_str());
						
						p.push_back(PathCubicSmoothCurvetoAbs(Magick::PathQuadraticCurvetoArgs(
							cx, cy, x, y
						)));
					}
					else
					{
						p.push_back(Magick::PathSmoothQuadraticCurvetoAbs(Magick::Coordinate(
							x, y
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

				if (curX < imgX1) imgX1 = curX;
				if (curY < imgY1) imgY1 = curY;
				if (curX > imgX2) imgX2 = curX;
				if (curY > imgY2) imgY2 = curY;
			}
			mainShape = Magick::DrawablePath(p);
			drawList.push_back(Magick::DrawableFillColor(INVISIBLE));
			strokeWidth = 2;
			width = imgX2;
			height = imgY2;
			dst.X = (ssize_t)ceil(imgX1);
			dst.Y = (ssize_t)ceil(imgY1);
			dst.W = (size_t)ceil(imgX2 - imgX1);
			dst.H = (size_t)ceil(imgY2 - imgY1);
		}
	}
	}

	int strokeAlign = 0;
	
	//Default color
	Magick::Drawable strokeColor = Magick::DrawableStrokeColor(Magick::Color("black"));

	for (int i = 0; i < setting.size(); i++)
	{
		std::wstring tempName, parameter;
		GetNamePara(setting[i], tempName, parameter);
		ParseInternalVariable(measure, parameter, dst);

		LPCWSTR name = tempName.c_str();

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
			strokeWidth = abs(MathParser::ParseF(parameter));
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEALIGN") == 0)
		{
			if (_wcsicmp(parameter.c_str(), L"CENTER") == 0)
				strokeAlign = 0;

			else if (_wcsicmp(parameter.c_str(), L"OUTSIDE") == 0)
				strokeAlign = 1;

			else if (_wcsicmp(parameter.c_str(), L"INSIDE") == 0)
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
			if (_wcsicmp(parameter.c_str(), L"MITER") == 0)
				lj = MagickCore::MiterJoin;
			else if (_wcsicmp(parameter.c_str(), L"ROUND") == 0)
				lj = MagickCore::RoundJoin;
			else if (_wcsicmp(parameter.c_str(), L"BEVEL") == 0)
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
			if (_wcsicmp(parameter.c_str(), L"FLAT") == 0)
				lc = MagickCore::ButtCap;
			else if (_wcsicmp(parameter.c_str(), L"ROUND") == 0)
				lc = MagickCore::RoundCap;
			else if (_wcsicmp(parameter.c_str(), L"SQUARE") == 0)
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
			double strokeWidth2 = strokeWidth * 2;
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth2));
			dst.contain.draw(drawList);

			drawList.push_back(Magick::DrawableStrokeWidth(0));
			temp.draw(drawList);
			if (strokeAlign == 1)
			{
				dst.X -= (ssize_t)ceil(strokeWidth);
				dst.Y -= (ssize_t)ceil(strokeWidth);
				dst.W += (size_t)ceil(strokeWidth2);
				dst.H += (size_t)ceil(strokeWidth2);
				dst.contain.composite(temp, 0, 0, MagickCore::OverCompositeOp);
			}
			else if (strokeAlign == 2)
			{
				dst.contain.composite(temp, 0, 0, MagickCore::CopyAlphaCompositeOp);
			}
		}
		else
		{
			dst.X -= (ssize_t)ceil(strokeWidth / 2);
			dst.Y -= (ssize_t)ceil(strokeWidth / 2);
			dst.W += (size_t)ceil(strokeWidth);
			dst.H += (size_t)ceil(strokeWidth);
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

void DrawRoundedCorner(Magick::VPathList &drawList, Magick::Coordinate &angularPoint,
	Magick::Coordinate &p1, Magick::Coordinate &p2, double radius, bool start)
{
	//Vector 1
	double dx1 = angularPoint.x() - p1.x();
	double dy1 = angularPoint.y() - p1.y();

	//Vector 2
	double dx2 = angularPoint.x() - p2.x();
	double dy2 = angularPoint.y() - p2.y();

	//Angle between vector 1 and vector 2 divided by 2
	double angle = (atan2(dy1, dx1) - atan2(dy2, dx2)) / 2;

	// The length of segment between angular point and the
	// points of intersection with the circle of a given radius
	double tanR = abs(tan(angle));
	double segment = radius / tanR;

	//Check the segment
	double length1 = GetLength(dx1, dy1);
	double length2 = GetLength(dx2, dy2);

	double length = min(length1, length2) / 2;

	if (segment > length)
	{
		segment = length;
		radius = length * tanR;
	}

	auto p1Cross = GetProportionPoint(angularPoint, segment, length1, dx1, dy1);
	auto p2Cross = GetProportionPoint(angularPoint, segment, length2, dx2, dy2);

	if (start) drawList.push_back(Magick::PathMovetoAbs(p1Cross));
	drawList.push_back(Magick::PathLinetoAbs(p1Cross));

	double diameter = 2 * radius;
	double degreeFactor = 180 / MagickPI;

	drawList.push_back(Magick::PathArcAbs(Magick::PathArcArgs(
		radius, radius, 0, false, false, p2Cross.x(), p2Cross.y())));
}

double GetLength(double dx, double dy)
{
	return sqrt(dx * dx + dy * dy);
}

Magick::Coordinate GetProportionPoint(Magick::Coordinate point, double segment,
	double length, double dx, double dy)
{
	double factor = segment / length;

	return Magick::Coordinate(
		point.x() - dx * factor,
		point.y() - dy * factor
	);
}