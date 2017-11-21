#include "MagickMeter.h"

BOOL CreateCombine(ImgStruct * dst, WSVector setting, Measure * measure)
{
	std::wstring baseImg = setting[0];
	int baseIndex = NameToIndex(baseImg);
	if (baseIndex != -1 && baseIndex < measure->imgList.size())
	{
		dst->contain = measure->imgList[baseIndex]->contain;
		measure->imgList[baseIndex]->isDelete = TRUE;
	}
	else
	{
		RmLogF(measure->rm, 2, L"ParentImageName: \"%s\" is invalid.", baseImg.c_str());
		return FALSE;
	}
	setting.erase(setting.begin());

	for (int i = 0; i < setting.size(); i++)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(setting[i], tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();

		int index = NameToIndex(parameter);
		if (index != -1 && index < measure->imgList.size())
		{
			MagickCore::CompositeOperator compOp = MagickCore::UndefinedCompositeOp;
			
			if (_wcsicmp(name, L"ALPHACOMP") == 0)
			{
				compOp = MagickCore::AlphaCompositeOp;
			}
			else if (_wcsicmp(name, L"ATOP") == 0)
			{
				compOp = MagickCore::AtopCompositeOp;
			}
			else if (_wcsicmp(name, L"BLEND") == 0)
			{
				compOp = MagickCore::BlendCompositeOp;
			}
			else if (_wcsicmp(name, L"BLURCOMP") == 0)
			{
				compOp = MagickCore::BlurCompositeOp;
			}
			else if (_wcsicmp(name, L"BUMPMAP") == 0)
			{
				compOp = MagickCore::BumpmapCompositeOp;
			}
			else if (_wcsicmp(name, L"CHANGEMASK") == 0)
			{
				compOp = MagickCore::ChangeMaskCompositeOp;
			}
			else if (_wcsicmp(name, L"CLEAR") == 0)
			{
				compOp = MagickCore::ClearCompositeOp;
			}
			else if (_wcsicmp(name, L"COLORBURN") == 0)
			{
				compOp = MagickCore::ColorBurnCompositeOp;
			}
			else if (_wcsicmp(name, L"COLORDODGE") == 0)
			{
				compOp = MagickCore::ColorDodgeCompositeOp;
			}
			else if (_wcsicmp(name, L"COLORIZE") == 0)
			{
				compOp = MagickCore::ColorizeCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYBLACK") == 0)
			{
				compOp = MagickCore::CopyBlackCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYBLUE") == 0)
			{
				compOp = MagickCore::CopyBlueCompositeOp;
			}
			else if (_wcsicmp(name, L"COPY") == 0)
			{
				compOp = MagickCore::CopyCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYCYAN") == 0)
			{
				compOp = MagickCore::CopyCyanCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYGREEN") == 0)
			{
				compOp = MagickCore::CopyGreenCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYMAGENTA") == 0)
			{
				compOp = MagickCore::CopyMagentaCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYALPHA") == 0)
			{
				compOp = MagickCore::CopyAlphaCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYRED") == 0)
			{
				compOp = MagickCore::CopyRedCompositeOp;
			}
			else if (_wcsicmp(name, L"COPYYELLOW") == 0)
			{
				compOp = MagickCore::CopyYellowCompositeOp;
			}
			else if (_wcsicmp(name, L"DARKEN") == 0)
			{
				compOp = MagickCore::DarkenCompositeOp;
			}
			else if (_wcsicmp(name, L"DARKENINTENSITY") == 0)
			{
				compOp = MagickCore::DarkenIntensityCompositeOp;
			}
			else if (_wcsicmp(name, L"DIFFERENCE") == 0)
			{
				compOp = MagickCore::DifferenceCompositeOp;
			}
			else if (_wcsicmp(name, L"DISPLACE") == 0)
			{
				compOp = MagickCore::DisplaceCompositeOp;
			}
			else if (_wcsicmp(name, L"DISSOLVE") == 0)
			{
				compOp = MagickCore::DissolveCompositeOp;
			}
			else if (_wcsicmp(name, L"DISTORT") == 0)
			{
				compOp = MagickCore::DistortCompositeOp;
			}
			else if (_wcsicmp(name, L"DIVIDEDST") == 0)
			{
				compOp = MagickCore::DivideDstCompositeOp;
			}
			else if (_wcsicmp(name, L"DIVIDESRC") == 0)
			{
				compOp = MagickCore::DivideSrcCompositeOp;
			}
			else if (_wcsicmp(name, L"DSTATOP") == 0)
			{
				compOp = MagickCore::DstAtopCompositeOp;
			}
			else if (_wcsicmp(name, L"DSTCOMP") == 0)
			{
				compOp = MagickCore::DstCompositeOp;
			}
			else if (_wcsicmp(name, L"DSTINCOMP") == 0)
			{
				compOp = MagickCore::DstInCompositeOp;
			}
			else if (_wcsicmp(name, L"DSTOUTCOMP") == 0)
			{
				compOp = MagickCore::DstOutCompositeOp;
			}
			else if (_wcsicmp(name, L"DSTOVERCOMP") == 0)
			{
				compOp = MagickCore::DstOverCompositeOp;
			}
			else if (_wcsicmp(name, L"EXCLUSION") == 0)
			{
				compOp = MagickCore::ExclusionCompositeOp;
			}
			else if (_wcsicmp(name, L"HARDLIGHT") == 0)
			{
				compOp = MagickCore::HardLightCompositeOp;
			}
			else if (_wcsicmp(name, L"HARDMIX") == 0)
			{
				compOp = MagickCore::HardMixCompositeOp;
			}
			else if (_wcsicmp(name, L"HUE") == 0)
			{
				compOp = MagickCore::HueCompositeOp;
			}
			else if (_wcsicmp(name, L"INCOMP") == 0)
			{
				compOp = MagickCore::InCompositeOp;
			}
			else if (_wcsicmp(name, L"INTENSITY") == 0)
			{
				compOp = MagickCore::IntensityCompositeOp;
			}
			else if (_wcsicmp(name, L"LIGHTEN") == 0)
			{
				compOp = MagickCore::LightenCompositeOp;
			}
			else if (_wcsicmp(name, L"LIGHTENINTENSITY") == 0)
			{
				compOp = MagickCore::LightenIntensityCompositeOp;
			}
			else if (_wcsicmp(name, L"LINEARBURN") == 0)
			{
				compOp = MagickCore::LinearBurnCompositeOp;
			}
			else if (_wcsicmp(name, L"LINEARDODGE") == 0)
			{
				compOp = MagickCore::LinearDodgeCompositeOp;
			}
			else if (_wcsicmp(name, L"LINEARLIGHT") == 0)
			{
				compOp = MagickCore::LinearLightCompositeOp;
			}
			else if (_wcsicmp(name, L"LUMINIZE") == 0)
			{
				compOp = MagickCore::LuminizeCompositeOp;
			}
			else if (_wcsicmp(name, L"MATHEMATICS") == 0)
			{
				compOp = MagickCore::MathematicsCompositeOp;
			}
			else if (_wcsicmp(name, L"MINUSDST") == 0)
			{
				compOp = MagickCore::MinusDstCompositeOp;
			}
			else if (_wcsicmp(name, L"MINUSSRC") == 0)
			{
				compOp = MagickCore::MinusSrcCompositeOp;
			}
			else if (_wcsicmp(name, L"MODULATE") == 0)
			{
				compOp = MagickCore::ModulateCompositeOp;
			}
			else if (_wcsicmp(name, L"MODULUSADD") == 0)
			{
				compOp = MagickCore::ModulusAddCompositeOp;
			}
			else if (_wcsicmp(name, L"MODULUSSUBTRACT") == 0)
			{
				compOp = MagickCore::ModulusSubtractCompositeOp;
			}
			else if (_wcsicmp(name, L"MULTIPLY") == 0)
			{
				compOp = MagickCore::MultiplyCompositeOp;
			}
			else if (_wcsicmp(name, L"NOCOMP") == 0)
			{
				compOp = MagickCore::NoCompositeOp;
			}
			else if (_wcsicmp(name, L"OUTCOMP") == 0)
			{
				compOp = MagickCore::OutCompositeOp;
			}
			else if (_wcsicmp(name, L"OVERCOMP") == 0)
			{
				compOp = MagickCore::OverCompositeOp;
			}
			else if (_wcsicmp(name, L"OVERLAY") == 0)
			{
				compOp = MagickCore::OverlayCompositeOp;
			}
			else if (_wcsicmp(name, L"PEGTOPLIGHT") == 0)
			{
				compOp = MagickCore::PegtopLightCompositeOp;
			}
			else if (_wcsicmp(name, L"PINLIGHT") == 0)
			{
				compOp = MagickCore::PinLightCompositeOp;
			}
			else if (_wcsicmp(name, L"PLUS") == 0)
			{
				compOp = MagickCore::PlusCompositeOp;
			}
			else if (_wcsicmp(name, L"REPLACE") == 0)
			{
				compOp = MagickCore::ReplaceCompositeOp;
			}
			else if (_wcsicmp(name, L"SATURATE") == 0)
			{
				compOp = MagickCore::SaturateCompositeOp;
			}
			else if (_wcsicmp(name, L"SCREEN") == 0)
			{
				compOp = MagickCore::ScreenCompositeOp;
			}
			else if (_wcsicmp(name, L"SOFTLIGHT") == 0)
			{
				compOp = MagickCore::SoftLightCompositeOp;
			}
			else if (_wcsicmp(name, L"SRCATOP") == 0)
			{
				compOp = MagickCore::SrcAtopCompositeOp;
			}
			else if (_wcsicmp(name, L"SRCCOMP") == 0)
			{
				compOp = MagickCore::SrcCompositeOp;
			}
			else if (_wcsicmp(name, L"SRCINCOMP") == 0)
			{
				compOp = MagickCore::SrcInCompositeOp;
			}
			else if (_wcsicmp(name, L"SRCOUTCOMP") == 0)
			{
				compOp = MagickCore::SrcOutCompositeOp;
			}
			else if (_wcsicmp(name, L"SRCOVERCOMP") == 0)
			{
				compOp = MagickCore::SrcOverCompositeOp;
			}
			else if (_wcsicmp(name, L"THRESHOLD") == 0)
			{
				compOp = MagickCore::ThresholdCompositeOp;
			}
			else if (_wcsicmp(name, L"VIVIDLIGHT") == 0)
			{
				compOp = MagickCore::VividLightCompositeOp;
			}
			else if (_wcsicmp(name, L"XOR") == 0)
			{
				compOp = MagickCore::XorCompositeOp;
			}

			setting[i] = L"";

			try
			{
				if (compOp != MagickCore::UndefinedCompositeOp)
				{
					measure->imgList[index]->isDelete = TRUE;

					//Extend main image size
					if (dst->contain.size() < measure->imgList[index]->contain.size())
						dst->contain.size(measure->imgList[index]->contain.size());

					dst->contain.composite(measure->imgList[index]->contain, 0, 0, compOp);
				}

			}
			catch (Magick::Exception &error_)
			{
				error2pws(error_);
				return FALSE;
			}

		}

		for (auto &settingIt : setting)
		{
			if (settingIt.empty()) continue;
			std::wstring name, parameter;
			GetNamePara(settingIt, name, parameter);

			if (!ParseEffect(measure->rm, dst->contain, name, parameter))
				return FALSE;
		}
	}
	return TRUE;
}