#include "MagickMeter.h"
#include <filesystem>

BOOL CreateNew(ImgStruct * dst, std::vector<std::wstring> setting, Measure * measure)
{
	if (measure->isGIF)
	{
		dst->contain = measure->gifList[measure->GIFSeq];
		goto AddEffect;
	}

	try
	{
		std::string targetFile;
		BOOL isSS = FALSE;
		if (_wcsnicmp(setting[0].c_str(), L"SCREENSHOT", 10) == 0)
		{
			isSS = TRUE;
			if (setting.size() > 0 && _wcsnicmp(setting[1].c_str(), L"AUTOHIDE", 8) == 0)
			{
				double autoHide = 1;
				MathParser::CheckedParse(setting[1].substr(8), &autoHide);			
				isSS = (autoHide == 1);
				setting.erase(setting.begin() + 1);
			}
			targetFile = "screenshot:";
			if (setting[0].find(L"@", 10) != std::wstring::npos)
				targetFile += "[" + ws2s(setting[0].substr(11).c_str()) + "]";
		}
		else //A FILE PATH
		{
			targetFile = ws2s(setting[0]);
		}

		setting.erase(setting.begin());

		BOOL isDefinedSize = FALSE;
		__int64 width = 0;
		__int64 height = 0;

		if (setting.size() > 0)
		{
			isDefinedSize = _wcsnicmp(setting[0].c_str(), L"CANVAS", 6) == 0;

			if (isDefinedSize)
			{
				size_t start = setting[0].find_first_not_of(L" \t\n\r", 6);
				if (start != std::wstring::npos)
				{
					setting[0] = setting[0].substr(start);
					std::vector<std::wstring> imgSize =
						SeparateList(setting[0].c_str(), L",", 2);

					width  = MathParser::ParseI(imgSize[0]);
					height = MathParser::ParseI(imgSize[1]);

					if (!(width > 0 && height > 0))
						isDefinedSize = FALSE;
				}
				else
					isDefinedSize = FALSE;

				setting.erase(setting.begin());
			}
		}

		if (isDefinedSize)
		{
			if (isSS) RmExecute(measure->skin, L"!SetTransparency 0");
			dst->contain.read(Magick::Geometry(width, height), targetFile);
			if (isSS) RmExecute(measure->skin, L"!SetTransparency 255");
		}
		else
		{
			if (isSS) RmExecute(measure->skin, L"!SetTransparency 0");
			dst->contain.read(targetFile);
			if (isSS) RmExecute(measure->skin, L"!SetTransparency 255");
		}
			

		dst->contain.strip();

		LPCWSTR fileExt = s2ws(dst->contain.magick()).c_str();
		RmLog(2, fileExt);
		if (_wcsicmp(fileExt, L"GIF") == 0 ||
			_wcsicmp(fileExt, L"MP4") == 0 ||
			_wcsicmp(fileExt, L"MKV") == 0)
		{
			Magick::readImages(&measure->gifList, targetFile);

			if (isDefinedSize)
			{
				std::for_each(
					measure->gifList.begin(),
					measure->gifList.end(),
					Magick::resizeImage(Magick::Geometry(width, height))
				);
			}
			dst->contain = measure->gifList[0];
			measure->isGIF = TRUE;
		}
	}
	catch(std::exception &error_)
	{
		RmLogF(measure->rm, 1, L"MagickMeter Plugin: %s", error2pws(error_));
		return FALSE;
	}

	AddEffect:
	for (auto &settingIt : setting)
	{
		std::wstring name, parameter;
		GetNamePara(settingIt, name, parameter);

		if (!ParseEffect(measure->rm, dst->contain, name, parameter))
			return FALSE;
	}
	return TRUE;
}