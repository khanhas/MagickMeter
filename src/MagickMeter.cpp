#include "MagickMeter.h"
#include <sstream>
#include "MagickCore\opencl.h"
#include <ctime>

int Measure::refCount = 0;

ImgType GetType(Config option)
{
	if		(option.Match(L"FILE"))			return NORMAL;
	else if (option.Match(L"TEXT"))			return TEXT;
	else if (option.Match(L"PATH"))			return PATH;
	else if (option.Match(L"CLONE"))		return CLONE;
	else if (option.Match(L"ELLIPSE"))		return ELLIPSE;
	else if (option.Match(L"POLYGON"))		return POLYGON;
	else if (option.Match(L"COMBINE"))		return COMBINE;
	else if (option.Match(L"GRADIENT"))		return GRADIENT;
	else if (option.Match(L"RECTANGLE"))	return RECTANGLE;
	else									return NOTYPE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	auto measure = new Measure;
    Measure::refCount++;
	*data = measure;
	measure->rm = rm;
	measure->skin = RmGetSkin(rm);

	Magick::InitializeMagick("\\");
    if (Magick::EnableOpenCL())
    {
        RmLog(measure->rm, LOG_DEBUG, L"MagickMeter: OpenCL supported");
        size_t count = 0;
        auto devices = MagickCore::GetOpenCLDevices(&count, nullptr);
        for (size_t i = 0; i < count; i++)
        {
            auto name = MagickCore::GetOpenCLDeviceName(devices[i]);
            RmLogF(rm, LOG_DEBUG, L"Device %d: %s", i, Utils::StringToWString(name).c_str());
        }
        const ssize_t chosenDevice{ RmReadInt(rm, L"Device", 0) };
        if (chosenDevice > -1 && chosenDevice < count)
        {
            MagickCore::SetOpenCLDeviceEnabled(devices[chosenDevice], MagickCore::MagickTrue);
            const auto enabled = MagickCore::GetOpenCLDeviceEnabled(devices[0]);
            if (enabled == MagickCore::MagickTrue)
            {
                RmLogF(rm, LOG_DEBUG, L"Device %d is enabled", chosenDevice);
            }
        }
    }

    std::string tempFile = std::tmpnam(nullptr);
    measure->outputA = tempFile + ".bmp";
    measure->outputW = Utils::StringToWString(measure->outputA);
}

PLUGIN_EXPORT void Reload(void * data, void * rm, double * maxValue)
{
	if (RmReadInt(rm, L"Disabled", 0) == 1 || RmReadInt(rm, L"Paused", 0) == 1)
	{
		return;
	}

	const auto measure = static_cast<Measure*>(data);

    measure->imgList.clear();
    measure->imgList.shrink_to_fit();

    std::wstring imgName = L"Image";
    int inpCount = 1;

    const auto begin = clock();
    while (true)
    {
        std::wstring image = RmReadString(rm, imgName.c_str(), L"");
        if (image.size() < 1)
            break;

        std::vector<Config> config = Utils::ParseConfig(image);

        measure->ParseExtend(config, imgName);

        auto imageCon = std::make_shared<ImgContainer>(inpCount - 1, config);

        if (!measure->GetImage(imageCon))
        {
            imageCon->img = ONEPIXEL;
        }

        measure->imgList.push_back(imageCon);

        imgName = L"Image" + std::to_wstring(++inpCount);
    }
    const auto end = clock();

    RmLogF(rm, LOG_DEBUG, L"Render time: %d", end - begin);

    measure->Compose();
}

PLUGIN_EXPORT double Update(void* data)
{
    return 0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
    auto measure = static_cast<const Measure*>(data);

    return measure->outputW.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
    const auto measure = static_cast<Measure*>(data);
    if (_wcsicmp(args, L"Update") == 0)
    {
        Reload(data, measure->rm, nullptr);
    }
    else if (_wcsnicmp(args, L"Reload", 6) == 0)
    {
        std::wstring list = args;

        list = list.substr(list.find_first_not_of(L" \t\r\n", 6));
        WSVector imageList = Utils::SeparateList(list, L",", NULL);
        for (auto &imageName : imageList)
        {
            const int index = Utils::NameToIndex(imageName);
            if (index >= 0 && index < measure->imgList.size())
            {
                auto outImg = measure->imgList.at(index);

                std::wstring image = RmReadString(measure->rm, imageName.c_str(), L"");
                std::vector<Config> config = Utils::ParseConfig(image);
                measure->ParseExtend(config, imageName);

                outImg->config = config;
                outImg->img = ONEPIXEL;

                if (!measure->GetImage(outImg))
                {
                    outImg->img = ONEPIXEL;
                }
            }
        }

        measure->Compose();
    }
}

PLUGIN_EXPORT void Finalize(void* data)
{
    const auto measure = static_cast<Measure*>(data);
    delete measure;
    Measure::refCount--;
    if (Measure::refCount == 0)
    {
        Magick::TerminateMagick();
    }
}

Measure::~Measure()
{
    std::error_code error;
    if (std::filesystem::exists(outputW, error))
        DeleteFile(outputW.c_str());
}

void Measure::LogError(Magick::Exception error) const noexcept
{
    wchar_t errorW[512];
    size_t * c = nullptr;
    mbstowcs_s(c, errorW, error.what(), 512 * sizeof(wchar_t));
    RmLog(rm, LOG_ERROR, errorW);
}

void Measure::ParseExtend(std::vector<Config> &parentVector, std::wstring parentName, BOOL isRecursion) const
{
    const size_t lastIndex = isRecursion ? 0 : 1;

    // Avoid first iteration
    for (size_t k = parentVector.size() - 1; k >= lastIndex; k--)
    {
        size_t insertIndex = k + 1;
        Config parentOption = parentVector.at(k);

        if (Utils::IsEqual(parentOption.name, L"EXTEND"))
        {
            auto extendList = Utils::SeparateList(parentOption.para, L",", 0);

            for (auto &extendName : extendList)
            {
                if (extendName.size() == 0)
                    continue;

                // Avoid Extend itself
                if (Utils::IsEqual(extendName, parentName)) continue;

                auto childVector = Utils::ParseConfig(
                    RmReadString(rm, extendName.c_str(), L""));

                ParseExtend(childVector, extendName, TRUE);

                for (auto child : childVector)
                {
                    parentVector.insert(parentVector.begin() + insertIndex, child);
                    insertIndex++;
                }
            }

            parentVector.at(k).isApplied = TRUE;
        }
    }
}

BOOL Measure::GetImage(std::shared_ptr<ImgContainer> curImg)
{
    Config &baseConfig = curImg->config.at(0);
    baseConfig.isApplied = TRUE;

    if (baseConfig.para.empty())
    {
        return FALSE;
    }

	const ImgType type = GetType(baseConfig);

	BOOL isValid = TRUE;

	ParseInternalVariable(baseConfig.para, curImg);

	switch (type)
	{
    case CLONE:
    {
        const int index = Utils::NameToIndex(baseConfig.para);

        if (index < imgList.size() && index >= 0)
        {
            curImg->img = imgList.at(index)->img;
            isValid = TRUE;
            break;
        }

        RmLogF(rm, LOG_ERROR, L"%s is invalid Image to clone.", baseConfig.para.c_str());
        isValid = FALSE;
        break;
    }
    case ELLIPSE:
    case PATH:
    case POLYGON:
    case RECTANGLE:
        isValid = CreateShape(type, curImg);
        break;
    case COMBINE:
        isValid = CreateCombine(curImg);
        break;
    case GRADIENT:
        isValid = CreateGradient(curImg);
        break;
	case NORMAL:
		isValid = CreateFromFile(curImg);
		break;
	case TEXT:
        isValid = CreateText(curImg);
		break;
    }

	if (!isValid || !curImg->img.isValid())
	{
        return FALSE;
    }

	if (type == COMBINE || type == CLONE)
	{
		try
		{
            curImg->geometry = curImg->img.boundingBox();
		}
		catch (Magick::Exception &error_)
		{
            curImg->geometry = Magick::Geometry{ 0,0,0,0 };
		}
	}

    for (auto &option : curImg->config)
	{
        if (option.isApplied)
            continue;

		ParseInternalVariable(option.para, curImg);

		if (!ParseEffect(*curImg, option))
			return FALSE;
	}

	return TRUE;
}

void Measure::Compose()
{
    auto finalImg = ONEPIXEL;
    finalImg.alpha(true);
    finalImg.backgroundColor(INVISIBLE);

    size_t newW = 0;
    size_t newH = 0;
    for (auto img : imgList)
    {
        if (img->isCombined || img->isIgnored) continue;

        //Extend main image size
        if (img->img.columns() > newW)
            newW = img->img.columns();

        if (img->img.rows() > newH)
            newH = img->img.rows();
    }

    finalImg.extent(Magick::Geometry(newW, newH));

    for (auto &img: imgList)
    {
        if (img->isCombined || img->isIgnored) continue;
        finalImg.composite(img->img, 0, 0, MagickCore::OverCompositeOp);
    }

    if (finalImg.isValid())
    {
        finalImg.write(outputA);
        std::wstring exportOption;

        exportOption = RmReadString(rm, L"ExportTo", L"");

        if (!exportOption.empty())
        {
            try
            {
                finalImg.write(Utils::WStringToString(exportOption));
            }
            catch (Magick::Exception &error_)
            {
                LogError(error_);
            }
        }
    }
    else
    {
        Magick::Image fallback = ONEPIXEL;
        fallback.write(outputA);
    }

    LPCWSTR finishAction = RmReadString(rm, L"OnFinishAction", L"");
    if (wcslen(finishAction) > 0)
    {
        RmExecute(skin, finishAction);
    }
}

BOOL Measure::ParseEffect(ImgContainer &container, Config &option)
{
	try
	{
		if (option.Match(L"MOVE"))
		{
			WSVector offsetXY = option.ToList(2);
			const ssize_t x = MathParser::ParseSSizeT(offsetXY[0]);
			const ssize_t y = MathParser::ParseSSizeT(offsetXY[1]);

            auto newSize = Magick::Geometry(
                container.img.columns() + x,
                container.img.rows() + y);

            auto tempImg = Magick::Image(newSize, INVISIBLE);

            tempImg.composite(container.img, x, y, Magick::OverCompositeOp);
            container.img = tempImg;

            container.geometry.xOff(container.geometry.xOff() + x);
            container.geometry.yOff(container.geometry.yOff() + y);
		}
		else if (option.Match(L"ADAPTIVEBLUR"))
		{
			WSVector valList = option.ToList(2);
			container.img.adaptiveBlur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			);
		}
		else if (option.Match(L"BLUR"))
		{
			WSVector valList = option.ToList(2);
			container.img.blur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			);
		}
		else if (option.Match(L"GAUSSIANBLUR"))
		{
			WSVector valList = option.ToList(2);
			container.img.gaussianBlur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			);
		}
		else if (option.Match(L"MOTIONBLUR"))
		{
			WSVector valList = option.ToList(3);
			container.img.motionBlur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1]),
				MathParser::ParseDouble(valList[2])
			);
		}
		else if (option.Match(L"NOISE"))
		{
			WSVector valList = option.ToList(2, L"");
			double density = 1.0;
			if (valList.size() > 1 && !valList[1].empty())
				density = MathParser::ParseDouble(valList[1]);


			container.img.addNoise(
				(Magick::NoiseType)MathParser::ParseInt(valList[0]),
				density
			);
		}
		else if (option.Match(L"SHADOW"))
		{
			WSVector dropShadow = Utils::SeparateList(option.para, L";", 2, L"");
			WSVector valList = Utils::SeparateParameter(dropShadow[0], 5);

			double sigma = MathParser::ParseDouble(valList[1]);
			const ssize_t offsetX = MathParser::ParseInt(valList[2]);
            const ssize_t offsetY = MathParser::ParseInt(valList[3]);

			Magick::Image tempShadow = container.img;

			if (!dropShadow[1].empty())
				tempShadow.backgroundColor(Utils::ParseColor(dropShadow[1]));
			else
				tempShadow.backgroundColor(Magick::Color("black"));

			if (sigma < 0) sigma = -sigma;
			tempShadow.shadow(
				MathParser::ParseDouble(valList[0]),
				sigma,
				0, 0
			);

			container.img.extent(Magick::Geometry(
				tempShadow.columns() + offsetX + (size_t)sigma,
				tempShadow.rows() + offsetY + (size_t)sigma
			), INVISIBLE);

			if (MathParser::ParseInt(valList[4]) == 1)
			{
				container.img.erase();
				container.img.composite(
					tempShadow,
                    offsetX - (ssize_t)(sigma * 2),
                    offsetY - (ssize_t)(sigma * 2),
					MagickCore::OverCompositeOp
				);
			}
			else
				container.img.composite(
					tempShadow,
					(ssize_t)(offsetX - sigma * 2),
					(ssize_t)(offsetY - sigma * 2),
					MagickCore::DstOverCompositeOp
				);

		}
		else if (option.Match(L"INNERSHADOW"))
		{
			//TODO: Crispy outline. fix it.
			WSVector dropShadow = Utils::SeparateList(option.para, L";", 2, L"");
			WSVector valList = Utils::SeparateParameter(dropShadow[0], 5);
			const double sigma = MathParser::ParseDouble(valList[1]);
			const ssize_t offsetX = MathParser::ParseInt(valList[2]);
			const ssize_t offsetY = MathParser::ParseInt(valList[3]);
			Magick::Image temp(Magick::Geometry(
				container.img.columns() + offsetX + (size_t)(sigma * 4),
				container.img.rows() + offsetY + (size_t)(sigma * 4)
			), Magick::Color("black"));
			temp.alpha(true);
			temp.composite(
				container.img,
				(size_t)sigma * 2,
				(size_t)sigma * 2,
				Magick::DstOutCompositeOp
			);
			temp.backgroundColor(Utils::ParseColor(dropShadow[1]));
			temp.shadow(
				MathParser::ParseDouble(valList[0]),
				sigma,
				0, 0
			);

			Magick::Image shadowOnly = container.img;
			shadowOnly.alpha(true);

			shadowOnly.composite(
				temp,
                offsetX - (ssize_t)(sigma * 4),
                offsetY - (ssize_t)(sigma * 4),
				Magick::SrcInCompositeOp
			);

			if (MathParser::ParseInt(valList[4]) == 1)
				container.img = shadowOnly;
			else
				container.img.composite(shadowOnly, 0, 0, Magick::OverCompositeOp);
		}
		else if (option.Match(L"DISTORT"))
		{
			WSVector valList = Utils::SeparateList(option.para, L";", 3);
			WSVector rawList = Utils::SeparateParameter(valList.at(1), NULL);

			std::vector<double> doubleList;
			for (auto &oneDouble : rawList)
				doubleList.push_back(MathParser::ParseDouble(oneDouble));

			const double * doubleArray = &*doubleList.begin();
			container.img.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
			container.img.distort(
				(Magick::DistortMethod)MathParser::ParseInt(valList[0]),
				doubleList.size(),
				doubleArray,
				MathParser::ParseBool(valList[2])
			);
		}
		//Same as Distort Perspective but this only requires position of 4 final points
		else if (option.Match(L"PERSPECTIVE"))
		{
			WSVector rawList = option.ToList(NULL);

			if (rawList.size() < 8)
			{
				RmLog(rm, LOG_WARNING, L"Perspective: Not enough control point. Requires 4 pairs of X and Y.");
				return TRUE;
			}
			double doubleArray[16];
			//Top Left
			doubleArray[0]  = (double)container.geometry.xOff();
			doubleArray[1]  = (double)container.geometry.yOff();
			doubleArray[2]  = Utils::ParseNumber2(rawList[0].c_str(), doubleArray[0], MathParser::ParseDouble);
			doubleArray[3]  = Utils::ParseNumber2(rawList[1].c_str(), doubleArray[1], MathParser::ParseDouble);
			//Top Right
			doubleArray[4]  = (double)(container.geometry.xOff() + container.geometry.width());
			doubleArray[5]  = (double)container.geometry.yOff();
			doubleArray[6]  = Utils::ParseNumber2(rawList[2].c_str(), doubleArray[4], MathParser::ParseDouble);
			doubleArray[7]  = Utils::ParseNumber2(rawList[3].c_str(), doubleArray[5], MathParser::ParseDouble);
			//Bottom Right
			doubleArray[8]  = (double)(container.geometry.xOff() + container.geometry.width());
			doubleArray[9]  = (double)(container.geometry.yOff() + container.geometry.height());
			doubleArray[10] = Utils::ParseNumber2(rawList[4].c_str(), doubleArray[8], MathParser::ParseDouble);
			doubleArray[11] = Utils::ParseNumber2(rawList[5].c_str(), doubleArray[9], MathParser::ParseDouble);
			//Bottom Left
			doubleArray[12] = (double)container.geometry.xOff();
			doubleArray[13] = (double)(container.geometry.yOff() + container.geometry.height());
			doubleArray[14] = Utils::ParseNumber2(rawList[6].c_str(), doubleArray[12], MathParser::ParseDouble);
			doubleArray[15] = Utils::ParseNumber2(rawList[7].c_str(), doubleArray[13], MathParser::ParseDouble);

			container.img.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
			container.img.distort(
				MagickCore::PerspectiveDistortion,
				16,
				doubleArray,
				true
			);
		}
		else if (option.Match(L"OPACITY"))
		{
			double a = option.ToDouble();
			if (a > 100) a = 100;
			if (a < 0) a = 0;
			a /= 100;

            auto temp = Magick::Image(
                Magick::Geometry(container.img.columns(), container.img.rows()),
                Magick::ColorRGB(1, 1, 1, a)
            );

            container.img.composite(temp, 0, 0, Magick::DstInCompositeOp);
		}
		else if (option.Match(L"SHADE"))
		{
			WSVector valList = option.ToList(3);

			container.img.shade(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1]),
				MathParser::ParseInt(valList[2]) == 1
			);
		}
		else if (option.Match(L"CHANNEL"))
		{
			if (option.Equal(L"RED"))
			{
				container.img = container.img.separate(Magick::RedChannel);
			}
			else if (option.Equal(L"GREEN"))
			{
				container.img = container.img.separate(Magick::GreenChannel);
			}
			else if (option.Equal(L"BLUE"))
			{
				container.img = container.img.separate(Magick::BlueChannel);
			}
			else if (option.Equal(L"BLACK"))
			{
				container.img = container.img.separate(Magick::BlackChannel);
			}
			else if (option.Equal(L"OPACITY"))
			{
				container.img = container.img.separate(Magick::OpacityChannel);
			}
		}
		else if (option.Match(L"MODULATE"))
		{
			WSVector valList = option.ToList(3, L"100");
			container.img.modulate(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1]),
				MathParser::ParseInt(valList[2])
			);
		}
		else if (option.Match(L"OILPAINT"))
		{
			WSVector valList = option.ToList(2);
			container.img.oilPaint(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);
		}
		else if (option.Match(L"SHEAR"))
		{
			WSVector valList = option.ToList(2);
			container.img.shear(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);
		}
		else if (option.Match(L"RESIZE"))
		{
			WSVector valList = option.ToList(3);
            Magick::Geometry newSize(
                MathParser::ParseInt(valList.at(0)),
                MathParser::ParseInt(valList.at(1))
            );
            if (valList.size() > 2)
                Utils::SetGeometryMode(MathParser::ParseInt(valList.at(2)), newSize);

			container.img.resize(newSize);
		}
		else if (option.Match(L"ADAPTIVERESIZE"))
		{
			WSVector valList = option.ToList(3);
			Magick::Geometry newSize(
				MathParser::ParseInt(valList.at(0)),
				MathParser::ParseInt(valList.at(1))
			);
            if (valList.size() > 2)
                Utils::SetGeometryMode(MathParser::ParseInt(valList.at(2)), newSize);

			container.img.adaptiveResize(newSize);
		}
		else if (option.Match(L"SCALE"))
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

                if(valList.size() > 2)
                    Utils::SetGeometryMode(MathParser::ParseInt(valList[2]), newSize);
			}

			container.img.scale(newSize);
		}
		else if (option.Match(L"SAMPLE"))
		{
			WSVector valList = option.ToList(2);
            Magick::Geometry newSize(
                MathParser::ParseInt(valList.at(0)),
                MathParser::ParseInt(valList.at(1))
            );
            if (valList.size() > 2)
                Utils::SetGeometryMode(MathParser::ParseInt(valList.at(2)), newSize);

			container.img.sample(newSize);
		}
		else if (option.Match(L"RESAMPLE"))
		{
			container.img.resample(option.ToDouble());
		}
		else if (option.Match(L"CROP"))
		{
			WSVector geometryRaw = option.ToList(5);
			ssize_t x = MathParser::ParseSSizeT(geometryRaw[0]);
            ssize_t y = MathParser::ParseSSizeT(geometryRaw[1]);
            size_t w = MathParser::ParseSizeT(geometryRaw[2]);
            size_t h = MathParser::ParseSizeT(geometryRaw[3]);
			const int origin = MathParser::ParseInt(geometryRaw[4]);

			if (w < 0 || h < 0)
			{
				RmLog(rm, LOG_WARNING, L"Crop: Invalid width or height value.");
				return TRUE;
			}
			else if (origin > 5 || origin < 0)
			{
				RmLog(rm, LOG_WARNING, L"Crop: Invalid Origin value. Left Top origin is used.");
				return TRUE;
			}

			//TOP LEFT anchor is default anchor.
			switch (origin)
			{
			case 2: //TOP RIGHT
				x += container.img.columns();
				break;
			case 3: //BOTTOM RIGHT
				x += container.img.columns();
				y += container.img.rows();
				break;
			case 4: //BOTTOM LEFT
				y += container.img.rows();
				break;
			case 5: //CENTER
				x += container.img.columns() / 2;
				y += container.img.rows() / 2;
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
			if ((w + x) > container.img.columns())
				w = container.img.columns() - x;

			if ((h + y) > container.img.rows())
				h = container.img.rows() - y;

			container.img.crop(Magick::Geometry(w, h, x, y));
            // Move cropped image to top left of canvas
            container.img.page(Magick::Geometry(0, 0, 0, 0));
        }
		else if (option.Match(L"ROTATIONALBLUR"))
		{
			container.img.rotationalBlur(option.ToDouble());
		}
		else if (option.Match(L"COLORIZE"))
		{
			WSVector valList = Utils::SeparateList(option.para, L";", 2);
			container.img.colorize(
				(unsigned int)MathParser::ParseInt(valList[0]),
				Utils::ParseColor(valList[1])
			);
		}
        else if (option.Match(L"CHARCOAL"))
        {
            WSVector valList = option.ToList(0);
            const auto vSize = valList.size();
            if (vSize == 0)
            {
                container.img.charcoal();
            }
            else if (vSize == 1)
            {
                container.img.charcoal(MathParser::ParseDouble(valList.at(0)));
            }
            else
            {
                container.img.charcoal(
                    MathParser::ParseDouble(valList.at(0)),
                    MathParser::ParseDouble(valList.at(1))
                );
            }
        }
		else if (option.Match(L"GRAYSCALE"))
		{
			const int method = MathParser::ParseInt(option.para);
			if (method > 0 && method <= 9)
				container.img.grayscale((Magick::PixelIntensityMethod)method);
		}
		else if (option.Match(L"ROLL"))
		{
			WSVector valList = option.ToList(2, L"0");
			size_t c = MathParser::ParseSizeT(valList[0]);
            size_t r = MathParser::ParseSizeT(valList[1]);
			if (c < 0)
				c += container.img.columns();
			if (r < 0)
				r += container.img.rows();

			container.img.roll(c, r);
		}
		else if (option.Match(L"NEGATE"))
		{
			container.img.negate(MathParser::ParseBool(option.para));
		}
		else if (option.Match(L"IMPLODE"))
		{
			container.img.implode(option.ToDouble());
		}
		else if (option.Match(L"SPREAD"))
		{
			container.img.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
			container.img.spread(option.ToDouble());
		}
		else if (option.Match(L"SWIRL"))
		{
			container.img.swirl(option.ToDouble());
		}
		else if (option.Match(L"MEDIANFILTER"))
		{
			container.img.medianFilter(option.ToDouble());
		}
		else if (option.Match(L"EQUALIZE"))
		{
			container.img.equalize();
		}
		else if (option.Match(L"ENHANCE"))
		{
			container.img.enhance();
		}
		else if (option.Match(L"DESPECKLE"))
		{
			container.img.despeckle();
		}
		else if (option.Match(L"REDUCENOISE"))
		{
			container.img.reduceNoise();
		}
		else if (option.Match(L"TRANSPOSE"))
		{
			container.img.transpose();
		}
		else if (option.Match(L"TRANSVERSE"))
		{
			container.img.transverse();
		}
		else if (option.Match(L"FLIP"))
		{
			container.img.flip();
		}
		else if (option.Match(L"FLOP"))
		{
			container.img.flop();
		}
		else if (option.Match(L"MAGNIFY"))
		{
			container.img.magnify();
		}
		else if (option.Match(L"MINIFY"))
		{
			container.img.minify();
		}
		else if (option.Match(L"IGNORE"))
		{
			container.isIgnored = MathParser::ParseBool(option.para);
		}

		return TRUE;
	}
	catch (Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}
}

void Measure::ParseInternalVariable(std::wstring &rawSetting, std::shared_ptr<ImgContainer> srcImg)
{
	size_t start = rawSetting.find(L"{");
	while (start != std::wstring::npos)
	{
        const size_t end = rawSetting.find(L"}", start + 1);
		if (end != std::wstring::npos)
		{
			std::wstring f = rawSetting.substr(start + 1, end - start - 1);

			WSVector v = Utils::SeparateList(f, L":", 2, L"");
            const int index = Utils::NameToIndex(v.at(0));
			if (index != -1 && index <= srcImg->index)
			{
				std::shared_ptr<ImgContainer> tempStruct = nullptr;
				if (index == srcImg->index)
					tempStruct = srcImg;
				else
					tempStruct = imgList.at(index);

				LPCWSTR para = v.at(1).c_str();
				std::wstring c = L"";
				if (_wcsnicmp(para, L"X", 1) == 0)
				{
					c = std::to_wstring(tempStruct->geometry.xOff());
				}
				else if (_wcsnicmp(para, L"Y", 1) == 0)
				{
					c = std::to_wstring(tempStruct->geometry.yOff());
				}
				else if (_wcsnicmp(para, L"W", 1) == 0)
				{
					c = std::to_wstring(tempStruct->geometry.width());
				}
				else if (_wcsnicmp(para, L"H", 1) == 0)
				{
					c = std::to_wstring(tempStruct->geometry.height());
				}

                if (_wcsnicmp(para, L"COLOR", 5) == 0 &&
                    tempStruct->colorList.size() < 5
                )
                {
                    GenColor(tempStruct->img, 5, tempStruct->colorList);
                }

				if (_wcsicmp(para, L"COLORBG") == 0)
				{
					c = Utils::ColorToWString(tempStruct->colorList.at(0));
				}
				else if (_wcsicmp(para, L"COLOR1") == 0)
				{
					c = Utils::ColorToWString(tempStruct->colorList.at(1));
				}
				else if (_wcsicmp(para, L"COLOR2") == 0)
				{
					c = Utils::ColorToWString(tempStruct->colorList.at(2));
				}
				else if (_wcsicmp(para, L"COLOR3") == 0)
				{
					c = Utils::ColorToWString(tempStruct->colorList.at(3));
				}
				else if (_wcsicmp(para, L"COLORFG") == 0)
				{
					c = Utils::ColorToWString(tempStruct->colorList.at(4));
				}

				if (!c.empty())
				{
					rawSetting.replace(
						rawSetting.begin() + start,
						rawSetting.begin() + end + 1,
						c
					);
					start = rawSetting.find(L"{");
				}
				else
				{
					start = rawSetting.find(L"{", end + 1);
				}

				continue;
			}
			else
			{
				start = rawSetting.find(L"{", end + 1);
				continue;
			}
		}
        else
        {
            break;
        }
	}
}

void GenColor(Magick::Image img, size_t totalColor, std::vector<Magick::Color> &outContainer)
{
	Magick::Geometry smallSize(25, 25);
	smallSize.percent(true);
	img.resize(smallSize);
	img.quantizeColors(totalColor);
	img.quantize();
	Magick::Image uniqueColor = img.uniqueColors();

    outContainer.clear();
    outContainer.shrink_to_fit();

	for (int i = 1; i <= uniqueColor.columns(); i++)
        outContainer.push_back(uniqueColor.pixelColor(i, 1));

	//Fill missing color. with last generated color.
	while (outContainer.size() < totalColor)
        outContainer.push_back(outContainer.at(outContainer.size() - 1));
}
