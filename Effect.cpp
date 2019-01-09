#include "MagickMeter.h"

typedef std::function<void(std::shared_ptr<ImgContainer> container, Config &option)> EffectFunction;

void Move(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector offsetXY = option.ToList(2);
    const ssize_t x = MathParser::ParseSSizeT(offsetXY[0]);
    const ssize_t y = MathParser::ParseSSizeT(offsetXY[1]);

    auto newSize = Magick::Geometry(
        container->img.columns() + x,
        container->img.rows() + y);

    auto tempImg = Magick::Image(newSize, INVISIBLE);

    tempImg.composite(container->img, x, y, Magick::OverCompositeOp);
    container->img = tempImg;

    container->geometry.xOff(container->geometry.xOff() + x);
    container->geometry.yOff(container->geometry.yOff() + y);
}

void AdaptiveBlur(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    container->img.adaptiveBlur(
        MathParser::ParseDouble(valList[0]),
        MathParser::ParseDouble(valList[1]));
}

void Blur(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    container->img.blur(
        MathParser::ParseDouble(valList[0]),
        MathParser::ParseDouble(valList[1]));
}

void GaussianBlur(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    container->img.gaussianBlur(
        MathParser::ParseDouble(valList[0]),
        MathParser::ParseDouble(valList[1]));
}

void MotionBlur(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(3);
    container->img.motionBlur(
        MathParser::ParseDouble(valList[0]),
        MathParser::ParseDouble(valList[1]),
        MathParser::ParseDouble(valList[2]));
}

void Noise(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2, L"");
    double density = 1.0;
    if (valList.size() > 1 && !valList[1].empty())
        density = MathParser::ParseDouble(valList[1]);

    container->img.addNoise(
        (Magick::NoiseType)MathParser::ParseInt(valList[0]),
        density);
}

void Shadow(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector dropShadow = Utils::SeparateList(option.para, L";", 2, L"");
    WSVector valList = Utils::SeparateParameter(dropShadow[0], 5);

    double sigma = MathParser::ParseDouble(valList[1]);
    const ssize_t offsetX = MathParser::ParseInt(valList[2]);
    const ssize_t offsetY = MathParser::ParseInt(valList[3]);

    Magick::Image tempShadow = container->img;

    if (!dropShadow[1].empty())
        tempShadow.backgroundColor(Utils::ParseColor(dropShadow[1]));
    else
        tempShadow.backgroundColor(Magick::Color("black"));

    if (sigma < 0)
        sigma = -sigma;
    tempShadow.shadow(
        MathParser::ParseDouble(valList[0]),
        sigma,
        0, 0);

    container->img.extent(
        Magick::Geometry(
            tempShadow.columns() + offsetX + (size_t)sigma,
            tempShadow.rows() + offsetY + (size_t)sigma
        ),
        INVISIBLE
    );

    if (MathParser::ParseInt(valList[4]) == 1)
    {
        container->img.erase();
        container->img.composite(
            tempShadow,
            offsetX - (ssize_t)(sigma * 2),
            offsetY - (ssize_t)(sigma * 2),
            MagickCore::OverCompositeOp);
    }
    else
    {
        container->img.composite(
            tempShadow,
            (ssize_t)(offsetX - sigma * 2),
            (ssize_t)(offsetY - sigma * 2),
            MagickCore::DstOverCompositeOp);
    }
}

void InnerShadow(std::shared_ptr<ImgContainer> container, Config &option)
{
    //TODO: Crispy outline. fix it.
    WSVector dropShadow = Utils::SeparateList(option.para, L";", 2, L"");
    WSVector valList = Utils::SeparateParameter(dropShadow[0], 5);
    const double sigma = MathParser::ParseDouble(valList[1]);
    const ssize_t offsetX = MathParser::ParseInt(valList[2]);
    const ssize_t offsetY = MathParser::ParseInt(valList[3]);
    Magick::Image temp(Magick::Geometry(
            container->img.columns() + offsetX + (size_t)(sigma * 4),
            container->img.rows() + offsetY + (size_t)(sigma * 4)),
        Magick::Color("black")
    );
    temp.alpha(true);
    temp.composite(
        container->img,
        (size_t)sigma * 2,
        (size_t)sigma * 2,
        Magick::DstOutCompositeOp);
    temp.backgroundColor(Utils::ParseColor(dropShadow[1]));
    temp.shadow(
        MathParser::ParseDouble(valList[0]),
        sigma,
        0, 0);

    Magick::Image shadowOnly = container->img;
    shadowOnly.alpha(true);

    shadowOnly.composite(
        temp,
        offsetX - (ssize_t)(sigma * 4),
        offsetY - (ssize_t)(sigma * 4),
        Magick::SrcInCompositeOp);

    if (MathParser::ParseInt(valList[4]) == 1)
        container->img = shadowOnly;
    else
        container->img.composite(shadowOnly, 0, 0, Magick::OverCompositeOp);
}

void Distort(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = Utils::SeparateList(option.para, L";", 3);
    WSVector rawList = Utils::SeparateParameter(valList.at(1), NULL);

    std::vector<double> doubleList;
    for (auto &oneDouble : rawList)
        doubleList.push_back(MathParser::ParseDouble(oneDouble));

    const double *doubleArray = &*doubleList.begin();
    container->img.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
    container->img.distort(
        (Magick::DistortMethod)MathParser::ParseInt(valList[0]),
        doubleList.size(),
        doubleArray,
        MathParser::ParseBool(valList[2]));
}

//Same as Distort Perspective but this only requires position of 4 final points
void Perspective(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector rawList = option.ToList(NULL);

    if (rawList.size() < 8)
    {
        throw(L"Perspective: Not enough control point. Requires 4 pairs of X and Y.");
    }

    double doubleArray[16];
    //Top Left
    doubleArray[0] = (double)container->geometry.xOff();
    doubleArray[1] = (double)container->geometry.yOff();
    doubleArray[2] = Utils::ParseNumber2(rawList[0].c_str(), doubleArray[0], MathParser::ParseDouble);
    doubleArray[3] = Utils::ParseNumber2(rawList[1].c_str(), doubleArray[1], MathParser::ParseDouble);
    //Top Right
    doubleArray[4] = (double)(container->geometry.xOff() + container->geometry.width());
    doubleArray[5] = (double)container->geometry.yOff();
    doubleArray[6] = Utils::ParseNumber2(rawList[2].c_str(), doubleArray[4], MathParser::ParseDouble);
    doubleArray[7] = Utils::ParseNumber2(rawList[3].c_str(), doubleArray[5], MathParser::ParseDouble);
    //Bottom Right
    doubleArray[8] = (double)(container->geometry.xOff() + container->geometry.width());
    doubleArray[9] = (double)(container->geometry.yOff() + container->geometry.height());
    doubleArray[10] = Utils::ParseNumber2(rawList[4].c_str(), doubleArray[8], MathParser::ParseDouble);
    doubleArray[11] = Utils::ParseNumber2(rawList[5].c_str(), doubleArray[9], MathParser::ParseDouble);
    //Bottom Left
    doubleArray[12] = (double)container->geometry.xOff();
    doubleArray[13] = (double)(container->geometry.yOff() + container->geometry.height());
    doubleArray[14] = Utils::ParseNumber2(rawList[6].c_str(), doubleArray[12], MathParser::ParseDouble);
    doubleArray[15] = Utils::ParseNumber2(rawList[7].c_str(), doubleArray[13], MathParser::ParseDouble);

    container->img.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
    container->img.distort(
        MagickCore::PerspectiveDistortion,
        16,
        doubleArray,
        true);
}

void Opacity(std::shared_ptr<ImgContainer> container, Config &option)
{
    double a = option.ToDouble();
    if (a > 100)
        a = 100;
    if (a < 0)
        a = 0;
    a /= 100;

    auto temp = Magick::Image(
        Magick::Geometry(container->img.columns(), container->img.rows()),
        Magick::ColorRGB(1, 1, 1, a));

    container->img.composite(temp, 0, 0, Magick::DstInCompositeOp);
}

void Shade(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(3);

    container->img.shade(
        MathParser::ParseInt(valList[0]),
        MathParser::ParseInt(valList[1]),
        MathParser::ParseInt(valList[2]) == 1);
}

void Channel(std::shared_ptr<ImgContainer> container, Config &option)
{
    if (option.Equal(L"RED"))
    {
        container->img = container->img.separate(Magick::RedChannel);
    }
    else if (option.Equal(L"GREEN"))
    {
        container->img = container->img.separate(Magick::GreenChannel);
    }
    else if (option.Equal(L"BLUE"))
    {
        container->img = container->img.separate(Magick::BlueChannel);
    }
    else if (option.Equal(L"BLACK"))
    {
        container->img = container->img.separate(Magick::BlackChannel);
    }
    else if (option.Equal(L"OPACITY"))
    {
        container->img = container->img.separate(Magick::OpacityChannel);
    }
}

void Modulate(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(3, L"100");
    container->img.modulate(
        MathParser::ParseInt(valList[0]),
        MathParser::ParseInt(valList[1]),
        MathParser::ParseInt(valList[2]));
}

void Oilpaint(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    container->img.oilPaint(
        MathParser::ParseInt(valList[0]),
        MathParser::ParseInt(valList[1]));
}

void Shear(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    container->img.shear(
        MathParser::ParseInt(valList[0]),
        MathParser::ParseInt(valList[1]));
}

void Resize(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(3);
    Magick::Geometry newSize(
        MathParser::ParseInt(valList.at(0)),
        MathParser::ParseInt(valList.at(1)));
    if (valList.size() > 2)
        Utils::SetGeometryMode(MathParser::ParseInt(valList.at(2)), newSize);

    container->img.resize(newSize);
}

void AdaptiveResize(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(3);
    Magick::Geometry newSize(
        MathParser::ParseInt(valList.at(0)),
        MathParser::ParseInt(valList.at(1)));
    if (valList.size() > 2)
        Utils::SetGeometryMode(MathParser::ParseInt(valList.at(2)), newSize);

    container->img.adaptiveResize(newSize);
}

void Scale(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    Magick::Geometry newSize;
    const size_t percentPos = valList[0].find(L"%");
    if (percentPos != std::wstring::npos) // One parameter with percentage value
    {
        newSize.percent(true);
        const size_t percent = MathParser::ParseInt(valList[0].substr(0, percentPos));
        newSize.width(percent);
        newSize.height(percent);
    }
    else
    {
        newSize.width(MathParser::ParseInt(valList[0]));
        newSize.height(MathParser::ParseInt(valList[1]));

        if (valList.size() > 2)
            Utils::SetGeometryMode(MathParser::ParseInt(valList[2]), newSize);
    }

    container->img.scale(newSize);
}

void Sample(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    Magick::Geometry newSize(
        MathParser::ParseInt(valList.at(0)),
        MathParser::ParseInt(valList.at(1)));
    if (valList.size() > 2)
        Utils::SetGeometryMode(MathParser::ParseInt(valList.at(2)), newSize);

    container->img.sample(newSize);
}

void Resample(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.resample(option.ToDouble());
}

void Crop(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector geometryRaw = option.ToList(5);
    ssize_t x = MathParser::ParseSSizeT(geometryRaw[0]);
    ssize_t y = MathParser::ParseSSizeT(geometryRaw[1]);
    size_t w = MathParser::ParseSizeT(geometryRaw[2]);
    size_t h = MathParser::ParseSizeT(geometryRaw[3]);
    const int origin = MathParser::ParseInt(geometryRaw[4]);

    if (w < 0 || h < 0)
    {
        throw(L"Crop: Invalid width or height value.");
    }
    else if (origin > 5 || origin < 0)
    {
        throw(L"Crop: Invalid Origin value. Left Top origin is used.");
    }

    //TOP LEFT anchor is default anchor.
    switch (origin)
    {
    case 2: //TOP RIGHT
        x += container->img.columns();
        break;
    case 3: //BOTTOM RIGHT
        x += container->img.columns();
        y += container->img.rows();
        break;
    case 4: //BOTTOM LEFT
        y += container->img.rows();
        break;
    case 5: //CENTER
        x += container->img.columns() / 2;
        y += container->img.rows() / 2;
        break;
    }

    if (x < 0)
    {
        w += x;
        x = 0;
    }

    if (y < 0)
    {
        h += y;
        y = 0;
    }

    //Chop off size when selected region is out of origin image
    if ((w + x) > container->img.columns())
        w = container->img.columns() - x;

    if ((h + y) > container->img.rows())
        h = container->img.rows() - y;

    container->img.crop(Magick::Geometry(w, h, x, y));
    // Move cropped image to top left of canvas
    container->img.page(Magick::Geometry(0, 0, 0, 0));
}

void RotationalBlur(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.rotationalBlur(option.ToDouble());
}

void Colorize(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = Utils::SeparateList(option.para, L";", 2);
    container->img.colorize(
        (unsigned int)MathParser::ParseInt(valList[0]),
        Utils::ParseColor(valList[1]));
}

void Charcoal(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(0);
    const auto vSize = valList.size();
    if (vSize == 0)
    {
        container->img.charcoal();
    }
    else if (vSize == 1)
    {
        container->img.charcoal(MathParser::ParseDouble(valList.at(0)));
    }
    else
    {
        container->img.charcoal(
            MathParser::ParseDouble(valList.at(0)),
            MathParser::ParseDouble(valList.at(1))
        );
    }
}

void Grayscale(std::shared_ptr<ImgContainer> container, Config &option)
{
    const int method = MathParser::ParseInt(option.para);
    if (method > 0 && method <= 9)
        container->img.grayscale((Magick::PixelIntensityMethod)method);
}

void Roll(std::shared_ptr<ImgContainer> container, Config &option)
{
    WSVector valList = option.ToList(2);
    size_t c = MathParser::ParseSizeT(valList[0]);
    size_t r = MathParser::ParseSizeT(valList[1]);
    if (c < 0)
        c += container->img.columns();
    if (r < 0)
        r += container->img.rows();

    container->img.roll(c, r);
}

void Negate(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.negate(option.ToBool());
}

void Implode(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.implode(option.ToDouble());
}

void Spread(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
    container->img.spread(option.ToDouble());
}

void Swirl(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.swirl(option.ToDouble());
}

void MedianFilter(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.medianFilter(option.ToDouble());
}

void Equalize(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.equalize();
}

void Enhance(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.enhance();
}

void Despeckle(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.despeckle();
}

void ReduceNoise(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.reduceNoise();
}

void Transpose(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.transpose();
}

void Transverse(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.transverse();
}

void Flip(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.flip();
}

void Flop(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.flop();
}

void Magnify(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.magnify();
}

void Minify(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.minify();
}

void Ignore(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->isIgnored = option.ToBool();
}

void SetFilter(std::shared_ptr<ImgContainer> container, Config &option)
{
    Magick::FilterType filterType = Magick::UndefinedFilter;

    if (option.Equal(L"POINT"))
    {
        filterType = Magick::PointFilter;
    }
    else if (option.Equal(L"BOX"))
    {
        filterType = Magick::BoxFilter;
    }
    else if (option.Equal(L"TRIANGLE"))
    {
        filterType = Magick::TriangleFilter;
    }
    else if (option.Equal(L"HERMITE"))
    {
        filterType = Magick::HermiteFilter;
    }
    else if (option.Equal(L"HANN"))
    {
        filterType = Magick::HannFilter;
    }
    else if (option.Equal(L"HAMMING"))
    {
        filterType = Magick::HammingFilter;
    }
    else if (option.Equal(L"BLACKMAN"))
    {
        filterType = Magick::BlackmanFilter;
    }
    else if (option.Equal(L"GAUSSIAN"))
    {
        filterType = Magick::GaussianFilter;
    }
    else if (option.Equal(L"QUADRATIC"))
    {
        filterType = Magick::QuadraticFilter;
    }
    else if (option.Equal(L"CUBIC"))
    {
        filterType = Magick::CubicFilter;
    }
    else if (option.Equal(L"CATROM"))
    {
        filterType = Magick::CatromFilter;
    }
    else if (option.Equal(L"MITCHELL"))
    {
        filterType = Magick::MitchellFilter;
    }
    else if (option.Equal(L"JINC"))
    {
        filterType = Magick::JincFilter;
    }
    else if (option.Equal(L"SINC"))
    {
        filterType = Magick::SincFilter;
    }
    else if (option.Equal(L"SINCFAST"))
    {
        filterType = Magick::SincFastFilter;
    }
    else if (option.Equal(L"KAISER"))
    {
        filterType = Magick::KaiserFilter;
    }
    else if (option.Equal(L"WELCH"))
    {
        filterType = Magick::WelchFilter;
    }
    else if (option.Equal(L"PARZEN"))
    {
        filterType = Magick::ParzenFilter;
    }
    else if (option.Equal(L"BOHMAN"))
    {
        filterType = Magick::BohmanFilter;
    }
    else if (option.Equal(L"BARTLETT"))
    {
        filterType = Magick::BartlettFilter;
    }
    else if (option.Equal(L"LAGRANGE"))
    {
        filterType = Magick::LagrangeFilter;
    }
    else if (option.Equal(L"LANCZOS"))
    {
        filterType = Magick::LanczosFilter;
    }
    else if (option.Equal(L"LANCZOSSHARP"))
    {
        filterType = Magick::LanczosSharpFilter;
    }
    else if (option.Equal(L"LANCZOS2"))
    {
        filterType = Magick::Lanczos2Filter;
    }
    else if (option.Equal(L"LANCZOS2SHARP"))
    {
        filterType = Magick::Lanczos2SharpFilter;
    }
    else if (option.Equal(L"ROBIDOUX"))
    {
        filterType = Magick::RobidouxFilter;
    }
    else if (option.Equal(L"ROBIDOUXSHARP"))
    {
        filterType = Magick::RobidouxSharpFilter;
    }
    else if (option.Equal(L"COSINE"))
    {
        filterType = Magick::CosineFilter;
    }
    else if (option.Equal(L"SPLINE"))
    {
        filterType = Magick::SplineFilter;
    }
    else if (option.Equal(L"LANCZOSRADIUS"))
    {
        filterType = Magick::LanczosRadiusFilter;
    }
    else if (option.Equal(L"CUBICSPLINE"))
    {
        filterType = MagickCore::CubicSplineFilter;
    }

    container->img.filterType(filterType);
}

void Monochrome(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.monochrome(true);
};

void BlueShift(std::shared_ptr<ImgContainer> container, Config &option)
{
    const double radian = static_cast<double>(option.ToInt()) / 180.0 * MagickPI;
    container->img.blueShift(radian);
};

void Rotate(std::shared_ptr<ImgContainer> container, Config &option)
{
    container->img.backgroundColor(INVISIBLE);
    container->img.rotate(option.ToDouble());
}

void Vignette(std::shared_ptr<ImgContainer> container, Config &option)
{
    auto valList = Utils::SeparateList(option.para, L";", 2, L"");
    if (!valList[1].empty())
        container->img.backgroundColor(Utils::ParseColor(valList[1]));
    else
        container->img.backgroundColor(Magick::Color("black"));

    auto paraList = Utils::SeparateParameter(valList[0], 4);
    container->img.vignette(
        MathParser::ParseDouble(valList.at(0)),
        MathParser::ParseDouble(valList.at(1)),
        MathParser::ParseSSizeT(valList.at(2)),
        MathParser::ParseSSizeT(valList.at(3))
    );

    container->img.backgroundColor(INVISIBLE);
}

const std::map<LPCWSTR, EffectFunction> effectMap{
    {L"ADAPTIVEBLUR",       AdaptiveBlur},
    {L"ADAPTIVERESIZE",     AdaptiveResize},
    {L"BLUESHIFT",          BlueShift},
    {L"BLUR",               Blur},
    {L"CHANNEL",            Channel},
    {L"CHARCOAL",           Charcoal},
    {L"COLORIZE",           Colorize},
    {L"CROP",               Crop},
    {L"DESPECKLE",          Despeckle},
    {L"DISTORT",            Distort},
    {L"ENHANCE",            Enhance},
    {L"EQUALIZE",           Equalize},
    {L"FLIP",               Flip},
    {L"FLOP",               Flop},
    {L"GAUSSIANBLUR",       GaussianBlur},
    {L"GRAYSCALE",          Grayscale},
    {L"IGNORE",             Ignore},
    {L"IMPLODE",            Implode},
    {L"INNERSHADOW",        InnerShadow},
    {L"MAGNIFY",            Magnify},
    {L"MEDIANFILTER",       MedianFilter},
    {L"MINIFY",             Minify},
    {L"MODULATE",           Modulate},
    {L"MONOCHROME",         Monochrome},
    {L"MOTIONBLUR",         MotionBlur},
    {L"MOVE",               Move},
    {L"NEGATE",             Negate},
    {L"NOISE",              Noise},
    {L"OILPAINT",           Oilpaint},
    {L"OPACITY",            Opacity},
    {L"PERSPECTIVE",        Perspective},
    {L"REDUCENOISE",        ReduceNoise},
    {L"RESAMPLE",           Resample},
    {L"RESIZE",             Resize},
    {L"ROLL",               Roll},
    {L"IMAGEROTATE",        Rotate},
    {L"ROTATIONALBLUR",     RotationalBlur},
    {L"SAMPLE",             Sample},
    {L"SCALE",              Scale},
    {L"SETFILTER",          SetFilter},
    {L"SHADE",              Shade},
    {L"SHADOW",             Shadow},
    {L"SHEAR",              Shear},
    {L"SPREAD",             Spread},
    {L"SWIRL",              Swirl},
    {L"TRANSPOSE",          Transpose},
    {L"TRANSVERSE",         Transverse},
    {L"VIGNETTE",           Vignette},
};

BOOL Measure::ApplyEffect(std::shared_ptr<ImgContainer> container, Config &option)
{
    auto effect = std::find_if(
        effectMap.begin(),
        effectMap.end(),
        [option](const std::pair<LPCWSTR, EffectFunction> &i) {
            return Utils::IsEqual(option.name, i.first);
        }
    );

    if (effect == effectMap.end())
    {
        return TRUE;
    }

    option.isApplied = TRUE;

    try
    {
        effect->second(container, option);
    }
    catch (LPCWSTR warning)
    {
        RmLog(rm, LOG_WARNING, warning);
    }
    catch (Magick::Exception &error_)
    {
        LogError(error_);
        return FALSE;
    }

    return TRUE;
}