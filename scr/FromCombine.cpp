#include "MagickMeter.h"

BOOL CreateCombine(ImgStruct * dst, std::vector<std::wstring> setting, std::vector<ImgStruct *> &list, Measure * measure)
{
	std::wstring baseImg = setting[0].substr(setting[0].find_first_not_of(L" \t\r\n", 7));
	int baseIndex = NameToIndex(baseImg);
	if (baseIndex != -1 && baseIndex < list.size())
	{
		dst->contain = list[baseIndex]->contain;
		list[baseIndex]->isDelete = TRUE;
	}
	else
	{
		RmLog(2, L"Dude! lol!");
		return FALSE;
	}
	setting.erase(setting.begin());
	for (auto &settingIt : setting)
	{
		std::wstring tempName, tempParameter;
		GetNamePara(settingIt, tempName, tempParameter);
		LPCWSTR name = tempName.c_str();
		LPCWSTR parameter = tempParameter.c_str();

		int index = NameToIndex(parameter);
		if (index != -1 && index < list.size())
		{
			/*useless comp
			BlurCompositeOp
			ClearCompositeOp (maybe)
			ReplaceCompositeOp
			DifferenceCompositeOp
			DisplaceCompositeOp
			DistortCompositeOp
			DissolveCompositeOp
			LightenIntensityCompositeOp
			DarkenIntensityCompositeOp
			ClearCompositeOp (delete all)
			*/
			list[index]->isDelete = TRUE;
			MagickCore::CompositeOperator compOp = MagickCore::UndefinedCompositeOp;
			if (_wcsicmp(name, L"MINUS") == 0)
			{
				compOp = MagickCore::MinusDstCompositeOp;
			}
			else if (_wcsicmp(name, L"BUMPMAP") == 0)
			{
				compOp = MagickCore::BumpmapCompositeOp;
			}
			else if (_wcsicmp(name, L"MASK") == 0)
			{
				compOp = MagickCore::CopyAlphaCompositeOp;
			}
			else if (_wcsicmp(name, L"UNION") == 0)
			{
				compOp = MagickCore::OverCompositeOp;
			}
			else if (_wcsicmp(name, L"XOR") == 0)
			{
				compOp = MagickCore::XorCompositeOp;
			}
			else if (_wcsicmp(name, L"BLEND") == 0)
			{
				compOp = MagickCore::BlendCompositeOp;
			}
			else if (_wcsicmp(name, L"HUE") == 0)
			{
				compOp = MagickCore::HueCompositeOp;
			}

			else if (_wcsicmp(name, L"COLORBURN") == 0)
			{
				compOp = MagickCore::ColorBurnCompositeOp;
			}
			else if (_wcsicmp(name, L"COLORDODGE") == 0)
			{
				compOp = MagickCore::ColorDodgeCompositeOp;
			}
			else if (_wcsicmp(name, L"DARKEN") == 0)
			{
				compOp = MagickCore::DarkenCompositeOp;
			}
			else if (_wcsicmp(name, L"DIVIDE2") == 0)
			{
				compOp = MagickCore::DivideSrcCompositeOp;
			}
			else if (_wcsicmp(name, L"DIVIDE") == 0)
			{
				compOp = MagickCore::DivideDstCompositeOp;
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

			try
			{
				if (compOp != MagickCore::UndefinedCompositeOp)
				{
					//Extend main image size
					if (dst->contain.size() < list[index]->contain.size())
						dst->contain.size(list[index]->contain.size());

					RmLog(2, name);
					RmLog(2, parameter);
					dst->contain.composite(list[index]->contain, 0, 0, compOp);
				}
			}
			catch (std::exception &error_)
			{
				RmLogF(measure->rm, 1, L"%s", error2pws(error_));
				return FALSE;
			}
		}
		else
		{
			if (!ParseEffect(measure->rm, dst->contain, name, parameter))
				return FALSE;
		}
	}
	return TRUE;
}