#include "MagickMeter.h"
#include <sstream>

int Measure::refCount = 0;

ImgType GetType(std::wstring input) noexcept
{
	LPCWSTR inP = input.c_str();
	if		(_wcsnicmp(inP, L"FILE", 4) == 0)			return NORMAL;
	else if (_wcsnicmp(inP, L"TEXT", 4) == 0)			return TEXT;
	else if (_wcsnicmp(inP, L"PATH", 4) == 0)			return PATH;
	else if (_wcsnicmp(inP, L"CLONE", 5) == 0)			return CLONE;
	else if (_wcsnicmp(inP, L"ELLIPSE", 7) == 0)		return ELLIPSE;
	else if (_wcsnicmp(inP, L"POLYGON", 7) == 0)		return POLYGON;
	else if (_wcsnicmp(inP, L"COMBINE", 7) == 0)		return COMBINE;
	else if (_wcsnicmp(inP, L"GRADIENT", 8) == 0)		return GRADIENT;
	else if (_wcsnicmp(inP, L"RECTANGLE", 9) == 0)		return RECTANGLE;
	else												return NOTYPE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
    Measure::refCount++;

	measure->rm = rm;
	measure->skin = RmGetSkin(rm);
	Magick::InitializeMagick("\\");

    if (Magick::EnableOpenCL())
    {
        RmLog(measure->rm, LOG_DEBUG, L"MagickMeter: OpenCL supported");
    }

    std::ostringstream fileName;
    fileName << std::tmpnam(nullptr) << ".bmp";
    measure->outputA = fileName.str();
    measure->outputW = Utils::StringToWString(fileName.str());
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
    while (true)
    {

        if (!measure->GetImage(imgName, TRUE))
            break;
        imgName = L"Image" + std::to_wstring(++inpCount);
    }

    measure->Compose();
}

PLUGIN_EXPORT double Update(void* data)
{
    return 0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
    const auto measure = static_cast<Measure*>(data);

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
        for (auto &image : imageList)
        {
            const int index = Utils::NameToIndex(image);
            if (index >= 0 && index < measure->imgList.size())
            {
                measure->imgList[index].img.erase();
                measure->GetImage(image, FALSE);
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

void Measure::ParseExtend(WSVector &parentVector, std::wstring parentName, BOOL isRecursion) const
{
    for (int k = (int)parentVector.size() - 1; k >= 1 - (int)isRecursion; k--) // Avoid first iteration
    {
        int insertIndex = k + 1;
        if (_wcsnicmp(parentVector[k].c_str(), L"EXTEND", 6) == 0)
        {
            auto extendList = Utils::SeparateList(parentVector[k].substr(6), L",", NULL);
            for (int i = 0; i < extendList.size(); i++)
            {
                if (_wcsicmp(parentName.c_str(), extendList[i].c_str()) == 0) continue; //Avoid Extend itself

                auto childVector = Utils::SeparateList(
                    RmReadString(rm, extendList[i].c_str(), L""),
                    L"|", NULL
                );

                ParseExtend(childVector, extendList[i], TRUE);

                for (int j = 0; j < childVector.size(); j++)
                {
                    parentVector.insert(parentVector.begin() + insertIndex, childVector[j]);
                    insertIndex++;
                }
            }
            parentVector.erase(parentVector.begin() + k);
        }
    }
}

BOOL Measure::GetImage(std::wstring imageName, BOOL isPush)
{
	LPCWSTR rawSetting = RmReadString(rm, imageName.c_str(), L"");

	if (wcslen(rawSetting) == 0) return FALSE;

	WSVector config = Utils::SeparateList(rawSetting, L"|", NULL);
    if (config.empty()) return FALSE;

	ParseExtend(config, imageName);

	ImgContainer curImg = ImgContainer(Utils::NameToIndex(imageName));

    std::wstring typeName;
    std::wstring typePara;
	Utils::GetNamePara(config[0], typeName, typePara);
    config[0].clear();

	const ImgType type = GetType(typeName);

	BOOL isValid = TRUE;
	if (!typePara.empty())
	{
		ParseInternalVariable(typePara, curImg);
		switch (type)
		{
        case CLONE:
        {
            const int index = Utils::NameToIndex(typePara);

            if (index < imgList.size() && index >= 0)
            {
                curImg.img = imgList[index].img;
                isValid = TRUE;
                break;
            }

            RmLogF(rm, LOG_ERROR, L"%s is invalid Image to clone.", config[0].c_str());
            isValid = FALSE;
            break;
        }
        case ELLIPSE:
        case PATH:
        case POLYGON:
        case RECTANGLE:
            isValid = CreateShape(type, typePara.c_str(), config, curImg);
            break;
        case COMBINE:
            isValid = CreateCombine(typePara.c_str(), config, curImg);
            break;
        case GRADIENT:
            isValid = CreateGradient(typePara.c_str(), config, curImg);
            break;
		case NORMAL:
			isValid = CreateFromFile(typePara.c_str(), config, curImg);
			break;
		case TEXT:
            isValid = CreateText(typePara, config, curImg);
			break;
        }
    }

	if (!isValid || !curImg.img.isValid())
	{
        RmLogF(rm, LOG_ERROR, L"%s is invalid. Check its modifiers and effects again.", imageName.c_str());
        return FALSE;
    }

	if (type == COMBINE || type == CLONE)
	{
		Magick::Geometry newBound;
		try
		{
			newBound = curImg.img.boundingBox();
			curImg.X = newBound.xOff();
			curImg.Y = newBound.yOff();
			curImg.W = newBound.width();
			curImg.H = newBound.height();
		}
		catch (Magick::Exception &error_)
		{
			curImg.X = 0;
			curImg.Y = 0;
			curImg.W = 0;
			curImg.H = 0;
		}
	}

    for (auto &option : config)
	{
        if (option.empty())
            continue;

		std::wstring name, parameter;
		Utils::GetNamePara(option, name, parameter);

		ParseInternalVariable(parameter, curImg);

		if (!ParseEffect(curImg, name, parameter))
			return FALSE;
	}

    if (isPush)
        imgList.push_back(curImg);
    else
        imgList[curImg.index] = curImg;

	return TRUE;
}

void Measure::Compose()
{
    auto finalImg = ONEPIXEL;
    finalImg.alpha(true);
    finalImg.backgroundColor(INVISIBLE);

    size_t newW = 0;
    size_t newH = 0;
    for (auto &img : imgList)
    {
        if (img.isCombined || img.isIgnored) continue;

        //Extend main image size
        if (img.img.columns() > newW)
            newW = img.img.columns();

        if (img.img.rows() > newH)
            newH = img.img.rows();
    }

    finalImg.extent(Magick::Geometry(newW, newH));

    for (auto &img: imgList)
    {
        if (img.isCombined || img.isIgnored) continue;
        finalImg.composite(img.img, 0, 0, MagickCore::OverCompositeOp);
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
BOOL Measure::ParseEffect(ImgContainer &container, std::wstring name, std::wstring parameter)
{
	LPCWSTR effect = name.c_str();
	try
	{
		if (_wcsicmp(effect, L"MOVE") == 0)
		{
			WSVector offsetXY = Utils::SeparateParameter(parameter, 2);
			const ssize_t x = MathParser::ParseSSizeT(offsetXY[0]);
			const ssize_t y = MathParser::ParseSSizeT(offsetXY[1]);

            auto newSize = Magick::Geometry(
                container.img.columns() + x,
                container.img.rows() + y);

            auto tempImg = Magick::Image(newSize, INVISIBLE);

            tempImg.composite(container.img, x, y, Magick::OverCompositeOp);
            container.img = tempImg;

            container.X += x;
            container.Y += y;
		}
		else if (_wcsicmp(effect, L"ADAPTIVEBLUR") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
			container.img.adaptiveBlur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"BLUR") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
			container.img.blur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"GAUSSIANBLUR") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
			container.img.gaussianBlur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"MOTIONBLUR") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 3);
			container.img.motionBlur(
				MathParser::ParseDouble(valList[0]),
				MathParser::ParseDouble(valList[1]),
				MathParser::ParseDouble(valList[2])
			);
		}
		else if (_wcsicmp(effect, L"NOISE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2, L"");
			double density = 1.0;
			if (valList.size() > 1 && !valList[1].empty())
				density = MathParser::ParseDouble(valList[1]);


			container.img.addNoise(
				(Magick::NoiseType)MathParser::ParseInt(valList[0]),
				density
			);
		}
		else if (_wcsicmp(effect, L"SHADOW") == 0)
		{
			WSVector dropShadow = Utils::SeparateList(parameter, L";", 2, L"");
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
		else if (_wcsicmp(effect, L"INNERSHADOW") == 0)
		{
			//TODO: Cripsy outline. fix it.
			WSVector dropShadow = Utils::SeparateList(parameter, L";", 2, L"");
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
		else if (_wcsicmp(effect, L"DISTORT") == 0)
		{
			WSVector valList = Utils::SeparateList(parameter, L";", 3);
			WSVector rawList = Utils::SeparateParameter(valList[1], NULL);

			std::vector<double> doubleList;
			for (auto &oneDouble : rawList)
				doubleList.push_back(MathParser::ParseDouble(oneDouble));

			const double * doubleArray = &doubleList[0];
			container.img.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
			container.img.distort(
				(Magick::DistortMethod)MathParser::ParseInt(valList[0]),
				doubleList.size(),
				doubleArray,
				MathParser::ParseBool(valList[2])
			);
		}
		//Same as Distort Perspective but this only requires position of 4 final points
		else if (_wcsicmp(effect, L"PERSPECTIVE") == 0)
		{
			WSVector rawList = Utils::SeparateParameter(parameter, NULL);

			if (rawList.size() < 8)
			{
				RmLog(rm, LOG_WARNING, L"Perspective: Not enough control point. Requires 4 pairs of X and Y.");
				return TRUE;
			}
			double doubleArray[16];
			//Top Left
			doubleArray[0]  = (double)container.X;
			doubleArray[1]  = (double)container.Y;
			doubleArray[2]  = Utils::ParseNumber2(rawList[0].c_str(), doubleArray[0], MathParser::ParseDouble);
			doubleArray[3]  = Utils::ParseNumber2(rawList[1].c_str(), doubleArray[1], MathParser::ParseDouble);
			//Top Right
			doubleArray[4]  = (double)(container.X + container.W);
			doubleArray[5]  = (double)container.Y;
			doubleArray[6]  = Utils::ParseNumber2(rawList[2].c_str(), doubleArray[4], MathParser::ParseDouble);
			doubleArray[7]  = Utils::ParseNumber2(rawList[3].c_str(), doubleArray[5], MathParser::ParseDouble);
			//Bottom Right
			doubleArray[8]  = (double)(container.X + container.W);
			doubleArray[9]  = (double)(container.Y + container.H);
			doubleArray[10] = Utils::ParseNumber2(rawList[4].c_str(), doubleArray[8], MathParser::ParseDouble);
			doubleArray[11] = Utils::ParseNumber2(rawList[5].c_str(), doubleArray[9], MathParser::ParseDouble);
			//Bottom Left
			doubleArray[12] = (double)container.X;
			doubleArray[13] = (double)(container.Y + container.H);
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
		else if (_wcsicmp(effect, L"OPACITY") == 0)
		{
			double a = MathParser::ParseDouble(parameter);
			if (a > 100) a = 100;
			if (a < 0) a = 0;
			a /= 100;

			container.img.alpha((unsigned int)(a * QuantumRange));
		}
		else if (_wcsicmp(effect, L"SHADE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 3);

			container.img.shade(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1]),
				MathParser::ParseInt(valList[2]) == 1
			);
		}
		else if (_wcsicmp(effect, L"CHANNEL") == 0)
		{
            LPCWSTR channelColor = parameter.c_str();
			if (_wcsicmp(channelColor, L"RED") == 0)
			{
				container.img = container.img.separate(Magick::RedChannel);
			}
			else if (_wcsicmp(channelColor, L"GREEN") == 0)
			{
				container.img = container.img.separate(Magick::GreenChannel);
			}
			else if (_wcsicmp(channelColor, L"BLUE") == 0)
			{
				container.img = container.img.separate(Magick::BlueChannel);
			}
			else if (_wcsicmp(channelColor, L"BLACK") == 0)
			{
				container.img = container.img.separate(Magick::BlackChannel);
			}
			else if (_wcsicmp(channelColor, L"OPACITY") == 0)
			{
				container.img = container.img.separate(Magick::OpacityChannel);
			}
		}
		else if (_wcsicmp(effect, L"MODULATE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 3, L"100");
			container.img.modulate(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1]),
				MathParser::ParseInt(valList[2])
			);
		}
		else if (_wcsicmp(effect, L"OILPAINT") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
			container.img.oilPaint(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"SHEAR") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
			container.img.shear(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"RESIZE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 3);
			Magick::Geometry newSize(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);

			switch (MathParser::ParseInt(valList[2]))
			{
			case 1:
				newSize.aspect(true);
				break;
			case 2:
				newSize.fillArea(true);
				break;
			case 3:
				newSize.greater(true);
				break;
			case 4:
				newSize.less(true);
				break;
			}

			container.img.resize(newSize);
		}
		else if (_wcsicmp(effect, L"ADAPTIVERESIZE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 3);
			Magick::Geometry newSize(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);

			switch (MathParser::ParseInt(valList[2]))
			{
			case 1:
				newSize.aspect(true);
				break;
			case 2:
				newSize.fillArea(true);
				break;
			case 3:
				newSize.greater(true);
				break;
			case 4:
				newSize.less(true);
				break;
			}

			container.img.adaptiveResize(newSize);
		}
		else if (_wcsicmp(effect, L"SCALE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
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
		else if (_wcsicmp(effect, L"SAMPLE") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2);
			Magick::Geometry newSize(
				MathParser::ParseInt(valList[0]),
				MathParser::ParseInt(valList[1])
			);

			switch (MathParser::ParseInt(valList[2]))
			{
			case 1:
				newSize.aspect(true);
				break;
			case 2:
				newSize.fillArea(true);
				break;
			case 3:
				newSize.greater(true);
				break;
			case 4:
				newSize.less(true);
				break;
			}

			container.img.sample(newSize);
		}
		else if (_wcsicmp(effect, L"RESAMPLE") == 0)
		{
			container.img.resample(MathParser::ParseDouble(parameter));
		}
		else if (_wcsicmp(effect, L"CROP") == 0)
		{
			WSVector geometryRaw = Utils::SeparateParameter(parameter, 5);
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
            container.img.page(Magick::Geometry(0, 0, 0, 0)); // Move cropped image to top left of canvas
        }
		else if (_wcsicmp(effect, L"ROTATIONALBLUR") == 0)
		{
			container.img.rotationalBlur(MathParser::ParseDouble(parameter));
		}
		else if (_wcsicmp(effect, L"COLORIZE") == 0)
		{
			WSVector valList = Utils::SeparateList(parameter, L";", 2);
			container.img.colorize(
				(unsigned int)MathParser::ParseInt(valList[0]),
				Utils::ParseColor(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"GRAYSCALE") == 0)
		{
			const int method = MathParser::ParseInt(parameter);
			if (method > 0 && method <= 9)
				container.img.grayscale((Magick::PixelIntensityMethod)method);
		}
		else if (_wcsicmp(effect, L"ROLL") == 0)
		{
			WSVector valList = Utils::SeparateParameter(parameter, 2, L"0");
			size_t c = MathParser::ParseSizeT(valList[0]);
            size_t r = MathParser::ParseSizeT(valList[1]);
			if (c < 0)
				c += container.img.columns();
			if (r < 0)
				r += container.img.rows();

			container.img.roll(c, r);
		}
		else if (_wcsicmp(effect, L"NEGATE") == 0)
		{
			container.img.negate(MathParser::ParseBool(parameter));
		}
		else if (_wcsicmp(effect, L"IMPLODE") == 0)
		{
			container.img.implode(MathParser::ParseDouble(parameter));
		}
		else if (_wcsicmp(effect, L"SPREAD") == 0)
		{
			container.img.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
			container.img.spread(MathParser::ParseDouble(parameter));
		}
		else if (_wcsicmp(effect, L"SWIRL") == 0)
		{
			container.img.swirl(MathParser::ParseDouble(parameter));
		}
		else if (_wcsicmp(effect, L"MEDIANFILTER") == 0)
		{
			container.img.medianFilter(MathParser::ParseDouble(parameter));
		}
		else if (_wcsicmp(effect, L"EQUALIZE") == 0)
		{
			container.img.equalize();
		}
		else if (_wcsicmp(effect, L"ENHANCE") == 0)
		{
			container.img.enhance();
		}
		else if (_wcsicmp(effect, L"DESPECKLE") == 0)
		{
			container.img.despeckle();
		}
		else if (_wcsicmp(effect, L"REDUCENOISE") == 0)
		{
			container.img.reduceNoise();
		}
		else if (_wcsicmp(effect, L"TRANSPOSE") == 0)
		{
			container.img.transpose();
		}
		else if (_wcsicmp(effect, L"TRANSVERSE") == 0)
		{
			container.img.transverse();
		}
		else if (_wcsicmp(effect, L"FLIP") == 0)
		{
			container.img.flip();
		}
		else if (_wcsicmp(effect, L"FLOP") == 0)
		{
			container.img.flop();
		}
		else if (_wcsicmp(effect, L"MAGNIFY") == 0)
		{
			container.img.magnify();
		}
		else if (_wcsicmp(effect, L"MINIFY") == 0)
		{
			container.img.minify();
		}
		else if (_wcsicmp(effect, L"IGNORE") == 0)
		{
			container.isIgnored = MathParser::ParseBool(parameter);
		}

		return TRUE;
	}
	catch (Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}
}

void Measure::ParseInternalVariable(std::wstring &outSetting, ImgContainer &srcImg)
{
	size_t start = outSetting.find(L"{");
	while (start != std::wstring::npos)
	{
        const size_t end = outSetting.find(L"}", start + 1);
		if (end != std::wstring::npos)
		{
			std::wstring f = outSetting.substr(start + 1, end - start - 1);

			WSVector v = Utils::SeparateList(f, L":", 2, L"");
            const int index = Utils::NameToIndex(v[0]);
			if (index != -1 && index <= srcImg.index)
			{
				ImgContainer* tempStruct = nullptr;
				if (index == srcImg.index)
					tempStruct = &srcImg;
				else
					tempStruct = &imgList.at(index);

				LPCWSTR para = v[1].c_str();
				std::wstring c = L"";
				if (_wcsnicmp(para, L"X", 1) == 0)
				{
					c = std::to_wstring(tempStruct->X);
				}
				else if (_wcsnicmp(para, L"Y", 1) == 0)
				{
					c = std::to_wstring(tempStruct->Y);
				}
				else if (_wcsnicmp(para, L"W", 1) == 0)
				{
					c = std::to_wstring(tempStruct->W);
				}
				else if (_wcsnicmp(para, L"H", 1) == 0)
				{
					c = std::to_wstring(tempStruct->H);
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
					outSetting.replace(
						outSetting.begin() + start,
						outSetting.begin() + end + 1,
						c
					);
					start = outSetting.find(L"{");
				}
				else
				{
					start = outSetting.find(L"{", end + 1);
				}

				continue;
			}
			else
			{
				start = outSetting.find(L"{", end + 1);
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