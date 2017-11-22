#include "MagickMeter.h"
#include <filesystem>

BOOL CreateNew(ImgStruct &dst, WSVector &setting, Measure * measure)
{
	/*if (measure->isGIF)
	{
		dst.contain = measure->gifList[measure->GIFSeq];
		goto AddEffect;
	}*/

	std::wstring baseFile = setting[0];
	setting.erase(setting.begin());

	try
	{
		std::string targetFile;
		BOOL isSS = FALSE;
		if (_wcsnicmp(baseFile.c_str(), L"SCREENSHOT", 10) == 0)
		{
			isSS = TRUE;
			if (setting.size() > 0 && _wcsnicmp(setting[0].c_str(), L"AUTOHIDE", 8) == 0)
			{
				int autoHide = MathParser::ParseI(setting[0].substr(8));
				isSS = (autoHide == 1);
				setting.erase(setting.begin());
			}
			targetFile = "screenshot:";
			size_t monitorFlag = baseFile.find(L"@", 10);
			if (monitorFlag != std::wstring::npos && (monitorFlag + 1) != baseFile.length())
				targetFile += "[" + std::to_string(MathParser::ParseI(baseFile.substr(11))) + "]";
		}
		else //A FILE PATH
		{
			targetFile = ws2s(baseFile);
		}


		BOOL isDefinedSize = FALSE;
		Magick::Geometry defineSize;

		if (!setting[0].empty())
		{
			isDefinedSize = _wcsnicmp(setting[0].c_str(), L"CANVAS", 6) == 0;

			if (isDefinedSize)
			{
				size_t start = setting[0].find_first_not_of(L" \t\n\r", 6);
				if (start != std::wstring::npos)
				{
					setting[0] = setting[0].substr(start);
					WSVector imgSize =
						SeparateList(setting[0].c_str(), L",", 3);

					int width  = MathParser::ParseI(imgSize[0]);
					int height = MathParser::ParseI(imgSize[1]);

					if (width <= 0 && height <= 0)
					{
						RmLogF(measure->rm, 2, L"%s: Invalid Canvas width or height values. Read image at normal size.", baseFile);
						isDefinedSize = FALSE;
					}
					else
					{
						if (width > 0) defineSize.width(width);
						if (height > 0) defineSize.height(height);
						int resizeType = MathParser::ParseI(imgSize[2]);
						switch (resizeType)
						{
						case 1:
							defineSize.aspect(true);
							break;
						case 2:
							defineSize.fillArea(true);
							break;
						case 3:
							defineSize.greater(true);
							break;
						case 4:
							defineSize.less(true);
							break;
						case 5:
							defineSize.limitPixels(true);
							break;
						}
					}
				}
				else
					isDefinedSize = FALSE;

				setting.erase(setting.begin());
			}
		}


		if (isSS) RmExecute(measure->skin, L"!SetTransparency 0");

		if (isDefinedSize)
		{
			//TODO: CRASH!
			dst.contain.read(defineSize, targetFile); //Why it just works with SVG?
			dst.contain.resize(defineSize);
		}
		else
		{
			dst.contain.read(targetFile);
		}
		
		if (isSS) RmExecute(measure->skin, L"!SetTransparency 255");

		if (!dst.contain.isValid()) return FALSE;

		dst.contain.strip();

		/*LPCWSTR fileExt = s2ws(dst.contain.magick()).c_str();
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
			dst.contain = measure->gifList[0];
			measure->isGIF = TRUE;
		}*/
	}
	catch(Magick::Exception &error_)
	{
		error2pws(error_);
		return FALSE;
	}

	return TRUE;
}