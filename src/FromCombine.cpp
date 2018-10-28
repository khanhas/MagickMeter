#include "MagickMeter.h"

const std::map<LPCWSTR, Magick::CompositeOperator> compOperationMap{
    {L"ALPHACOMP",          Magick::AlphaCompositeOp},
    {L"ATOP",               Magick::AtopCompositeOp},
    {L"BLEND",              Magick::BlendCompositeOp},
    {L"BLURCOMP",           Magick::BlurCompositeOp},
    {L"BUMPMAP",            Magick::BumpmapCompositeOp},
    {L"CHANGEMASK",         Magick::ChangeMaskCompositeOp},
    {L"CLEAR",              Magick::ClearCompositeOp},
    {L"COLORBURN",          Magick::ColorBurnCompositeOp},
    {L"COLORDODGE",         Magick::ColorDodgeCompositeOp},
    {L"COLORIZE",           Magick::ColorizeCompositeOp},
    {L"COPYBLACK",          Magick::CopyBlackCompositeOp},
    {L"COPYBLUE",           Magick::CopyBlueCompositeOp},
    {L"COPY",               Magick::CopyCompositeOp},
    {L"COPYCYAN",           Magick::CopyCyanCompositeOp},
    {L"COPYGREEN",          Magick::CopyGreenCompositeOp},
    {L"COPYMAGENTA",        Magick::CopyMagentaCompositeOp},
    {L"COPYALPHA",          Magick::CopyAlphaCompositeOp},
    {L"COPYRED",            Magick::CopyRedCompositeOp},
    {L"COPYYELLOW",         Magick::CopyYellowCompositeOp},
    {L"DARKEN",             Magick::DarkenCompositeOp},
    {L"DARKENINTENSITY",    Magick::DarkenIntensityCompositeOp},
    {L"DIFFERENCE",         Magick::DifferenceCompositeOp},
    {L"DISPLACE",           Magick::DisplaceCompositeOp},
    {L"DISSOLVE",           Magick::DissolveCompositeOp},
    {L"DISTORT",            Magick::DistortCompositeOp},
    {L"DIVIDEDST",          Magick::DivideDstCompositeOp},
    {L"DIVIDESRC",          Magick::DivideSrcCompositeOp},
    {L"DSTATOP",            Magick::DstAtopCompositeOp},
    {L"DSTCOMP",            Magick::DstCompositeOp},
    {L"DSTINCOMP",          Magick::DstInCompositeOp},
    {L"DSTOUTCOMP",         Magick::DstOutCompositeOp},
    {L"DSTOVERCOMP",        Magick::DstOverCompositeOp},
    {L"EXCLUSION",          Magick::ExclusionCompositeOp},
    {L"HARDLIGHT",          Magick::HardLightCompositeOp},
    {L"HARDMIX",            Magick::HardMixCompositeOp},
    {L"HUE",                Magick::HueCompositeOp},
    {L"INCOMP",             Magick::InCompositeOp},
    {L"INTENSITY",          Magick::IntensityCompositeOp},
    {L"LIGHTEN",            Magick::LightenCompositeOp},
    {L"LIGHTENINTENSITY",   Magick::LightenIntensityCompositeOp},
    {L"LINEARBURN",         Magick::LinearBurnCompositeOp},
    {L"LINEARDODGE",        Magick::LinearDodgeCompositeOp},
    {L"LINEARLIGHT",        Magick::LinearLightCompositeOp},
    {L"LUMINIZE",           Magick::LuminizeCompositeOp},
    {L"MATHEMATICS",        Magick::MathematicsCompositeOp},
    {L"MINUSDST",           Magick::MinusDstCompositeOp},
    {L"MINUSSRC",           Magick::MinusSrcCompositeOp},
    {L"MODULATE",           Magick::ModulateCompositeOp},
    {L"MODULUSADD",         Magick::ModulusAddCompositeOp},
    {L"MODULUSSUBTRACT",    Magick::ModulusSubtractCompositeOp},
    {L"MULTIPLY",           Magick::MultiplyCompositeOp},
    {L"NOCOMP",             Magick::NoCompositeOp},
    {L"OUTCOMP",            Magick::OutCompositeOp},
    {L"OVERCOMP",           Magick::OverCompositeOp},
    {L"OVERLAY",            Magick::OverlayCompositeOp},
    {L"PEGTOPLIGHT",        Magick::PegtopLightCompositeOp},
    {L"PINLIGHT",           Magick::PinLightCompositeOp},
    {L"PLUS",               Magick::PlusCompositeOp},
    {L"REPLACE",            Magick::ReplaceCompositeOp},
    {L"SATURATE",           Magick::SaturateCompositeOp},
    {L"SCREEN",             Magick::ScreenCompositeOp},
    {L"SOFTLIGHT",          Magick::SoftLightCompositeOp},
    {L"SRCATOP",            Magick::SrcAtopCompositeOp},
    {L"SRCCOMP",            Magick::SrcCompositeOp},
    {L"SRCINCOMP",          Magick::SrcInCompositeOp},
    {L"SRCOUTCOMP",         Magick::SrcOutCompositeOp},
    {L"SRCOVERCOMP",        Magick::SrcOverCompositeOp},
    {L"THRESHOLD",          Magick::ThresholdCompositeOp},
    {L"VIVIDLIGHT",         Magick::VividLightCompositeOp},
    {L"XOR",                Magick::XorCompositeOp},
    {L"STEREO",             Magick::CompositeOperator::StereoCompositeOp}
};

BOOL Measure::CreateCombine(LPCWSTR baseImage, WSVector &config, ImgContainer &out)
{
    const int baseIndex = Utils::NameToIndex(baseImage);
    if (baseIndex != -1 && baseIndex < imgList.size())
    {
        out.img = imgList.at(baseIndex).img;
        imgList.at(baseIndex).isCombined = TRUE;
    }
    else
    {
        RmLogF(rm, 2, L"Combine: %s is invalid.", baseImage);
        return FALSE;
    }

    for (auto &option : config)
    {
        if (option.empty())
            continue;

        std::wstring tempName;
        std::wstring tempPara;
        Utils::GetNamePara(option, tempName, tempPara);

        LPCWSTR name = tempName.c_str();
        LPCWSTR parameter = tempPara.c_str();

        auto compOp = std::find_if(
            compOperationMap.begin(),
            compOperationMap.end(),
            [name](const std::pair<LPCWSTR, Magick::CompositeOperator> &i) noexcept {
                return _wcsicmp(name, i.first) == 0;
            }
        );

        if (compOp == compOperationMap.end())
        {
            continue;
        }

        // Found valid Composite operation
        option.clear();

        const int index = Utils::NameToIndex(tempPara);

        if (index < 0 || index >= imgList.size())
        {
            RmLogF(rm, 2, L"Combine: %s is invalid.", baseImage);
            return FALSE;
        }

        ImgContainer* childImage = &imgList[index];

        childImage->isCombined = TRUE;

        //Extend main image size
        const size_t newW = max(
            childImage->img.columns(),
            out.img.columns()
        );
        const size_t newH = max(
            childImage->img.rows(),
            out.img.rows()
        );

        Magick::Geometry newSize(newW, newH);

        try
        {
            Magick::Image temp = Magick::Image(out.img);

            out.img = Magick::Image(newSize, INVISIBLE);
            out.img.composite(temp, 0, 0, Magick::OverCompositeOp);

            temp = Magick::Image(newSize, INVISIBLE);
            temp.composite(childImage->img, 0, 0, Magick::OverCompositeOp);

            out.img.composite(temp, 0, 0, compOp->second);
        }
        catch (Magick::Exception &error_)
        {
            LogError(error_);
            return FALSE;
        }
    }

    return TRUE;
}
