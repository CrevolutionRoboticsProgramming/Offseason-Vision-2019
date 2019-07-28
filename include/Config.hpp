#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <functional>

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

struct Config
{
    std::string label{"UNLABELED"};
    virtual void parse(std::string string)
    {
    }
};

struct SystemConfig : public Config
{
    bool verbose{false};
    bool tuning{false};
    std::string address{"127.0.0.1"};
    int videoPort{1181};
    int sendPort{1182};
    int receivePort{1183};

    SystemConfig()
    {
        label = "SYSTEM:";
    }

    void parse(std::string string)
    {
        for (std::string pair : split(string, ";"))
        {
            std::string setting = split(pair, ":").at(0);
            std::string value = split(pair, ":").at(1);

            if (setting == "verbose")
                verbose = value == "true";
            else if (setting == "tuning")
                tuning = value == "true";
            else if (setting == "video-port")
                videoPort = std::stoi(value);
            else if (setting == "send-port")
                sendPort = std::stoi(value);
            else if (setting == "receive-port")
                receivePort = std::stoi(value);
        }
    }
};

struct VisionConfig : public Config
{
    int lowHue{0};
    int lowSaturation{0};
    int lowValue{0};
    int highHue{255};
    int highSaturation{255};
    int highValue{255};

    int erosionDilationPasses{1};
    int minArea{0};
    int minRotation{0};
    int allowableError{10000000};

    VisionConfig()
    {
        label = "VISION:";
    }

    void parse(std::string string)
    {
        for (std::string pair : split(string, ";"))
        {
            std::string setting = split(pair, ":").at(0);
            std::string value = split(pair, ":").at(1);

            if (setting == "low-hue")
                lowHue = std::stoi(value);
            else if (setting == "high-hue")
                highHue = std::stoi(value);
            else if (setting == "low-saturation")
                lowSaturation = std::stoi(value);
            else if (setting == "high-saturation")
                highSaturation = std::stoi(value);
            else if (setting == "low-value")
                lowValue = std::stoi(value);
            else if (setting == "high-value")
                highValue = std::stoi(value);
            else if (setting == "erosion-dilation-passes")
                erosionDilationPasses = std::stoi(value);
            else if (setting == "min-area")
                minArea = std::stoi(value);
            else if (setting == "min-rotation")
                minRotation = std::stoi(value);
            else if (setting == "allowable-error")
                allowableError = std::stoi(value);
        }
    }
};

struct UVCCameraConfig : public Config
{
    int width{320};
    int height{240};
    int fps{15};
    int exposure{0};
    int exposure_auto{1};

    UVCCameraConfig()
    {
        label = "UVCCAM:";
    }

    void parse(std::string string)
    {
        for (std::string pair : split(string, ";"))
        {
            std::string setting = split(pair, ":").at(0);
            std::string value = split(pair, ":").at(1);

            if (setting == "width")
                width = std::stoi(value);
            else if (setting == "height")
                height = std::stoi(value);
            else if (setting == "fps")
                fps = std::stoi(value);
            else if (setting == "exposure")
                exposure = std::stoi(value);
            else if (setting == "exposure_auto")
                exposure_auto = std::stoi(value);
        }
    }
};

struct RaspiCameraConfig : public Config
{
    int width{320};
    int height{240};
    int fps{15};
    int shutter_speed{200};
    int exposure_mode{1};
    int horizontalFOV{75};

    RaspiCameraConfig()
    {
        label = "RASPICAM:";
    }

    void parse(std::string string)
    {
        for (std::string pair : split(string, ";"))
        {
            std::string setting = split(pair, ":").at(0);
            std::string value = split(pair, ":").at(1);

            if (setting == "width")
                width = std::stoi(value);
            else if (setting == "height")
                height = std::stoi(value);
            else if (setting == "fps")
                fps = std::stoi(value);
            else if (setting == "shutter_speed")
                shutter_speed = std::stoi(value);
            else if (setting == "exposure_mode")
                exposure_mode = std::stoi(value);
            else if (setting == "horizontal-fov")
                horizontalFOV = std::stoi(value);
        }
    }
};

// Replace with varidic function/template?
void parseConfigs(std::vector<Config *> &configs)
{
    std::vector<bool> parsedConfigs;
    parsedConfigs.resize(configs.size());

    std::ifstream file{"config"};
    if (file.is_open())
    {
        std::string line{};
        while (std::getline(file, line))
        {
            for (int i{0}; i < configs.size(); ++i)
            {
                if (line.find(configs.at(i)->label) != std::string::npos)
                {
                    if (parsedConfigs.at(i))
                        std::cout << "Config " + configs.at(i)->label + " was parsed twice\n";
                    configs.at(i)->parse(line);
                    parsedConfigs.at(i) = true;
                    break;
                }
            }
        }

        for (int i{0}; i < configs.size(); ++i)
        {
            if (!parsedConfigs.at(i))
                std::cout << "Config " + configs.at(i)->label + " was not found\n";
        }
    }
    else
    {
        std::cout << "Failed to open configuration file\n";
    }
}