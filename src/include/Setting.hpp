#include <iostream>
#include <string>
#include <vector>
#include <functional>

std::vector<std::string> split(std::string string, std::string regex);

class Setting
{
public:
    std::string label{};
    virtual bool setValue(std::string value) = 0;
    virtual std::string asString() = 0;
};

class IntSetting : public Setting
{
private:
    bool forcePositive;

public:
    int value{};
    IntSetting(std::string label, int value, bool forcePositive = false);
    bool setValue(std::string value) override;
    std::string asString() override;
};

class DoubleSetting : public Setting
{
private:
    bool forcePositive;

public:
    double value{};
    DoubleSetting(std::string label, double value, bool forcePositive = false);
    bool setValue(std::string value) override;
    std::string asString() override;
};

class BoolSetting : public Setting
{
public:
    bool value{};
    BoolSetting(std::string label, bool value);
    bool setValue(std::string value) override;
    std::string asString() override;
};

class StringSetting : public Setting
{
private:
    bool customSanitizer{false};
    std::function<bool (std::string)> sanitizer{};

public:
    std::string value{};
    StringSetting(std::string label, std::string value);
    StringSetting(std::string label, std::string value, const std::function<bool (std::string)> &sanitizer);
    bool setValue(std::string value) override;
    std::string asString() override;
};
