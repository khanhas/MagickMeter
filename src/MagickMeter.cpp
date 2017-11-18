#include "MagickMeter.h"
#include "..\..\API\RainmeterAPI.h"

ImgType GetType(std::wstring input);
BOOL CreateNew(ImgStruct * dst, std::vector<std::wstring> setting, Measure * measure);
BOOL CreateText(ImgStruct * dst, std::vector<std::wstring> setting, Measure * measure);
BOOL CreateShape(ImgStruct * dst, std::vector<std::wstring> setting, ImgType shape, Measure * measure);
BOOL CreateCombine(ImgStruct * dst, std::vector<std::wstring> setting, std::vector<ImgStruct *> &list, Measure * measure);

ImgType GetType(std::wstring input)
{
	LPCWSTR inPtr = input.c_str();

	if		(_wcsnicmp(inPtr, L"TEXT", 4) == 0)			return TEXT;
	else if (_wcsnicmp(inPtr, L"ELLIPSE", 7) == 0)		return ELLIPSE;
	else if (_wcsnicmp(inPtr, L"RECTANGLE", 9) == 0)	return RECTANGLE;
	else if (_wcsnicmp(inPtr, L"COMBINE", 7) == 0)		return COMBINE;
	else if (_wcsnicmp(inPtr, L"IMAGE", 5) == 0)		return REFERENCE;
	else												return NORMAL;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
	measure->rm = rm;
	measure->name = RmGetMeasureName(rm);

	Magick::InitializeMagick("\\");

	std::wstring tempFolder = RmReplaceVariables(rm, L"%temp%");
	std::wstring disName = RmGetSkinName(rm);
	size_t slashPos = disName.find(L"\\", 0);
	while (slashPos != std::wstring::npos)
	{
		disName.replace(slashPos, 1, L"_");
		slashPos = disName.find(L"\\", slashPos);
	}
	measure->outputFile = tempFolder + L"\\" + disName + L"_" + measure->name + L".bmp";
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	measure->skin = RmGetSkin(rm);
	LPCWSTR imgInp = RmReadString(rm, L"Image", L"");
	int inpCount = 1;
	Magick::Image allImg;
	try
	{
		allImg.alpha(true);
		allImg.backgroundColor(Magick::Color("transparent"));
	}
	catch (std::exception &error_)
	{
		RmLogF(rm, 1, L"%s", error2pws(error_));

	}
	std::vector<ImgStruct *> imgList;

	while (*imgInp)
	{
		std::vector<std::wstring> imgVec = SeparateList(imgInp, L"|", NULL);

		ImgStruct * curImg = new ImgStruct;

		curImg->contain.alpha(true);
		curImg->contain.backgroundColor(Magick::Color("transparent"));

		ImgType type = GetType(imgVec[0]);

		BOOL isValid = TRUE;

		switch (type)
		{
		case NORMAL:
			isValid = CreateNew(curImg, imgVec, measure);
			break;
		case COMBINE:
			isValid = CreateCombine(curImg, imgVec, imgList, measure);
			break;
		case TEXT:
			isValid = CreateText(curImg, imgVec, measure);
			break;
		case ELLIPSE:
			isValid = CreateShape(curImg, imgVec, ELLIPSE, measure);
			break;
		case RECTANGLE:
			isValid = CreateShape(curImg, imgVec, RECTANGLE, measure);
			break;
		case REFERENCE:
			int index = NameToIndex(imgVec[0]);
			if (index < imgList.size() && index >= 0)
				curImg->contain = imgList[index]->contain;
			else
			{
				isValid = FALSE;
				break;
			}
			imgVec.erase(imgVec.begin());

			for (auto &settingIt : imgVec)
			{
				std::wstring name, parameter;
				GetNamePara(settingIt, name, parameter);

				if (!ParseEffect(rm, curImg->contain, name, parameter))
				{
					isValid = FALSE;
					break;
				}
					
			}
			break;

		}

		if (isValid || curImg->contain.isValid()) 
			imgList.push_back(curImg);

		std::wstring newImgName = L"Image" + std::to_wstring(++inpCount);
		imgInp = RmReadString(rm, newImgName.c_str(), L"");
	}

	for (int i = 0; i < imgList.size(); i++)
	{
		if (imgList[i]->isDelete) continue;

		//Extend main image size
		size_t newW = allImg.columns();
		if (imgList[i]->contain.columns() > newW)
			newW = imgList[i]->contain.columns();

		size_t newH = allImg.rows();
		if (imgList[i]->contain.rows() > newH)
			newH = imgList[i]->contain.rows();

		allImg.size(Magick::Geometry(newW, newH));

		allImg.composite(imgList[i]->contain, 0, 0, MagickCore::OverCompositeOp);
	}

	allImg.write(ws2s(measure->outputFile));
}


PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	if (measure->isGIF)
	{
		if (measure->GIFSeq >= (measure->gifList.size() - 1))
		{
			measure->GIFSeq = 0;
		}
		else
		{
			measure->GIFSeq++;
		}
		Reload(data, measure->rm, 0);
	}
	return NULL;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	return measure->outputFile.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	if (_wcsicmp(args, L"Update") == 0)
		Reload(data, measure->rm, 0);
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	delete measure;
}

void GetNamePara(std::wstring input, std::wstring& name, std::wstring& para)
{
	name = input.substr(0, input.find_first_of(L" \t\r\n"));
	if (name.length() != input.length())
		para = input.substr(input.find_first_not_of(L" \t\r\n", name.length()));
	else
		para = L"";
}

int NameToIndex(std::wstring name)
{
	if (_wcsnicmp(name.c_str(), L"IMAGE", 5) == 0)
	{
		if (name.length() == 5)
			return 0;
		else
		{
			int index = _wtoi(name.substr(5).c_str()) - 1;
			if (index >= 0)
				return index;
		}
	}

	return -1;
}

std::wstring TrimString(std::wstring bloatedString)
{
	size_t start = bloatedString.find_first_not_of(L" \t\r\n", 0);
	std::wstring trimmedString = bloatedString.substr(start, bloatedString.length());
	trimmedString.erase(trimmedString.find_last_not_of(L" \t\r\n") + 1);
	return trimmedString;
}

std::vector<std::wstring> SeparateList(LPCWSTR rawString, wchar_t* separtor, int maxElement, wchar_t* defValue)
{
	std::vector<std::wstring> vectorList;

	if (!*rawString)
		return vectorList;

	std::wstring tempList = rawString;
	size_t start = 0;
	size_t end = tempList.find(separtor, 0);

	while (end != std::wstring::npos)
	{
		std::wstring element = tempList.substr(start, end - start);

		if (!element.empty())
		{
			element = TrimString(element);
			vectorList.push_back(element.c_str());
		}
		start = end + 1;
		end = tempList.find(separtor, start);
	}

	if (start < tempList.length())
	{
		std::wstring element = tempList.substr(start, tempList.length() - start);

		if (!element.empty())
		{
			element = TrimString(element);
			vectorList.push_back(element.c_str());
		}
	}

	if (maxElement)
	{
		while (vectorList.size() < maxElement)
		{
			vectorList.push_back(L"0");
		}
	}

	return vectorList;
}

Magick::ColorRGB GetColor(LPCWSTR rawString)
{
	if (!*rawString)
		return Magick::ColorRGB(0, 0, 0, 0);

	std::vector<double> rgba;
	std::wstring tempList = rawString;
	size_t start = 0;
	size_t end = tempList.find(L",", 0);

	if (end == std::wstring::npos) //RRGGBBAA
	{
		if (tempList.length() == 6 || tempList.length() == 8)
		{
			while (start < 6)
			{
				rgba.push_back(std::stoul(tempList.substr(start, 2), 0, 16) / 255);
				start += 2;
			}
		}
		else
			return Magick::ColorRGB(0, 0, 0, 1);

	}
	else //R,G,B,A
	{
		for (auto &color : SeparateList(rawString, L",", NULL))
		{
			rgba.push_back(_wtof(color.c_str()) / 255);
		}

	}

	while (rgba.size() < 4)
	{
		rgba.push_back(255);
	}

	return Magick::ColorRGB(rgba[0], rgba[1], rgba[2], rgba[3]);
}

std::wstring error2pws(std::exception &error)
{
	wchar_t errorW[512];
	size_t * c = 0;
	mbstowcs_s(c, errorW, error.what(), 512 * sizeof(wchar_t));
	return errorW;
}

BOOL ParseEffect(void * rm, Magick::Image &img, std::wstring name, std::wstring para)
{
	LPCWSTR effect = name.c_str();
	LPCWSTR parameter = para.c_str();
	try
	{
		if (_wcsicmp(effect, L"MOVE") == 0) //Only NORMAL, REFERENCE, COMBINE
		{
			std::vector<std::wstring> offsetXY = SeparateList(parameter, L",", 2);
			size_t x = MathParser::ParseI(offsetXY[0]);
			size_t y = MathParser::ParseI(offsetXY[1]);
			Magick::Image tempImg = Magick::Image(
				Magick::Geometry(img.columns() + x, img.rows() + y),
				Magick::Color("transparent")
			);

			tempImg.composite(img, x, y, MagickCore::SrcCompositeOp);

			img = tempImg;
		}
		else if (_wcsicmp(effect, L"ADAPTIVEBLUR") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L",", 2);
			img.adaptiveBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"BLUR") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L",", 2);
			img.blur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"GAUSSIANBLUR") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L",", 2);
			img.gaussianBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"MOTIONBLUR") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L",", 3);
			img.motionBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1]),
				MathParser::ParseF(valList[2])
			);
		}
		else if (_wcsicmp(effect, L"NOISE") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L";", 2);
			img.attenuate(MathParser::ParseF(valList[0]));
			img.addNoise((Magick::NoiseType)MathParser::ParseI(valList[0]));
		}
		else if (_wcsicmp(effect, L"SHADOW") == 0)
		{
			std::vector<std::wstring> dropShadow = SeparateList(parameter, L";", 2, L"");
			std::vector<std::wstring> valList = SeparateList(dropShadow[0].c_str(), L",", 4);

			size_t offsetX = MathParser::ParseI(valList[2]);
			size_t offsetY = MathParser::ParseI(valList[3]);

			Magick::Image tempShadow = img;

			if (!dropShadow[1].empty())
				tempShadow.backgroundColor(GetColor(dropShadow[1].c_str()));
			else
				tempShadow.backgroundColor(Magick::Color("black"));

			tempShadow.shadow(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1]),
				0, 0
			);
			
			img.extent(Magick::Geometry(
				tempShadow.columns() + offsetX,
				tempShadow.rows() + offsetY
			), Magick::Color("transparent"));

			img.composite(
				tempShadow, 
				offsetX,
				offsetY,
				MagickCore::DstOverCompositeOp
			);
		}
		else if (_wcsicmp(effect, L"RESIZE") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L";", 2);

			img.resize(Magick::Geometry(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			));
		}
		else if (_wcsicmp(effect, L"ADAPTIVERESIZE") == 0)
		{
			std::vector<std::wstring> valList = SeparateList(parameter, L";", 2);

			img.adaptiveResize(Magick::Geometry(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			));
		}
		else if (_wcsicmp(effect, L"SCALE") == 0)
		{
			std::vector<std::wstring> scale = SeparateList(parameter, L";", 2);
			Magick::Geometry newSize;
			if (scale[0].find(L"%") != std::wstring::npos)
			{
				newSize.percent(true);
				size_t percent = MathParser::ParseI(scale[0]);
				newSize.width(percent);
				newSize.height(percent);
			}
			else
			{
				newSize.width(MathParser::ParseI(scale[0]));
				newSize.height(MathParser::ParseI(scale[1]));
			}

			img.scale(newSize);
		}
		else if (_wcsicmp(effect, L"CROP") == 0)
		{
			std::vector<std::wstring> geometryRaw = SeparateList(parameter, L",", 5);
			int x = MathParser::ParseI(geometryRaw[0]);
			int y = MathParser::ParseI(geometryRaw[1]);
			int w = MathParser::ParseI(geometryRaw[2]);
			int h = MathParser::ParseI(geometryRaw[3]);
			int g = MathParser::ParseI(geometryRaw[4]);

			if (w < 0 || h < 0)
			{
				RmLog(2, L"Crop: Invalid width or height value.");
				return TRUE;
			}
			else if (g > 5 || g < 0)
			{
				RmLog(2, L"Crop: Invalid Anchor value.");
				return TRUE;
			}

			//TOP LEFT anchor is default anchor.
			switch (g)
			{
			case 2: //TOP RIGHT
				x += img.columns();
				break;
			case 3: //BOTTOM RIGHT
				x += img.columns();
				y += img.rows();
				break;
			case 4: //BOTTOM LEFT
				y += img.rows();
				break;
			case 5: //CENTER
				x += (img.columns() / 2);
				y += (img.rows() / 2);
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
			if ((size_t)(w + x) > img.columns())
				w = img.columns() - x;

			if ((size_t)(h + y) > img.rows())
				h = img.rows() - y;

			img.crop(Magick::Geometry(w, h, x, y));
		}
		else if (_wcsicmp(effect, L"ROTATIONALBLUR") == 0)
		{
			double val = MathParser::ParseF(parameter);
			if (val)
				img.rotationalBlur(val);
		}
		else if (_wcsicmp(effect, L"IMPLODE") == 0)
		{
			double val = MathParser::ParseF(parameter);
			if (val)
				img.implode(val);
		}
		else if (_wcsicmp(effect, L"SPREAD") == 0)
		{
			double val = MathParser::ParseF(parameter);
			if (val)
				img.spread(val);
		}
		else if (_wcsicmp(effect, L"SWIRL") == 0)
		{
			double val = MathParser::ParseF(parameter);
			if (val)
				img.swirl(val);
		}
		else if (_wcsicmp(effect, L"EQUALIZE") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.equalize();
		}
		else if (_wcsicmp(effect, L"ENHANCE") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.enhance();
		}
		else if (_wcsicmp(effect, L"DESPECKLE") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.despeckle();
		}
		else if (_wcsicmp(effect, L"TRANSPOSE") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.transpose();
		}
		else if (_wcsicmp(effect, L"TRANSVERSE") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.transverse();
		}
		else if (_wcsicmp(effect, L"FLIP") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.flip();
		}
		else if (_wcsicmp(effect, L"FLOP") == 0)
		{
			if (MathParser::ParseI(parameter) == 1)
				img.flop();
		}
		return TRUE;
	}
	catch (std::exception &error_)
	{
		RmLogF(rm, 1, L"%s", error2pws(error_));
		return FALSE;
	}
}

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}