#include "MagickMeter.h"
#include <filesystem>
#include <sstream>

BOOL Measure::CreateFromFile(std::shared_ptr<ImgContainer> out)
{
    std::ostringstream targetFile;
    BOOL isScreenshot = FALSE;

    if (out->config.at(0).Equal(L"SCREENSHOT"))
    {
        isScreenshot = TRUE;
        targetFile << "screenshot:";

        std::wstring baseString = out->config.at(0).para;
        const size_t monitorFlag = baseString.find(L"@", 10);

        if (monitorFlag != std::wstring::npos &&
            (monitorFlag + 1) != baseString.length())
        {
            const int monitorIndex = MathParser::ParseInt(baseString.substr(11));
            targetFile << "[" << monitorIndex << "]";
        }

        if (out->config.size() > 1 && out->config.at(1).Match(L"AUTOHIDE"))
        {
            isScreenshot = out->config.at(1).ToBool();
            out->config.at(1).isApplied = TRUE;
        }
    }
    else if (out->config.at(0).Equal(L"CLIPBOARD"))
    {
        targetFile << "clipboard:";
    }
    else // baseFile is a file path or URL
    {
        targetFile << out->config.at(0).ToString();
    }

    BOOL isDefinedSize = FALSE;
    Magick::Geometry defineSize;

    for (auto &option : out->config)
    {
        if (option.isApplied)
            continue;

        ParseInternalVariable(option.para, out);


        if (option.Match(L"RENDERSIZE"))
        {
            WSVector imgSize = option.ToList(3);

            const size_t width = MathParser::ParseSizeT(imgSize.at(0));
            const size_t height = MathParser::ParseSizeT(imgSize.at(1));

            if (width <= 0 && height <= 0)
            {
                RmLogF(rm, 2, L"%s is invalid RenderSize. Render image at normal size.", option.para.c_str());
                isDefinedSize = FALSE;
            }
            else
            {
                if (width > 0) defineSize.width(width);
                if (height > 0) defineSize.height(height);

                Utils::SetGeometryMode(MathParser::ParseInt(imgSize.at(2)), defineSize);

                isDefinedSize = TRUE;
            }

            option.isApplied = TRUE;
        }
    }

	try
	{
        if (isScreenshot) RmExecute(skin, L"!SetTransparency 0");

        if (isDefinedSize)
		{
			out->img.read(defineSize, targetFile.str()); // Why it just works with SVG?
			out->img.resize(defineSize);
		}
		else
		{
			out->img.read(targetFile.str());
		}

		if (isScreenshot) RmExecute(skin, L"!SetTransparency 255");

		if (!out->img.isValid()) return FALSE;

		out->img.strip();
		out->img.alpha(true);

        out->geometry = Magick::Geometry{
            out->img.columns(),
            out->img.rows()
        };
	}
	catch(Magick::Exception &error_)
	{
		LogError(error_);
		return FALSE;
	}

	return TRUE;
}