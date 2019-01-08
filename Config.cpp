#include "MagickMeter.h"

BOOL Config::Match(LPCWSTR input)
{
    return Utils::IsEqual(name, input);
}

BOOL Config::Equal(LPCWSTR input)
{
    return Utils::IsEqual(para, input);
}

WSVector Config::ToList(int minItems, LPCWSTR defVal)
{
    return Utils::SeparateList(para, L",", minItems, defVal);
}

Magick::Color Config::ToColor() {
    return Utils::ParseColor(para);
}

BOOL Config::ToBool() {
    return MathParser::ParseBool(para);
}

std::string Config::ToString() {
    return Utils::WStringToString(para);
}

double Config::ToDouble() {
    return MathParser::ParseDouble(para);
}

int Config::ToInt() {
    return MathParser::ParseInt(para);
}

size_t Config::ToSizeT() {
    return MathParser::ParseSizeT(para);
}

ssize_t Config::ToSSizeT() {
    return MathParser::ParseSSizeT(para);
}