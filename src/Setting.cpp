#include "Setting.hpp"

void Setting::setValue(std::string value)
{
}
std::string Setting::asString()
{
    return std::string{};
}

IntSetting::IntSetting(std::string label, int value)
{
    this->label = label;
    this->value = value;
}
void IntSetting::setValue(std::string value)
{
    this->value = std::stoi(value);
}
std::string IntSetting::asString()
{
    return std::to_string(value);
}

DoubleSetting::DoubleSetting(std::string label, double value)
{
    this->label = label;
    this->value = value;
}
void DoubleSetting::setValue(std::string value)
{
    this->value = std::stod(value);
}
std::string DoubleSetting::asString()
{
    return std::to_string(value);
}

BoolSetting::BoolSetting(std::string label, bool value)
{
    this->label = label;
    this->value = value;
}
void BoolSetting::setValue(std::string value)
{
    this->value = value == "true" || value == "1";
}
std::string BoolSetting::asString()
{
    return value ? "true" : "false";
}

StringSetting::StringSetting(std::string label, std::string value)
{
    this->label = label;
    this->value = value;
}
void StringSetting::setValue(std::string value)
{
    this->value = value;
}
std::string StringSetting::asString()
{
    return value;
}
