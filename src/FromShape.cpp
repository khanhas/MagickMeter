#include "MagickMeter.h"

double GetLength(double dx, double dy) noexcept;
Magick::Coordinate GetProportionPoint(Magick::Coordinate point, double segment,
	double length, double dx, double dy);
void DrawRoundedCorner(Magick::VPathList &drawList, const Magick::Coordinate &angularPoint,
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
	auto Create(void) { return Magick::DrawableRectangle(x, y, x + w, y + h); }
	auto CreateRound(void) { return Magick::DrawableRoundRectangle(x, y, x + w, y + h, cornerX, cornerY); }
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
	/*virtual*/ void operator() (MagickCore::DrawingWand *context_) const override
	{
		for (Magick::PathQuadraticCurvetoArgsList::const_iterator p = _args.begin();
			p != _args.end(); p++)
		{
			DrawPathCurveToSmoothAbsolute(context_, p->x1(), p->y1(), p->x(), p->y());
		}
	}

	// Return polymorphic copy of object
	/*virtual*/
	VPathBase* copy() const override
	{
        return new PathCubicSmoothCurvetoAbs(*this);
	}

private:
	Magick::PathQuadraticCurvetoArgsList _args;
};

BOOL Measure::CreateShape(ImgType shapeType, LPCWSTR shapeParas, WSVector &config, ImgContainer &out)
{
	std::vector<Magick::Drawable> drawList;
	Magick::Drawable mainShape;
	WSVector paraList;

	double width = 0;
	double height = 0;

	double strokeWidth = 0;

	Magick::Geometry customCanvas;
	switch (shapeType)
	{
	case ELLIPSE:
	{
		paraList = Utils::SeparateParameter(shapeParas, NULL);
		const size_t pSize = paraList.size();
		if (pSize < 3)
		{
			RmLog(2, L"Ellipse: Not enough parameter.");
			return FALSE;
		}
		ShapeEllipse e;
		e.x = MathParser::ParseDouble(paraList[0]);
		e.y = MathParser::ParseDouble(paraList[1]);
		e.radiusX = MathParser::ParseDouble(paraList[2]);

		e.radiusY = e.radiusX;
		if (pSize > 3) e.radiusY = MathParser::ParseDouble(paraList[3]);

		if (pSize > 4) e.start = MathParser::ParseDouble(paraList[4]);

		if (pSize > 5) e.end = MathParser::ParseDouble(paraList[5]);

		mainShape = e.Create();
		drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));

		width = e.x + abs(e.radiusX);
		height = e.y + abs(e.radiusY);

		out.X = (ssize_t)(e.x - abs(e.radiusX));
		out.Y = (ssize_t)(e.y - abs(e.radiusY));
		out.W = (size_t)(e.radiusX * 2.0);
		out.H = (size_t)(e.radiusY * 2.0);

		break;
	}
	case RECTANGLE:
	{
		paraList = Utils::SeparateParameter(shapeParas, NULL);
		const size_t pSize = paraList.size();
		if (pSize < 4)
		{
			RmLog(rm, LOG_ERROR, L"Rectangle: Not enough parameter.");
			return FALSE;
		}

		ShapeRectangle r;
		r.x = MathParser::ParseDouble(paraList[0]);
		r.y = MathParser::ParseDouble(paraList[1]);
		r.w = MathParser::ParseDouble(paraList[2]);
		r.h = MathParser::ParseDouble(paraList[3]);

		if (pSize > 4) r.cornerX = MathParser::ParseDouble(paraList[4]);

		r.cornerY = r.cornerX;
		if (pSize > 5) r.cornerY = MathParser::ParseDouble(paraList[5]);

		if (r.cornerX == 0 && r.cornerY == 0)
			mainShape = r.Create();
		else
			mainShape = r.CreateRound();

		drawList.push_back(Magick::DrawableFillColor(Magick::Color("white")));

		if (r.w > 0)
		{
			width = r.w + r.x;
			out.X = (ssize_t)r.x;
		}
		else
		{
			width = r.x;
			out.X = (ssize_t)(r.x + r.w);
		}
		out.W = (size_t)abs(r.w);

		if (r.h > 0)
		{
			height = r.h + r.y;
			out.Y = (ssize_t)r.y;
		}
		else
		{
			height = r.y;
			out.Y = (ssize_t)(r.y + r.h);
		}
		out.H = (size_t)abs(r.h);
		break;
	}
	case POLYGON:
	{
		paraList = Utils::SeparateParameter(shapeParas, NULL);
		const size_t listSize = paraList.size();
		if (listSize < 4)
		{
			RmLogF(rm, LOG_ERROR, L"Polygon %s: Not enough parameter", shapeParas);
			return FALSE;
		}
		const double origX = MathParser::ParseDouble(paraList[0]);
		const double origY = MathParser::ParseDouble(paraList[1]);
		const double side = round(MathParser::ParseDouble(paraList[2]));
		if (side < 3)
		{
			RmLogF(rm, LOG_ERROR, L"Polygon %s: Invalid number of sides.", shapeParas);
			return FALSE;
		}
		const double radiusX = abs(MathParser::ParseDouble(paraList[3]));
		double radiusY = radiusX;
		if (listSize > 4) radiusY = abs(MathParser::ParseDouble(paraList[4]));
		double roundCorner = 0;
		if (listSize > 5) roundCorner = abs(MathParser::ParseDouble(paraList[5]));
		double startAngle = 0;
		if (listSize > 6) startAngle = MathParser::ParseDouble(paraList[6]) * MagickPI / 180;

		Magick::VPathList p;
		if (roundCorner != 0)
		{
			Magick::Coordinate c1(
				origX + radiusX * cos(MagickPI2 + startAngle),
				origY - radiusY * sin(MagickPI2 + startAngle)
			);
			const double factor = 1 / side;
			for (int i = 1; i <= side; i++)
			{
				double a = (double)i * factor;
				Magick::Coordinate angPoint(
					origX + radiusX * cos(a * Magick2PI + MagickPI2 + startAngle),
					origY - radiusY * sin(a * Magick2PI + MagickPI2 + startAngle)
				);
				a = ((double)i + 1.0) * factor;
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
				const double a = (double)i / side;
				const double curX = origX + radiusX * cos(a * Magick2PI + MagickPI2 + startAngle);
				const double curY = origY - radiusY * sin(a * Magick2PI + MagickPI2 + startAngle);

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
		out.X = (ssize_t)round(origX - radiusX);
		out.Y = (ssize_t)round(origY - radiusY);
		out.W = (size_t)ceil(radiusX * 2);
		out.H = (size_t)ceil(radiusY * 2);
		break;
	}
	case PATH:
	{
		LPCWSTR pathString = RmReadString(rm, shapeParas, L"");
		if (wcslen(pathString) == 0)
		{
            RmLogF(rm, LOG_ERROR, L"Path %s: Not found", shapeParas);
            break;
		}

        WSVector pathList = Utils::SeparateList(pathString, L"|", NULL);

        Magick::VPathList p;
        double curX = 0.0;
        double curY = 0.0;

        //Start point
        WSVector s = Utils::SeparateParameter(pathList[0], NULL);
        if (s.size() >= 2)
        {
            curX = MathParser::ParseDouble(s[0]);
            curY = MathParser::ParseDouble(s[1]);
            p.push_back(Magick::PathMovetoAbs(Magick::Coordinate(
                curX,
                curY
            )));
        }
        else
        {
            RmLogF(rm, 2, L"%s is invalid start point.", pathList[0].c_str());
            return FALSE;
        }

        double imgX1 = curX;
        double imgY1 = curY;
        double imgX2 = curX;
        double imgY2 = curY;

        for (auto &path : pathList)
        {
            std::wstring tempName, tempPara;
            Utils::GetNamePara(path, tempName, tempPara);
            ParseInternalVariable(tempPara, out);

            LPCWSTR name = tempName.c_str();
            WSVector segmentList = Utils::SeparateParameter(tempPara, NULL);
            const size_t segmentSize = segmentList.size();

            if (_wcsicmp(name, L"LINETO") == 0)
            {
                if (segmentSize > 1)
                {
                    curX = MathParser::ParseDouble(segmentList[0]);
                    curY = MathParser::ParseDouble(segmentList[1]);
                    p.push_back(Magick::PathLinetoAbs(Magick::Coordinate(
                        curX,
                        curY
                    )));
                }
            }
            else if (_wcsicmp(name, L"ARCTO") == 0)
            {
                if (segmentSize >= 2)
                {
                    const double x = MathParser::ParseDouble(segmentList[0]);
                    const double y = MathParser::ParseDouble(segmentList[1]);
                    const double dx = x - curX;
                    const double dy = y - curY;
                    double xRadius = std::sqrt(dx * dx + dy * dy) / 2.0;
                    double angle = 0.0;
                    bool sweep = false;
                    bool size = false;

                    if (segmentSize > 2) xRadius = MathParser::ParseDouble(segmentList[2]);

                    double yRadius = xRadius;

                    if (segmentSize > 3) yRadius = Utils::ParseNumber(yRadius, segmentList[3].c_str(), MathParser::ParseDouble);
                    if (segmentSize > 4) angle = Utils::ParseNumber(angle, segmentList[4].c_str(), MathParser::ParseDouble);
                    if (segmentSize > 5) Utils::ParseBool(sweep, segmentList[5]);
                    if (segmentSize > 6) Utils::ParseBool(size, segmentList[6]);

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
                if (segmentSize < 4)
                {
                    RmLogF(rm, LOG_WARNING, L"%s %s is invalid CurveTo segment.", name, tempPara);
                    continue;
                }
                const double x = MathParser::ParseDouble(segmentList[0]);
                const double y = MathParser::ParseDouble(segmentList[1]);
                const double cx1 = MathParser::ParseDouble(segmentList[2]);
                const double cy1 = MathParser::ParseDouble(segmentList[3]);
                if (segmentSize < 6)
                {
                    p.push_back(Magick::PathQuadraticCurvetoAbs(Magick::PathQuadraticCurvetoArgs(
                        cx1, cy1, x, y
                    )));
                }
                else
                {
                    const double cx2 = MathParser::ParseDouble(segmentList[4]);
                    const double cy2 = MathParser::ParseDouble(segmentList[5]);
                    p.push_back(Magick::PathCurvetoAbs(Magick::PathCurvetoArgs(
                        cx1, cy1, cx2, cy2, x, y
                    )));
                }
                curX = x;
                curY = y;
            }
            else if (_wcsicmp(name, L"SMOOTHCURVETO") == 0)
            {
                if (segmentSize < 2)
                {
                    RmLogF(rm, LOG_WARNING, L"%s %s is invalid path segment.", name, tempPara);
                    continue;
                }
                const double x = MathParser::ParseDouble(segmentList[0]);
                const double y = MathParser::ParseDouble(segmentList[1]);

                if (segmentSize > 3)
                {
                    const double cx = MathParser::ParseDouble(segmentList[2]);
                    const double cy = MathParser::ParseDouble(segmentList[3]);

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
                if (MathParser::ParseBool(tempPara))
                    p.push_back(Magick::PathClosePath());
            }
            else
            {
                continue;
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
        out.X = (ssize_t)ceil(imgX1);
        out.Y = (ssize_t)ceil(imgY1);
        out.W = (size_t)ceil(imgX2 - imgX1);
        out.H = (size_t)ceil(imgY2 - imgY1);
        break;
	}
	}

	int strokeAlign = 0;

	//Default color
	Magick::Drawable strokeColor = Magick::DrawableStrokeColor(Magick::Color("black"));

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
			WSVector valList = Utils::SeparateList(tempPara, L",", 2);
            const size_t tempW = MathParser::ParseSizeT(valList[0]);
            const size_t tempH = MathParser::ParseSizeT(valList[1]);

            if (tempW <= 0 || tempH <= 0)
            {
                RmLog(2, L"Invalid Width or Height value. Default canvas is used.");
            }
			else
			{
				customCanvas = Magick::Geometry(tempW, tempH);
				customCanvas.aspect(true);
			}
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"OFFSET") == 0)
		{
			WSVector valList = Utils::SeparateParameter(tempPara, 2);
			drawList.push_back(Magick::DrawableTranslation(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"ROTATE") == 0)
		{
			drawList.push_back(Magick::DrawableRotation(
				MathParser::ParseDouble(tempPara)
			));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"COLOR") == 0)
		{
			drawList.push_back(Magick::DrawableFillColor(
                Utils::ParseColor(tempPara)
            ));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"FILLPATTERN") == 0)
		{
			const int index = Utils::NameToIndex(tempPara);
			if (index >= 0 && index < imgList.size())
				out.img.fillPattern(imgList[index].img);
		}
		else if (_wcsicmp(name, L"ANTIALIAS") == 0)
		{
			drawList.push_back(Magick::DrawableStrokeAntialias(
                MathParser::ParseBool(tempPara)
            ));
			isSetting = TRUE;
		}
		if (_wcsicmp(name, L"STROKECOLOR") == 0)
		{
			strokeColor = Magick::DrawableStrokeColor(
                Utils::ParseColor(tempPara)
            );
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEWIDTH") == 0)
		{
			strokeWidth = abs(MathParser::ParseDouble(tempPara));
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth));
			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKEALIGN") == 0)
		{
			if (_wcsicmp(tempPara.c_str(), L"CENTER") == 0)
				strokeAlign = 0;

			else if (_wcsicmp(tempPara.c_str(), L"OUTSIDE") == 0)
				strokeAlign = 1;

			else if (_wcsicmp(tempPara.c_str(), L"INSIDE") == 0)
				strokeAlign = 2;
			else
				RmLog(2, L"Invalid StrokeAlign value. Center align is applied.");

			isSetting = TRUE;
		}
		else if (_wcsicmp(name, L"STROKELINEJOIN") == 0)
		{
			Magick::LineJoin lj = MagickCore::MiterJoin;
			if (_wcsicmp(tempPara.c_str(), L"MITER") == 0)
				lj = MagickCore::MiterJoin;
			else if (_wcsicmp(tempPara.c_str(), L"ROUND") == 0)
				lj = MagickCore::RoundJoin;
			else if (_wcsicmp(tempPara.c_str(), L"BEVEL") == 0)
				lj = MagickCore::BevelJoin;
			else
			{
				RmLog(2, L"Invalid StrokeLineJoin value. Miter line join is applied.");
			}
			drawList.push_back(Magick::DrawableStrokeLineJoin(lj));
		}
		else if (_wcsicmp(name, L"STROKELINECAP") == 0)
		{
			Magick::LineCap lc = MagickCore::ButtCap;;
			if (_wcsicmp(tempPara.c_str(), L"FLAT") == 0)
				lc = MagickCore::ButtCap;
			else if (_wcsicmp(tempPara.c_str(), L"ROUND") == 0)
				lc = MagickCore::RoundCap;
			else if (_wcsicmp(tempPara.c_str(), L"SQUARE") == 0)
				lc = MagickCore::SquareCap;
			else
			{
				RmLog(2, L"Invalid StrokeLineCap value. Flat line cap is applied.");
			}
			drawList.push_back(Magick::DrawableStrokeLineCap(lc));
		}

		if (isSetting)
			option.clear();
	}

	try
	{
		if (!customCanvas.isValid())
		{
            customCanvas = Magick::Geometry(
                (size_t)(width + strokeWidth),
                (size_t)(height + strokeWidth)
            );
            customCanvas.aspect(true);
		}

        out.img.size(customCanvas);

		drawList.push_back(mainShape);

		if (strokeWidth == 0)
		{
			drawList.push_back(Magick::DrawableStrokeColor(INVISIBLE));
			out.img.draw(drawList);
		}
		else if (strokeWidth != 0 && strokeAlign != 0)
		{
			drawList.push_back(strokeColor);
			Magick::Image temp = out.img;
			const double strokeWidth2 = strokeWidth * 2;
			drawList.push_back(Magick::DrawableStrokeWidth(strokeWidth2));
			out.img.draw(drawList);

			drawList.push_back(Magick::DrawableStrokeWidth(0));
			temp.draw(drawList);
			if (strokeAlign == 1)
			{
				out.X -= (ssize_t)ceil(strokeWidth);
				out.Y -= (ssize_t)ceil(strokeWidth);
				out.W += (size_t)ceil(strokeWidth2);
				out.H += (size_t)ceil(strokeWidth2);
				out.img.composite(temp, 0, 0, Magick::OverCompositeOp);
			}
			else if (strokeAlign == 2)
			{
				out.img.composite(temp, 0, 0, Magick::CopyAlphaCompositeOp);
			}
		}
		else
		{
			out.X -= (ssize_t)ceil(strokeWidth / 2);
			out.Y -= (ssize_t)ceil(strokeWidth / 2);
			out.W += (size_t)ceil(strokeWidth);
			out.H += (size_t)ceil(strokeWidth);
			drawList.push_back(strokeColor);
			out.img.draw(drawList);
		}
	}
	catch (Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}

	return TRUE;
}

void DrawRoundedCorner(Magick::VPathList &drawList, const Magick::Coordinate &angularPoint,
	Magick::Coordinate &p1, Magick::Coordinate &p2, double radius, bool start)
{
	// Vector 1
	const double dx1 = angularPoint.x() - p1.x();
	const double dy1 = angularPoint.y() - p1.y();

	// Vector 2
    const double dx2 = angularPoint.x() - p2.x();
    const double dy2 = angularPoint.y() - p2.y();

	// Angle between vector 1 and vector 2 divided by 2
    const double angle = (atan2(dy1, dx1) - atan2(dy2, dx2)) / 2;

	// The length of segment between angular point and the
	// points of intersection with the circle of a given radius
    const double tanR = abs(tan(angle));
	double segment = radius / tanR;

	// Check the segment
    const double length1 = GetLength(dx1, dy1);
    const double length2 = GetLength(dx2, dy2);

    const double length = min(length1, length2) / 2;

	if (segment > length)
	{
		segment = length;
		radius = length * tanR;
	}

	auto p1Cross = GetProportionPoint(angularPoint, segment, length1, dx1, dy1);
	auto p2Cross = GetProportionPoint(angularPoint, segment, length2, dx2, dy2);

	if (start) drawList.push_back(Magick::PathMovetoAbs(p1Cross));
	drawList.push_back(Magick::PathLinetoAbs(p1Cross));

    const double diameter = 2 * radius;
    const double degreeFactor = 180 / MagickPI;

	drawList.push_back(Magick::PathArcAbs(Magick::PathArcArgs(
		radius, radius, 0, false, false, p2Cross.x(), p2Cross.y())));
}

double GetLength(double dx, double dy) noexcept
{
	return sqrt(dx * dx + dy * dy);
}

Magick::Coordinate GetProportionPoint(Magick::Coordinate point, double segment,
	double length, double dx, double dy)
{
	const double factor = segment / length;

	return Magick::Coordinate(
		point.x() - dx * factor,
		point.y() - dy * factor
	);
}