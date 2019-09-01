#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <functional>
#include <memory>
#include <sstream>
#include "Setting.hpp"

std::vector<std::string> split(std::string string, std::string regex);

struct Config
{
    std::string label{"UNLABELED"};
    std::vector<std::unique_ptr<Setting>> settings;
};

struct SystemConfig : public Config
{
    BoolSetting verbose{"verbose", false};
    BoolSetting tuning{"tuning", false};

    StringSetting address{"address", "127.0.0.1",
    [=] (std::string value)
    {
        bool returnValue{true};

        if (split(value, ".").size() != 4)
            returnValue = false;
        for (std::string number : split(value, "."))
        {
            if (number.length() == 0 || number.length() > 3)
                returnValue = false;
        }

        return returnValue;
    }};

    IntSetting videoPort{"video-port", 1181, true};
    IntSetting communicatorPort{"communicator-port", 1182, true};
    IntSetting robotPort{"robot-port", 1183, true};
    IntSetting receivePort{"receive-port", 1184, true};

    SystemConfig();
};

struct VisionConfig : public Config
{
    IntSetting lowHue{"low-hue", 0, true};
    IntSetting lowSaturation{"low-saturation", 0, true};
    IntSetting lowValue{"low-value", 0, true};
    IntSetting highHue{"high-hue", 255, true};
    IntSetting highSaturation{"high-saturation", 255, true};
    IntSetting highValue{"high-value", 255, true};
    IntSetting erosionDilationPasses{"erosion-dilation-passes", 1, true};
    IntSetting minArea{"min-area", 0, true};
    IntSetting minRotation{"min-rotation", 0, true};
    IntSetting allowableError{"allowable-error", 10000000, true};

    VisionConfig();
};

struct UVCCameraConfig : public Config
{
    IntSetting width{"width", 320, true};
    IntSetting height{"height", 240, true};
    IntSetting fps{"fps", 15, true};
    IntSetting exposure{"exposure", 0, true};
    IntSetting exposureAuto{"exposure-auto", 1, true};

    UVCCameraConfig();
};

struct RaspiCameraConfig : public Config
{
    IntSetting width{"width", 320, true};
    IntSetting height{"height", 240, true};
    IntSetting fps{"fps", 15, true};
    IntSetting shutterSpeed{"shutter-speed", 200, true};
    IntSetting exposureMode{"exposure-mode", 1, true};
    IntSetting horizontalFOV{"horizontal-fov", 75, true};

    RaspiCameraConfig();
};

// Replace with varidic function/template?
void parseConfigs(std::vector<std::unique_ptr<Config>> &configs);
void parseConfigs(std::vector<std::unique_ptr<Config>> &configs, std::string string);

void writeConfigs(std::vector<std::unique_ptr<Config>> &configs);
