#include "MagickMeter.h"
#include "..\..\API\RainmeterAPI.h"

ImgType GetType(std::wstring input);
BOOL CreateNew(ImgStruct * dst, WSVector setting, Measure * measure);
BOOL CreateText(ImgStruct * dst, WSVector setting, Measure * measure);
BOOL CreateShape(ImgStruct * dst, WSVector setting, ImgType shape, Measure * measure);
BOOL CreateCombine(ImgStruct * dst, WSVector setting, Measure * measure);
BOOL CreateGradient(ImgStruct * dst, WSVector setting, Measure * measure);

ImgType GetType(std::wstring input)
{
	LPCWSTR inPtr = input.c_str();

	if (_wcsnicmp(inPtr, L"FILE", 4) == 0)				return NORMAL;
	else if (_wcsnicmp(inPtr, L"ELLIPSE", 7) == 0)		return ELLIPSE;
	else if (_wcsnicmp(inPtr, L"RECTANGLE", 9) == 0)	return RECTANGLE;
	else if (_wcsnicmp(inPtr, L"COMBINE", 7) == 0)		return COMBINE;
	else if (_wcsnicmp(inPtr, L"CLONE", 5) == 0)		return CLONE;
	else if (_wcsnicmp(inPtr, L"TEXT", 4) == 0)			return TEXT;
	else if (_wcsnicmp(inPtr, L"GRADIENT", 8) == 0)		return GRADIENT;
	else												return NOTYPE;
}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
	measure->rm = rm;
	measure->name = RmGetMeasureName(rm);
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
	measure->outputFile = tempFolder + L"\\" + disName + L"_" + measure->name + L".bmp";
	measure->finalImg.alpha(true);
	measure->finalImg.backgroundColor(INVISIBLE);
}

PLUGIN_EXPORT void Reload(void * data, void * rm, double * maxValue)
{
	Measure* measure = (Measure*)data;

	std::wstring imgName = L"Image";
	int inpCount = 1;
	std::wstring imgInp = RmReadString(rm, L"Image", L"");

	if (measure->finalImg.isValid())
	{
		measure->finalImg.erase();
	}

	measure->imgList.clear();
	measure->imgList.shrink_to_fit();

	while (!imgInp.empty())
	{
		WSVector imgVec = SeparateList(imgInp, L"|", NULL);
		if (imgVec.size() == 0) break;

		ParseExtend(rm, imgVec, imgName);

		ImgStruct * curImg = new ImgStruct;

		curImg->contain.alpha(true);
		curImg->contain.backgroundColor(INVISIBLE);

		std::wstring name, parameter;
		GetNamePara(imgVec[0], name, parameter);

		ImgType type = GetType(name);

		BOOL isValid = TRUE;

		if (!parameter.empty())
		{
			imgVec[0] = parameter;
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
			case RECTANGLE:
			case ELLIPSE:
				isValid = CreateShape(curImg, imgVec, type, measure);
				break;
			case CLONE:
			{
				int index = NameToIndex(imgVec[0]);

				if (index < measure->imgList.size() && index >= 0)
					curImg->contain = measure->imgList[index]->contain;
				else
				{
					RmLogF(rm, 2, L"%s is invalid Image to clone.", imgVec[0].c_str());
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
						RmLogF(rm, 2, L"Effect %s %s is invalid.", name.c_str(), parameter.c_str());
						isValid = FALSE;
						break;
					}
				}
				break;
			}
			case GRADIENT:
				isValid = CreateGradient(curImg, imgVec, measure);
				break;
			}
		}

		if (isValid && curImg->contain.isValid())
			measure->imgList.push_back(curImg);
		else
		{
			RmLogF(rm, 2, L"%s is invalid. Check its modifiers and effects again.", imgName.c_str());
			break;
		}
		
		imgName = L"Image" + std::to_wstring(++inpCount);
		imgInp = RmReadString(rm, imgName.c_str(), L"");
	}

	for (int i = 0; i < measure->imgList.size(); i++)
	{
		if (measure->imgList[i]->isDelete) continue;

		//Extend main image size
		size_t newW = measure->finalImg.columns();
		if (measure->imgList[i]->contain.columns() > newW)
			newW = measure->imgList[i]->contain.columns();

		size_t newH = measure->finalImg.rows();
		if (measure->imgList[i]->contain.rows() > newH)
			newH = measure->imgList[i]->contain.rows();

		measure->finalImg.size(Magick::Geometry(newW, newH));

		measure->finalImg.composite(measure->imgList[i]->contain, 0, 0, MagickCore::OverCompositeOp);
	}

	if (measure->finalImg.isValid()) measure->finalImg.write(ws2s(measure->outputFile));
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
	if (measure->finalImg.isValid())
		return measure->outputFile.c_str();
	else
		return nullptr;
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
	if (start != std::wstring::npos)
	{
		std::wstring trimmedString = bloatedString.substr(start, bloatedString.length());
		trimmedString.erase(trimmedString.find_last_not_of(L" \t\r\n") + 1);
		return trimmedString;
	}
	return L""; 
}

WSVector SeparateList(std::wstring rawString, wchar_t* separtor, int maxElement, wchar_t* defValue)
{
	WSVector vectorList;

	if (rawString.empty())
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
			vectorList.push_back(element);
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
			WSVector hsl = SeparateList(rawString, L",", NULL);
			if (hsl.size() >= 3)
			{
				result = Magick::ColorHSL(
					MathParser::ParseF(hsl[0]) / 360,
					MathParser::ParseF(hsl[1]) / 100,
					MathParser::ParseF(hsl[2]) / 100
				);

				//TODO: Add alpha parameter for HSL
				/*if (!hsl[3].empty())
				{
					int alphaValue = round(MathParser::ParseF(hsl[4]) / 100 * (Magick::Quantum)QuantumRange);
					result.quantumAlpha = alphaValue;
				}*/
			}

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
				while (start < 6)
				{
					rgba.push_back(std::stoul(rawString.substr(start, 2), 0, 16) / 255);
					start += 2;
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
		else if (rgba.size() == 4)
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
	for (int k = parentVector.size() - 1 ; k >=  1 - isRecursion; k--) // Avoid first iteration
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

BOOL ParseEffect(void * rm, Magick::Image &img, std::wstring name, std::wstring para)
{
	LPCWSTR effect = name.c_str();
	LPCWSTR parameter = para.c_str();
	try
	{
		if (_wcsicmp(effect, L"MOVE") == 0) //Only NORMAL, REFERENCE, COMBINE
		{
			WSVector offsetXY = SeparateList(parameter, L",", 2);
			size_t x = MathParser::ParseI(offsetXY[0]);
			size_t y = MathParser::ParseI(offsetXY[1]);
			Magick::Image tempImg = Magick::Image(
				Magick::Geometry(img.columns() + x, img.rows() + y),
				INVISIBLE
			);

			tempImg.composite(img, x, y, MagickCore::OverCompositeOp);

			img = tempImg;
		}
		else if (_wcsicmp(effect, L"ADAPTIVEBLUR") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			img.adaptiveBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"BLUR") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			img.blur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"GAUSSIANBLUR") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			img.gaussianBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"MOTIONBLUR") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 3);
			img.motionBlur(
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1]),
				MathParser::ParseF(valList[2])
			);
		}
		else if (_wcsicmp(effect, L"NOISE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			img.attenuate(MathParser::ParseF(valList[0]));
			img.addNoise((Magick::NoiseType)MathParser::ParseI(valList[0]));
		}
		else if (_wcsicmp(effect, L"SHADOW") == 0)
		{
			WSVector dropShadow = SeparateList(parameter, L";", 2, L"");
			WSVector valList = SeparateList(dropShadow[0], L",", 5);

			double sigma = MathParser::ParseF(valList[1]);
			int offsetX = MathParser::ParseI(valList[2]);
			int offsetY = MathParser::ParseI(valList[3]);

			Magick::Image tempShadow = img;

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

			img.extent(Magick::Geometry(
				tempShadow.columns() + offsetX + sigma,
				tempShadow.rows() + offsetY + sigma
			), INVISIBLE);
			
			if (MathParser::ParseI(valList[4]) == 1)
			{
				img.erase();
				img.composite(
					tempShadow,
					offsetX - sigma * 2,
					offsetY - sigma * 2,
					MagickCore::CopyCompositeOp
				);
			}
			else
				img.composite(
					tempShadow,
					offsetX - sigma * 2,
					offsetY - sigma * 2,
					MagickCore::DstOverCompositeOp
				);
				
		}
		else if (_wcsicmp(effect, L"PERSPECTIVE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", NULL);
			if (valList.size() < 16)
			{
				RmLog(2, L"Perspective: Not enough parameter. Require 16 parameters as 4 control points.");
				return FALSE;
			}
			
			const double argsPer[16] = {
				MathParser::ParseF(valList[0]),
				MathParser::ParseF(valList[1]),
				MathParser::ParseF(valList[2]),
				MathParser::ParseF(valList[3]),
				MathParser::ParseF(valList[4]),
				MathParser::ParseF(valList[5]),
				MathParser::ParseF(valList[6]),
				MathParser::ParseF(valList[7]),
				MathParser::ParseF(valList[8]),
				MathParser::ParseF(valList[9]),
				MathParser::ParseF(valList[10]),
				MathParser::ParseF(valList[11]),
				MathParser::ParseF(valList[12]),
				MathParser::ParseF(valList[13]),
				MathParser::ParseF(valList[14]),
				MathParser::ParseF(valList[15])
			};
			img.distort(
				MagickCore::PerspectiveDistortion,
				16,
				argsPer,
				false
			);
		}
		else if (_wcsicmp(effect, L"SHADE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 3);

			img.shade(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1]),
				MathParser::ParseI(valList[2]) == 1
			);
		}
		else if (_wcsicmp(effect, L"OILPAINT") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);
			img.oilPaint(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			);
		}
		else if (_wcsicmp(effect, L"RESIZE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);

			img.resize(Magick::Geometry(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			));
		}
		else if (_wcsicmp(effect, L"ADAPTIVERESIZE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);

			img.adaptiveResize(Magick::Geometry(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			));
		}
		else if (_wcsicmp(effect, L"SCALE") == 0)
		{
			WSVector scale = SeparateList(parameter, L",", 2);
			Magick::Geometry newSize;
			size_t percentPos = scale[0].find(L"%");
			if (percentPos != std::wstring::npos)
			{
				newSize.percent(true);
				size_t percent = MathParser::ParseI(scale[0].substr(0, percentPos));
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
		else if (_wcsicmp(effect, L"SAMPLE") == 0)
		{
			WSVector valList = SeparateList(parameter, L",", 2);

			img.sample(Magick::Geometry(
				MathParser::ParseI(valList[0]),
				MathParser::ParseI(valList[1])
			));
		}
		else if (_wcsicmp(effect, L"RESAMPLE") == 0)
		{
			double val = MathParser::ParseF(parameter);
			img.resample(val);
		}
		else if (_wcsicmp(effect, L"CROP") == 0)
		{
			WSVector geometryRaw = SeparateList(parameter, L",", 5);
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
			img.rotationalBlur(val);
		}
		else if (_wcsicmp(effect, L"IMPLODE") == 0)
		{
			double val = MathParser::ParseF(parameter);
			img.implode(val);
		}
		else if (_wcsicmp(effect, L"SPREAD") == 0)
		{
			double val = MathParser::ParseF(parameter);
			img.spread(val);
		}
		else if (_wcsicmp(effect, L"SWIRL") == 0)
		{
			double val = MathParser::ParseF(parameter);
			img.swirl(val);
		}
		else if (_wcsicmp(effect, L"MEDIANFILTER") == 0)
		{
			double val = MathParser::ParseF(parameter);
			img.medianFilter(val);

		}
		else if (_wcsicmp(effect, L"EQUALIZE") == 0)
		{
			img.equalize();
		}
		else if (_wcsicmp(effect, L"ENHANCE") == 0)
		{
			img.enhance();
		}
		else if (_wcsicmp(effect, L"DESPECKLE") == 0)
		{
			img.despeckle();
		}
		else if (_wcsicmp(effect, L"REDUCENOISE") == 0)
		{
			img.reduceNoise();
		}
		else if (_wcsicmp(effect, L"TRANSPOSE") == 0)
		{
			img.transpose();
		}
		else if (_wcsicmp(effect, L"TRANSVERSE") == 0)
		{
			img.transverse();
		}
		else if (_wcsicmp(effect, L"FLIP") == 0)
		{
			img.flip();
		}
		else if (_wcsicmp(effect, L"FLOP") == 0)
		{
			img.flop();
		}
		else if (_wcsicmp(effect, L"MAGNIFY") == 0)
		{
			img.magnify();
		}
		else if (_wcsicmp(effect, L"MINIFY") == 0)
		{
			img.minify();
		}
		return TRUE;
	}
	catch (Magick::Exception &error_)
	{
		error2pws(error_);
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