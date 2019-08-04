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
    StringSetting address{"address", "127.0.0.1"};
    IntSetting videoPort{"video-port", 1181};
    IntSetting sendPort{"send-port", 1182};
    IntSetting receivePort{"receive-port", 1183};

    SystemConfig();
};

struct VisionConfig : public Config
{
    IntSetting lowHue{"low-hue", 0};
    IntSetting lowSaturation{"low-saturation", 0};
    IntSetting lowValue{"low-value", 0};
    IntSetting highHue{"high-hue", 255};
    IntSetting highSaturation{"high-saturation", 255};
    IntSetting highValue{"high-value", 255};
    IntSetting erosionDilationPasses{"erosion-dilation-passes", 1};
    IntSetting minArea{"min-area", 0};
    IntSetting minRotation{"min-rotation", 0};
    IntSetting allowableError{"allowable-error", 10000000};

    VisionConfig();
};

struct UVCCameraConfig : public Config
{
    IntSetting width{"width", 320};
    IntSetting height{"height", 240};
    IntSetting fps{"fps", 15};
    IntSetting exposure{"exposure", 0};
    IntSetting exposureAuto{"exposure-auto", 1};

    UVCCameraConfig();
};

struct RaspiCameraConfig : public Config
{
    IntSetting width{"width", 320};
    IntSetting height{"height", 240};
    IntSetting fps{"fps", 15};
    IntSetting shutterSpeed{"shutter-speed", 200};
    IntSetting exposureMode{"exposure-mode", 1};
    IntSetting horizontalFOV{"horizontal-fov", 75};

    RaspiCameraConfig();
};

// Replace with varidic function/template?
void parseConfigs(std::vector<std::unique_ptr<Config>> &configs);
void parseConfigs(std::vector<std::unique_ptr<Config>> &configs, std::string string);
