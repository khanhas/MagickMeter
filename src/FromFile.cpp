#include "MagickMeter.h"
#include <filesystem>

BOOL CreateNew(ImgStruct &dst, WSVector &setting, Measure * measure)
{
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
		else if (_wcsicmp(baseFile.c_str(), L"CLIPBOARD") == 0)
			targetFile = "clipboard:";
		else //A FILE PATH
			targetFile = ws2s(baseFile);

		BOOL isDefinedSize = FALSE;
		Magick::Geometry defineSize;

		for (auto &settingIt : setting)
		{
			std::wstring tempName, tempParameter;
			GetNamePara(settingIt, tempName, tempParameter);
			LPCWSTR name = tempName.c_str();
			LPCWSTR parameter = tempParameter.c_str();

			if (_wcsicmp(name, L"RENDERSIZE") == 0)
			{
				WSVector imgSize = SeparateParameter(parameter, 3);

				int width = MathParser::ParseI(imgSize[0]);
				int height = MathParser::ParseI(imgSize[1]);

				if (width <= 0 && height <= 0)
				{
					RmLogF(measure->rm, 2, L"%s is invalid RenderSize. Render image at normal size.", parameter);
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
					}
					isDefinedSize = TRUE;
				}

				settingIt = L"";
			}
		}


		if (isSS) RmExecute(measure->skin, L"!SetTransparency 0");

		if (isDefinedSize)
		{
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

		dst.W = dst.contain.columns();
		dst.H = dst.contain.rows();
	}
	catch(Magick::Exception &error_)
	{
		error2pws(error_);
		return FALSE;
	}

	return TRUE;
}