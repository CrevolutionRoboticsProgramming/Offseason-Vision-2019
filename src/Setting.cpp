#include "Setting.hpp"

std::vector<std::string> split(std::string string, std::string regex)
{
    std::string newString;
    std::vector<std::string> returnVector;
    while (string.find(regex) != std::string::npos)
    {
        newString = string.substr(0, string.find(regex));
        string = string.substr(string.find(regex) + 1, string.length() - 1);
        returnVector.push_back(newString);
    }
    returnVector.push_back(string);
    return returnVector;
}

IntSetting::IntSetting(std::string label, int value, bool forcePositive)
{
    this->label = label;
    this->value = value;
    this->forcePositive = forcePositive;
}
bool IntSetting::setValue(std::string value)
{
    bool isSanitary{true};

    for (int i{0}; i < value.size(); ++i)
    {
        if (!std::isdigit(value.at(i)))
        {
            if (!forcePositive && i == 0 && value.at(i) == '-' && value.size() > 1)
                break;
            isSanitary = false;
        }
    }

    if (isSanitary)
        this->value = std::stoi(value);
        
    return isSanitary;
}
std::string IntSetting::toString()
{
    return std::to_string(value);
}

DoubleSetting::DoubleSetting(std::string label, double value, bool forcePositive)
{
    this->label = label;
    this->value = value;
    this->forcePositive = forcePositive;
}
bool DoubleSetting::setValue(std::string value)
{
    bool isSanitary{true};

    // If there's more than one decimal
    if (split(value, ".").size() > 2)
        isSanitary = false;
    for (int i{0}; i < value.size(); ++i)
    {
        if (!std::isdigit(value.at(i)) && value.at(i) != '.' && !(value.at(i) == 0 && value.at(i) == '-' && value.size() > 1))
            isSanitary = false;
    }

    if (isSanitary)
        this->value = std::stod(value);

    return isSanitary;
}
std::string DoubleSetting::toString()
{
    return std::to_string(value);
}

BoolSetting::BoolSetting(std::string label, bool value)
{
    this->label = label;
    this->value = value;
}
bool BoolSetting::setValue(std::string value)
{
    this->value = value == "true" || value == "1";
    return value == "true" || value == "false" || value == "0" || value == "1";
}
std::string BoolSetting::toString()
{
    return value ? "true" : "false";
}

StringSetting::StringSetting(std::string label, std::string value)
{
    this->label = label;
    this->value = value;
}
StringSetting::StringSetting(std::string label, std::string value, const std::function<bool (std::string)> &sanitizer)
    : StringSetting(label, value)
{
    this->sanitizer = sanitizer;
    customSanitizer = true;
}
bool StringSetting::setValue(std::string value)
{
    bool isSanitary{true};

    if (customSanitizer)
        isSanitary = sanitizer(value);

    this->value = value;

    return isSanitary;
}
std::string StringSetting::toString()
{
    return value;
}
