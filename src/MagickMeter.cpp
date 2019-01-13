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

        measure->InsertExtend(config, imgName);

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
                measure->InsertExtend(config, imageName);

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

void Measure::InsertExtend(std::vector<Config> &parentVector, std::wstring parentName, BOOL isRecursion) const
{
    const ssize_t lastIndex = isRecursion ? 0 : 1;

    // Avoid first iteration
    for (ssize_t k = parentVector.size() - 1; k >= lastIndex; k--)
    {
        size_t insertIndex = k + 1;

        Config &parentOption = parentVector.at(k);

        if (parentOption.Match(L"EXTEND"))
        {
            auto extendList = parentOption.ToList();

            for (auto extendName : extendList)
            {
                if (extendName.size() == 0) continue;

                // Avoid Extend itself
                if (Utils::IsEqual(extendName, parentName)) continue;

                std::wstring rawChild = RmReadString(rm, extendName.c_str(), L"");

                if (rawChild.length() == 0) continue;

                auto childVector = Utils::ParseConfig(rawChild);

                InsertExtend(childVector, extendName, TRUE);

                for (auto child : childVector)
                {
                    parentVector.insert(parentVector.begin() + insertIndex, child);
                    insertIndex++;
                }
            }

            parentOption.isApplied = TRUE;
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

	ReplaceInternalVariable(baseConfig.para, curImg);

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

		ReplaceInternalVariable(option.para, curImg);

		if (!ApplyEffect(curImg, option))
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

void Measure::ReplaceInternalVariable(std::wstring &rawSetting, std::shared_ptr<ImgContainer> srcImg)
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
