#include <string>
#include <iostream>

struct Setting
{
    std::string label{};
    virtual void setValue(std::string value);
    virtual std::string asString();
};

struct IntSetting : public Setting
{
    int value{};
    IntSetting(std::string label, int value);
    void setValue(std::string value);
    std::string asString();
};

struct DoubleSetting : public Setting
{
    double value{};
    DoubleSetting(std::string label, double value);
    void setValue(std::string value);
    std::string asString();
};

struct BoolSetting : public Setting
{
    bool value{};
    BoolSetting(std::string label, bool value);
    void setValue(std::string value);
    std::string asString();
};

struct StringSetting : public Setting
{
    std::string value{};
    StringSetting(std::string label, std::string value);
    void setValue(std::string value);
    std::string asString();
};
