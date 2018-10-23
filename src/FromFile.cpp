#include "MagickMeter.h"
#include <filesystem>
#include <sstream>

BOOL Measure::CreateFromFile(LPCWSTR baseFile, WSVector &config, ImgContainer &out)
{
    std::ostringstream targetFile;
    BOOL isScreenshot = FALSE;

    if (_wcsnicmp(baseFile, L"SCREENSHOT", 10) == 0)
    {
        isScreenshot = TRUE;
        if (config.size() > 0 && _wcsnicmp(config[0].c_str(), L"AUTOHIDE", 8) == 0)
        {
            isScreenshot = MathParser::ParseBool(config[0].substr(8));
            config[0].clear();
        }
        targetFile << "screenshot:";

        std::wstring baseString = baseFile;
        const size_t monitorFlag = baseString.find(L"@", 10);

        if (monitorFlag != std::wstring::npos &&
            (monitorFlag + 1) != baseString.length())
        {
            const int monitorIndex = MathParser::ParseInt(baseString.substr(11));
            targetFile << "[" << monitorIndex << "]";
        }
    }
    else if (_wcsicmp(baseFile, L"CLIPBOARD") == 0)
    {
        targetFile << "clipboard:";
    }
    else // baseFile is a file path or URL
    {
        targetFile << Utils::WStringToString(baseFile);
    }

    BOOL isDefinedSize = FALSE;
    Magick::Geometry defineSize;

    for (auto &option : config)
    {
        if (option.empty())
            continue;

        std::wstring tempName;
        std::wstring tempPara;
        Utils::GetNamePara(option, tempName, tempPara);
        ParseInternalVariable(tempPara, out);

        LPCWSTR name = tempName.c_str();

        if (_wcsicmp(name, L"RENDERSIZE") == 0)
        {
            WSVector imgSize = Utils::SeparateParameter(tempPara, 3);

            const size_t width = MathParser::ParseSizeT(imgSize[0]);
            const size_t height = MathParser::ParseSizeT(imgSize[1]);

            if (width <= 0 && height <= 0)
            {
                RmLogF(rm, 2, L"%s is invalid RenderSize. Render image at normal size.", tempPara);
                isDefinedSize = FALSE;
            }
            else
            {
                if (width > 0) defineSize.width(width);
                if (height > 0) defineSize.height(height);
                const int resizeType = MathParser::ParseInt(imgSize[2]);
                Utils::SetGeometryMode(resizeType, defineSize);
                isDefinedSize = TRUE;
            }

            option.clear();
        }
    }

	try
	{
        if (isScreenshot) RmExecute(skin, L"!SetTransparency 0");

        if (isDefinedSize)
		{
			out.img.read(defineSize, targetFile.str()); // Why it just works with SVG?
			out.img.resize(defineSize);
		}
		else
		{
			out.img.read(targetFile.str());
		}

		if (isScreenshot) RmExecute(skin, L"!SetTransparency 255");

		if (!out.img.isValid()) return FALSE;

		out.img.strip();
		out.img.alpha(true);

		out.W = out.img.columns();
		out.H = out.img.rows();
	}
	catch(Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}

	return TRUE;
}