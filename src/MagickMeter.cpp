#include "MagickMeter.h"
#include "MagickCore\magick-type.h"
#include "..\..\API\RainmeterAPI.h"

ImgType GetType(std::wstring input);
void ComposeFinalImage(Measure * measure);
BOOL GetImage(Measure * measure, std::wstring imageName, BOOL isPush = TRUE);
BOOL CreateNew(ImgStruct &dst, WSVector &setting, Measure * measure);
BOOL CreateText(ImgStruct &dst, WSVector &setting, Measure * measure);
BOOL CreateShape(ImgStruct &dst, WSVector &setting, ImgType shape, Measure * measure);
BOOL CreateCombine(ImgStruct &dst, WSVector &setting, Measure * measure);
BOOL CreateGradient(ImgStruct &dst, WSVector &setting, Measure * measure);
BOOL ParseEffect(Measure * measure, ImgStruct &img, std::wstring name, std::wstring para);
std::wstring color2wstr(Magick::Color c);
std::vector<Magick::Color> GenColor(Magick::Image img, size_t totalColor);

ImgType GetType(std::wstring input)
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
	measure->rm = rm;
	std::wstring mesName = RmGetMeasureName(rm);
	measure->skin = RmGetSkin(rm);
	Magick::InitializeMagick("\\");

	std::wstring tempFolder = RmReplaceVariables(rm, L"%temp%");
	std::wstring disName = RmGetSkinName(rm);
	size_t slashPos = disName.find(L"\\", 0);
	while (slashPos != std::wstring::npos)
	{
		disName.replace(slashPos, 1, L"_");
		slashPos = disName.find(L"\\", slashPos);
	}
	measure->outputW = tempFolder + L"\\" + disName + L"_" + mesName + L".bmp";
	measure->outputA = ws2s(measure->outputW);
	measure->finalImg.alpha(true);
	measure->finalImg.backgroundColor(INVISIBLE);
}

PLUGIN_EXPORT void Reload(void * data, void * rm, double * maxValue)
{
	Measure* measure = (Measure*)data;

	std::wstring imgName = L"Image";
	int inpCount = 1;

	for (auto &img : measure->imgList)
	{
		img.contain.erase();
	}

	measure->imgList.clear();
	measure->imgList.shrink_to_fit();

	while (true)
	{
		if (!GetImage(measure, imgName))
			break;
		imgName = L"Image" + std::to_wstring(++inpCount);
	}
	
	ComposeFinalImage(measure);
}

BOOL GetImage(Measure * measure, std::wstring imageName, BOOL isPush)
{
	std::wstring rawSetting = RmReadString(measure->rm, imageName.c_str(), L"");
	if (rawSetting.empty()) return FALSE;

	WSVector imgVec = SeparateList(rawSetting, L"|", NULL);
	if (imgVec.size() == 0) return FALSE;

	ParseExtend(measure->rm, imgVec, imageName);

	ImgStruct curImg;

	if (curImg.contain.isValid()) curImg.contain.erase();

	curImg.contain = ONEPIXEL;
	curImg.contain.alpha(true);
	curImg.contain.backgroundColor(INVISIBLE);

	std::wstring typeName, typePara;
	GetNamePara(imgVec[0], typeName, typePara);

	ImgType type = GetType(typeName);

	curImg.index = NameToIndex(imageName);

	curImg.colorList.resize(5, INVISIBLE);

	BOOL isValid = TRUE;
	if (!typePara.empty())
	{
		imgVec[0] = typePara;
		ParseInternalVariable(measure, imgVec[0], curImg);
		switch (type)
		{
		case NORMAL:
			isValid = CreateNew(curImg, imgVec, measure);
			break;
		case COMBINE:
			isValid = CreateCombine(curImg, imgVec, measure);
			break;
		case TEXT:
			isValid = CreateText(curImg, imgVec, measure);
			break;
		case PATH:
		case RECTANGLE:
		case ELLIPSE:
		case POLYGON:
			isValid = CreateShape(curImg, imgVec, type, measure);
			break;
		case CLONE:
		{
			int index = NameToIndex(imgVec[0]);

			if (index < measure->imgList.size() && index >= 0)
			{
				int saveIndex = curImg.index;
				curImg = measure->imgList[index];
				//Reset to normal state.
				curImg.isIgnore = FALSE;
				curImg.isDelete = FALSE;
				curImg.index = saveIndex;
			}
			else
			{
				RmLogF(measure->rm, 2, L"%s is invalid Image to clone.", imgVec[0].c_str());
				isValid = FALSE;
				break;
			}

			imgVec.erase(imgVec.begin());
			isValid = TRUE;
			break;
		}
		case GRADIENT:
			isValid = CreateGradient(curImg, imgVec, measure);
			break;
		}
	}

	if (isValid && curImg.contain.isValid())
	{
		curImg.colorList = GenColor(curImg.contain, 5);

		if (type == COMBINE || type == CLONE)
		{
			Magick::Geometry newBound;
			try
			{
				newBound = curImg.contain.boundingBox();
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

		for (auto &settingIt : imgVec)
		{
			std::wstring name, parameter;
			GetNamePara(settingIt, name, parameter);
			ParseInternalVariable(measure, parameter, curImg);

			if (!ParseEffect(measure, curImg, name, parameter))
				return FALSE;
		}

		if (isPush)
			measure->imgList.push_back(curImg);
		else
			measure->imgList[curImg.index] = curImg;
	}
	else
	{
		RmLogF(measure->rm, 2, L"%s is invalid. Check its modifiers and effects again.", imageName.c_str());
		return FALSE;
	}

	return TRUE;
}

void ComposeFinalImage(Measure * measure)
{
	if (measure->finalImg.isValid())
		measure->finalImg.erase();

	measure->finalImg = ONEPIXEL;
	measure->finalImg.alpha(true);
	measure->finalImg.backgroundColor(INVISIBLE);

	size_t newW = 0;
	size_t newH = 0;
	for (int i = 0; i < measure->imgList.size(); i++)
	{
		if (measure->imgList[i].isDelete || measure->imgList[i].isIgnore) continue;

		//Extend main image size
		if (measure->imgList[i].contain.columns() > newW)
			newW = measure->imgList[i].contain.columns();

		if (measure->imgList[i].contain.rows() > newH)
			newH = measure->imgList[i].contain.rows();
	}

	Magick::Geometry newSize(newW, newH);
	newSize.aspect(true);
	measure->finalImg.scale(newSize);

	for (int i = 0; i < measure->imgList.size(); i++)
	{
		if (measure->imgList[i].isDelete || measure->imgList[i].isIgnore) continue;
		measure->finalImg.composite(measure->imgList[i].contain, 0, 0, MagickCore::OverCompositeOp);
	}

	if (measure->finalImg.isValid())
	{
		GenColor(measure->finalImg, 3);
		measure->finalImg.write(measure->outputA);
		std::wstring exportOption = RmReadString(measure->rm, L"ExportTo", L"");
		if (!exportOption.empty())
		{
			try
			{
				measure->finalImg.write(ws2s(exportOption));
			}
			catch (Magick::Exception &error_)
			{
				error2pws(error_);
			}
		}
	}
	else
	{
		Magick::Image fallback = ONEPIXEL;
		fallback.write(measure->outputA);
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	if (measure->finalImg == ONEPIXEL)
	{
		return -1;
	}
	return 1;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;

	return measure->outputW.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	if (_wcsicmp(args, L"Update") == 0)
		Reload(data, measure->rm, 0);
	else if (_wcsnicmp(args, L"Reload", 6) == 0)
	{
		std::wstring list = args;
		list = list.substr(list.find_first_not_of(L" \t\r\n", 6));
		WSVector imageList = SeparateList(list, L",", NULL);
		for (auto &image : imageList)
		{
			int index = NameToIndex(image);
			if (index >= 0 && index < measure->imgList.size())
			{
				measure->imgList[index].contain.erase();
				GetImage(measure, image, FALSE);
			}
		}
		ComposeFinalImage(measure);
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	if (std::experimental::filesystem::exists(measure->outputW))
		DeleteFile(measure->outputW.c_str());
	//Magick::TerminateMagick();
	delete measure;
}


void GetNamePara(std::wstring input, std::wstring& name, std::wstring& para)
{
	size_t firstSpace = input.find_first_of(L" \t\r\n");
	if (firstSpace == std::wstring::npos)
	{
		if (!input.empty())
			name = input;
		else
			name = L"";
		para = L"";
		return;
	}

	name = input.substr(0, firstSpace);
	para = input.substr(input.find_first_not_of(L" \t\r\n", firstSpace + 1));
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
	if (start != std::wstring::npos)
	{
		std::wstring trimmedString = bloatedString.substr(start, bloatedString.length());
		trimmedString.erase(trimmedString.find_last_not_of(L" \t\r\n") + 1);
		return trimmedString;
	}
	return L"";
}

WSVector SeparateList(std::wstring rawString, wchar_t* separator, int maxElement, wchar_t* defValue)
{
	WSVector vectorList;

	if (rawString.empty())
		return vectorList;

	size_t start = 0;
	size_t end = rawString.find(separator);

	while (end != std::wstring::npos)
	{
		std::wstring element = rawString.substr(start, end - start);
		element = TrimString(element);

		if (!element.empty())
		{
			vectorList.push_back(element);
		}
		start = end + 1;
		end = rawString.find(separator, start);
	}

	if (start < rawString.length())
	{
		std::wstring element = rawString.substr(start, rawString.length() - start);
		element = TrimString(element);

		if (!element.empty())
		{
			vectorList.push_back(element);
		}
	}

	if (maxElement)
	{
		while (vectorList.size() < maxElement)
		{
			vectorList.push_back(defValue);
		}
	}

	return vectorList;
}

WSVector SeparateParameter(std::wstring rawPara, int maxPara, std::wstring defValue)
{
	WSVector vectorList;
	size_t s = 0;
	size_t e = rawPara.find(L",");

	while (e != std::wstring::npos)
	{
		std::wstring t = rawPara.substr(s, e - s);
		LPCWSTR tP = t.c_str();
		int b = 0;
		while (*tP)
		{
			if (*tP == L'(')
				++b;
			else if (*tP == L')')
				--b;
			++tP;
		}
		if (b != 0)
			e = rawPara.find(L",", e + 1);
		else
		{
			vectorList.push_back(t);
			s = e + 1;
			e = rawPara.find(L",", s);
		}
	}
	if (s != rawPara.length())
		vectorList.push_back(rawPara.substr(s, rawPara.length() - s));

	while (vectorList.size() < maxPara)
		vectorList.push_back(defValue);

	return vectorList;
}

Magick::Color GetColor(std::wstring rawString)
{
	if (rawString.empty())
		return Magick::Color("black");

	Magick::Color result;
	if (_wcsnicmp(rawString.c_str(), L"HSL(", 4) == 0)
	{
		size_t end = rawString.find_last_of(L")");

		if (end != std::wstring::npos)
		{
			rawString = rawString.substr(4, end - 4);
			WSVector hsl = SeparateList(rawString, L",", 3, L"0");
			Magick::ColorRGB temp = Magick::ColorHSL(
				MathParser::ParseF(hsl[0]) / 360,
				MathParser::ParseF(hsl[1]) / 100,
				MathParser::ParseF(hsl[2]) / 100
			);

			if (hsl.size() > 3 && !hsl[3].empty())
				temp.alpha(MathParser::ParseF(hsl[3]) / 100.0);

			if (temp.isValid()) result = temp;
		}
	}
	else
	{
		std::vector<double> rgba;
		size_t start = 0;
		size_t end = rawString.find(L",");

		if (end == std::wstring::npos) //RRGGBBAA
		{
			if (rawString.find_first_not_of(L"0123456789ABCDEFabcdef") == std::wstring::npos) //Find non hex character
			{
				size_t remain = rawString.length();
				while (remain >= 2)
				{
					int val = std::stoi(rawString.substr(start, 2), 0, 16);
					rgba.push_back((double)val / 255);
					start += 2;
					remain -= 2;
				}
			}
		}
		else //R,G,B,A
		{
			for (auto &color : SeparateList(rawString, L",", NULL))
			{
				rgba.push_back(MathParser::ParseF(color) / 255);
			}
		}
		if (rgba.size() == 3)
			result = Magick::ColorRGB(rgba[0], rgba[1], rgba[2]);
		else if (rgba.size() > 3)
			result = Magick::ColorRGB(rgba[0], rgba[1], rgba[2], rgba[3]);
	}

	if (result.isValid())
		return result;
	else
		return Magick::Color("black");
}

void error2pws(Magick::Exception error)
{
	wchar_t errorW[512];
	size_t * c = 0;
	mbstowcs_s(c, errorW, error.what(), 512 * sizeof(wchar_t));
	RmLog(2, errorW);
}

void ParseExtend(void * rm, WSVector &parentVector, std::wstring parentName, BOOL isRecursion)
{
	for (int k = (int)parentVector.size() - 1 ; k >=  1 - isRecursion; k--) // Avoid first iteration
	{
		int insertIndex = k + 1;
		if (_wcsnicmp(parentVector[k].c_str(), L"EXTEND", 6) == 0)
		{
			WSVector extendList = SeparateList(parentVector[k].substr(6), L",", NULL);
			for (int i = 0; i < extendList.size(); i++)
			{
				if (_wcsicmp(parentName.c_str(), extendList[i].c_str()) == 0) continue; //Avoid Extend itself

				WSVector childVector = SeparateList(
					RmReadString(rm, extendList[i].c_str(), L""),
					L"|", NULL
				);

				ParseExtend(rm, childVector, extendList[i], TRUE);

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

auto ParseNumber2 = [](const WCHAR* value, auto defVal, auto* func) -> decltype(defVal)
{
	if (_wcsnicmp(value, L"*", 1) == 0) return defVal;
	return (decltype(defVal))func(value);
};

BOOL ParseEffect(Measure * measure, ImgStruct &image, std::wstring name, std::wstring para)
{
	LPCWSTR effect = name.c_str();
	LPCWSTR parameter = para.c_str();
	try
	{
		if (_wcsicmp(effect, L"MOVE") == 0)
		{
			WSVector offsetXY = SeparateParameter(parameter, 2);
			ssize_t x = MathParser::ParseI(offsetXY[0]);
			ssize_t y = MathParser::ParseI(offsetXY[1]);
			Magick::Image tempImg = Magick::Image(
				Magick::Geometry(image.contain.columns() + x, image.contain.rows() + y),
				INVISIBLE
			);

			tempImg.composite(image.contain, x, y, MagickCore::OverCompositeOp);

			image.contain = tempImg;

			image.X += x;
			image.Y += y;
		}
		else if (_wcsicmp(effect, L"ADAPTIVEBLUR") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			image.contain.adaptiveBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"BLUR") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			image.contain.blur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"GAUSSIANBLUR") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			image.contain.gaussianBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"MOTIONBLUR") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 3);
			image.contain.motionBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1]),
				MathParser::ParseF(valList[2])
			);
		}
		else if (_wcsicmp(effect, L"NOISE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2, L"");
			double density;
			if (valList[1].empty())
				density = 1.0;
			else
				density = MathParser::ParseF(valList[1]);

			image.contain.image() = MagickCore::AddNoiseImage(
				image.contain.image(),
				(Magick::NoiseType)MathParser::ParseI(valList[0]),
				density,
				nullptr
			);
		}
		else if (_wcsicmp(effect, L"SHADOW") == 0)
		{
			WSVector dropShadow = SeparateList(parameter, L";", 2, L"");
			WSVector valList = SeparateParameter(dropShadow[0], 5);

			double sigma = MathParser::ParseF(valList[1]);
			int offsetX = MathParser::ParseI(valList[2]);
			int offsetY = MathParser::ParseI(valList[3]);

			Magick::Image tempShadow = image.contain;

			if (!dropShadow[1].empty())
				tempShadow.backgroundColor(GetColor(dropShadow[1].c_str()));
			else
				tempShadow.backgroundColor(Magick::Color("black"));

			if (sigma < 0) sigma = -sigma;
			tempShadow.shadow(
				MathParser::ParseF(valList[0]),
				sigma,
				0, 0
			);

			image.contain.extent(Magick::Geometry(
				(int)tempShadow.columns() + offsetX + (int)sigma,
				(int)tempShadow.rows() + offsetY + (int)sigma
			), INVISIBLE);

			if (MathParser::ParseI(valList[4]) == 1)
			{
				image.contain.erase();
				image.contain.composite(
					tempShadow,
					(ssize_t)(offsetX - sigma * 2),
					(ssize_t)(offsetY - sigma * 2),
					MagickCore::OverCompositeOp
				);
			}
			else
				image.contain.composite(
					tempShadow,
					(ssize_t)(offsetX - sigma * 2),
					(ssize_t)(offsetY - sigma * 2),
					MagickCore::DstOverCompositeOp
				);

		}
		else if (_wcsicmp(effect, L"INNERSHADOW") == 0)
		{
			//TODO: Cripsy outline. fix it.
			WSVector dropShadow = SeparateList(parameter, L";", 2, L"");
			WSVector valList = SeparateParameter(dropShadow[0], 5);
			double sigma = MathParser::ParseF(valList[1]);
			int offsetX = MathParser::ParseI(valList[2]);
			int offsetY = MathParser::ParseI(valList[3]);
			Magick::Image temp(Magick::Geometry(
				image.contain.columns() + offsetX + (int)sigma * 4,
				image.contain.rows() + offsetY + (int)sigma * 4
			), Magick::Color("black"));
			temp.alpha(true);
			temp.composite(image.contain, (size_t)sigma * 2, (size_t)sigma * 2, Magick::DstOutCompositeOp);
			temp.backgroundColor(GetColor(dropShadow[1]));
			temp.shadow(
				MathParser::ParseF(valList[0]),
				sigma,
				0, 0
			);
			
			Magick::Image shadowOnly = image.contain;
			shadowOnly.composite(temp, (ssize_t)(offsetX - sigma * 4), (ssize_t)(offsetY - sigma * 4), Magick::SrcInCompositeOp);
			if (MathParser::ParseI(valList[4]) == 1)
				image.contain = shadowOnly;
			else
				image.contain.composite(shadowOnly, 0, 0, Magick::OverCompositeOp);
		}
		else if (_wcsicmp(effect, L"DISTORT") == 0)
		{
			WSVector valList = SeparateList(parameter, L";", 3);
			WSVector rawList = SeparateParameter(valList[1], NULL);

			std::vector<double> doubleList;
			for (auto &oneDouble : rawList)
			{
				doubleList.push_back(MathParser::ParseF(oneDouble));
			}

			double * doubleArray = &doubleList[0];
			image.contain.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
			image.contain.distort(
				(MagickCore::DistortMethod)MathParser::ParseI(valList[0]),
				doubleList.size(),
				doubleArray,
				MathParser::ParseB(valList[2])
			);
		}
		//Same as Distort Perspective but this only requires position of 4 final points
		else if (_wcsicmp(effect, L"PERSPECTIVE") == 0)
		{
			WSVector rawList = SeparateParameter(parameter, NULL);

			if (rawList.size() < 8)
			{
				RmLog(2, L"Perspective: Not enough control point. Requires 4 pairs of X and Y.");
				return TRUE;
			}
			double doubleArray[16];
			//Top Left
			doubleArray[0]  = (double)image.X;
			doubleArray[1]  = (double)image.Y;
			doubleArray[2]  = ParseNumber2(rawList[0].c_str(), doubleArray[0], MathParser::ParseF);
			doubleArray[3]  = ParseNumber2(rawList[1].c_str(), doubleArray[1], MathParser::ParseF);
			//Top Right
			doubleArray[4]  = (double)(image.X + image.W);
			doubleArray[5]  = (double)image.Y;
			doubleArray[6]  = ParseNumber2(rawList[2].c_str(), doubleArray[4], MathParser::ParseF);
			doubleArray[7]  = ParseNumber2(rawList[3].c_str(), doubleArray[5], MathParser::ParseF);
			//Bottom Right
			doubleArray[8]  = (double)(image.X + image.W);
			doubleArray[9]  = (double)(image.Y + image.H);
			doubleArray[10] = ParseNumber2(rawList[4].c_str(), doubleArray[8], MathParser::ParseF);
			doubleArray[11] = ParseNumber2(rawList[5].c_str(), doubleArray[9], MathParser::ParseF);
			//Bottom Left
			doubleArray[12] = (double)image.X;
			doubleArray[13] = (double)(image.Y + image.H);
			doubleArray[14] = ParseNumber2(rawList[6].c_str(), doubleArray[12], MathParser::ParseF);
			doubleArray[15] = ParseNumber2(rawList[7].c_str(), doubleArray[13], MathParser::ParseF);

			image.contain.virtualPixelMethod(Magick::TransparentVirtualPixelMethod);
			image.contain.distort(
				MagickCore::PerspectiveDistortion,
				16,
				doubleArray,
				true
			);
		}
		else if (_wcsicmp(effect, L"OPACITY") == 0)
		{
			double a = MathParser::ParseF(parameter);
			if (a > 100) a = 100;
			if (a < 0) a = 0;
			a /= 100;
			for (int i = 0; i < image.contain.columns(); i++)
			{
				for (int j = 0; j < image.contain.rows(); j++)
				{
					Magick::ColorRGB c = image.contain.pixelColor(i, j);
					c.alpha(c.alpha() * a);
					image.contain.pixelColor(i, j, c);
				}
			}
		}
		else if (_wcsicmp(effect, L"SHADE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 3);

			image.contain.shade(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1]),
				MathParser::ParseI(valList[2]) == 1
			);
		}
		else if (_wcsicmp(effect, L"CHANNEL") == 0)
		{
			RmLogF(measure->rm, 2, L"%d", image.contain.channels());
			if (_wcsicmp(parameter, L"RED") == 0)
			{
				image.contain = image.contain.separate(Magick::RedChannel);
			}
			else if (_wcsicmp(parameter, L"BLUE") == 0)
			{
				image.contain = image.contain.separate(Magick::BlueChannel);
			}
		}
		else if (_wcsicmp(effect, L"MODULATE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 3, L"100");
			image.contain.modulate(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1]),
				MathParser::ParseI(valList[2])
			);
		}
		else if (_wcsicmp(effect, L"OILPAINT") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			image.contain.oilPaint(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"SHEAR") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			image.contain.shear(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"RESIZE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 3);
			Magick::Geometry newSize(
				MathParser::ParseI(valList[0]), 
				MathParser::ParseI(valList[1])
			);

			switch (MathParser::ParseI(valList[2]))
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

			image.contain.resize(newSize);
		}
		else if (_wcsicmp(effect, L"ADAPTIVERESIZE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 3);
			Magick::Geometry newSize(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			);

			switch (MathParser::ParseI(valList[2]))
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

			image.contain.adaptiveResize(newSize);
		}
		else if (_wcsicmp(effect, L"SCALE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			Magick::Geometry newSize;
			size_t percentPos = valList[0].find(L"%");
			if (percentPos != std::wstring::npos)
			{
				newSize.percent(true);
				size_t percent = MathParser::ParseI(valList[0].substr(0, percentPos));
				newSize.width(percent);
				newSize.height(percent);
			}
			else
			{
				newSize.width(MathParser::ParseI(valList[0]));
				newSize.height(MathParser::ParseI(valList[1]));

				switch (MathParser::ParseI(valList[2]))
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
			}

			image.contain.scale(newSize);
		}
		else if (_wcsicmp(effect, L"SAMPLE") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2);
			Magick::Geometry newSize(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			);

			switch (MathParser::ParseI(valList[2]))
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

			image.contain.sample(newSize);
		}
		else if (_wcsicmp(effect, L"RESAMPLE") == 0)
		{
			image.contain.resample(MathParser::ParseF(parameter));
		}
		else if (_wcsicmp(effect, L"CROP") == 0)
		{
			WSVector geometryRaw = SeparateParameter(parameter, 5);
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
				RmLog(2, L"Crop: Invalid Origin value. Left Top origin is used.");
				return TRUE;
			}

			//TOP LEFT anchor is default anchor.
			switch (g)
			{
			case 2: //TOP RIGHT
				x += (int)image.contain.columns();
				break;
			case 3: //BOTTOM RIGHT
				x += (int)image.contain.columns();
				y += (int)image.contain.rows();
				break;
			case 4: //BOTTOM LEFT
				y += (int)image.contain.rows();
				break;
			case 5: //CENTER
				x += (int)(image.contain.columns() / 2);
				y += (int)(image.contain.rows() / 2);
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
			if ((size_t)(w + x) > image.contain.columns())
				w = (int)image.contain.columns() - x;

			if ((size_t)(h + y) > image.contain.rows())
				h = (int)image.contain.rows() - y;

			image.contain.crop(Magick::Geometry(w, h, x, y));
		}
		else if (_wcsicmp(effect, L"ROTATIONALBLUR") == 0)
		{
			image.contain.rotationalBlur(MathParser::ParseF(parameter));
		}
		else if (_wcsicmp(effect, L"COLORIZE") == 0)
		{
			WSVector valList = SeparateList(parameter, L";", 2);
			image.contain.colorize(
				(unsigned int)MathParser::ParseI(valList[0]),
				GetColor(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"GRAYSCALE") == 0)
		{
			int method = MathParser::ParseI(parameter);
			if (method > 0 && method <= 9)
				image.contain.grayscale((Magick::PixelIntensityMethod)method);
		}
		else if (_wcsicmp(effect, L"ROLL") == 0)
		{
			WSVector valList = SeparateParameter(parameter, 2, L"0");
			int c = MathParser::ParseI(valList[0]);
			int r = MathParser::ParseI(valList[1]);
			if (c < 0)
				c += (int)image.contain.columns();
			if (r < 0)
				r += (int)image.contain.rows();

			image.contain.roll((size_t)c, (size_t)r);
		}
		else if (_wcsicmp(effect, L"NEGATE") == 0)
		{
			image.contain.negate(MathParser::ParseB(parameter));
		}
		else if (_wcsicmp(effect, L"IMPLODE") == 0)
		{
			image.contain.implode(MathParser::ParseF(parameter));
		}
		else if (_wcsicmp(effect, L"SPREAD") == 0)
		{
			image.contain.virtualPixelMethod(Magick::BackgroundVirtualPixelMethod);
			image.contain.spread(MathParser::ParseF(parameter));
		}
		else if (_wcsicmp(effect, L"SWIRL") == 0)
		{
			image.contain.swirl(MathParser::ParseF(parameter));
		}
		else if (_wcsicmp(effect, L"MEDIANFILTER") == 0)
		{
			image.contain.medianFilter(MathParser::ParseF(parameter));
		}
		else if (_wcsicmp(effect, L"EQUALIZE") == 0)
		{
			image.contain.equalize();
		}
		else if (_wcsicmp(effect, L"ENHANCE") == 0)
		{
			image.contain.enhance();
		}
		else if (_wcsicmp(effect, L"DESPECKLE") == 0)
		{
			image.contain.despeckle();
		}
		else if (_wcsicmp(effect, L"REDUCENOISE") == 0)
		{
			image.contain.reduceNoise();
		}
		else if (_wcsicmp(effect, L"TRANSPOSE") == 0)
		{
			image.contain.transpose();
		}
		else if (_wcsicmp(effect, L"TRANSVERSE") == 0)
		{
			image.contain.transverse();
		}
		else if (_wcsicmp(effect, L"FLIP") == 0)
		{
			image.contain.flip();
		}
		else if (_wcsicmp(effect, L"FLOP") == 0)
		{
			image.contain.flop();
		}
		else if (_wcsicmp(effect, L"MAGNIFY") == 0)
		{
			image.contain.magnify();
		}
		else if (_wcsicmp(effect, L"MINIFY") == 0)
		{
			image.contain.minify();
		}
		else if (_wcsicmp(effect, L"IGNORE") == 0)
		{
			image.isIgnore = MathParser::ParseB(parameter);
		}
		effect = parameter = nullptr;
		return TRUE;
	}
	catch (Magick::Exception &error_)
	{
		error2pws(error_);
		return FALSE;
	}
}

void ParseInternalVariable(Measure * measure, std::wstring &rawSetting, ImgStruct &srcImg)
{
	size_t start = rawSetting.find(L"{");
	while (start != std::wstring::npos)
	{
		size_t end = rawSetting.find(L"}", start + 1);
		if (end != std::wstring::npos)
		{
			std::wstring f = rawSetting.substr(start + 1, end - start - 1);

			WSVector v = SeparateList(f, L":", 2, L"");
			int index = NameToIndex(v[0]);
			if (index != -1 && index <= srcImg.index)
			{
				ImgStruct tempStruct;
				if (index == srcImg.index)
					tempStruct = srcImg;
				else
					tempStruct = measure->imgList[index];
				LPCWSTR para = v[1].c_str();
				std::wstring c = L"";
				if (_wcsnicmp(para, L"X", 1) == 0)
				{
					c = std::to_wstring(tempStruct.X);
				}
				else if (_wcsnicmp(para, L"Y", 1) == 0)
				{
					c = std::to_wstring(tempStruct.Y);
				}
				else if (_wcsnicmp(para, L"W", 1) == 0)
				{
					c = std::to_wstring(tempStruct.W);
				}
				else if (_wcsnicmp(para, L"H", 1) == 0)
				{
					c = std::to_wstring(tempStruct.H);
				}
				else if (_wcsicmp(para, L"COLORBG") == 0)
				{
					c = color2wstr(tempStruct.colorList[0]);
				}
				else if (_wcsicmp(para, L"COLOR1") == 0)
				{
					c = color2wstr(tempStruct.colorList[1]);
				}
				else if (_wcsicmp(para, L"COLOR2") == 0)
				{
					c = color2wstr(tempStruct.colorList[2]);
				}
				else if (_wcsicmp(para, L"COLOR3") == 0)
				{
					c = color2wstr(tempStruct.colorList[3]);
				}
				else if (_wcsicmp(para, L"COLORFG") == 0)
				{
					c = color2wstr(tempStruct.colorList[4]);
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
					start = rawSetting.find(L"{", end + 1);

				continue;
			}
			else
			{
				start = rawSetting.find(L"{", end + 1);
				continue;
			}
		}
		else
			break;
	}
}
std::vector<Magick::Color> GenColor(Magick::Image img, size_t totalColor)
{
	Magick::Geometry smallSize(25, 25);
	smallSize.percent(true);
	img.resize(smallSize);
	img.quantizeColors(totalColor);
	img.quantize();
	Magick::Image uniqueColor = img.uniqueColors();
	std::vector<Magick::Color> colorList;

	for (int i = 1; i <= uniqueColor.columns(); i++)
		colorList.push_back(uniqueColor.pixelColor(i, 1));

	//Fill missing color. with last generated color.
	while (colorList.size() < totalColor)
		colorList.push_back(colorList[colorList.size() - 1]);

	return colorList;
}
std::wstring color2wstr(Magick::Color c)
{
	Magick::ColorRGB rgb = c;
	std::wstring s;
	s += std::to_wstring((int)round(rgb.red() * 255)) + L",";
	s += std::to_wstring((int)round(rgb.green() * 255)) + L",";
	s += std::to_wstring((int)round(rgb.blue() * 255));
	return s;
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