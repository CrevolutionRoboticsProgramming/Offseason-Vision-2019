#include "Config.hpp"

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

SystemConfig::SystemConfig()
{
    label = "SYSTEM";

    settings.push_back(std::unique_ptr<Setting>{std::move(&verbose)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&tuning)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&address)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&videoPort)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&sendPort)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&receivePort)});
}

VisionConfig::VisionConfig()
{
    label = "VISION";

    settings.push_back(std::unique_ptr<Setting>{std::move(&lowHue)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&lowSaturation)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&lowValue)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&highHue)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&highSaturation)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&highValue)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&erosionDilationPasses)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&minArea)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&minRotation)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&allowableError)});
}

UVCCameraConfig::UVCCameraConfig()
{
    label = "UVCCAM";

    settings.push_back(std::unique_ptr<Setting>{std::move(&width)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&height)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&fps)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&exposure)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&exposureAuto)});
}

RaspiCameraConfig::RaspiCameraConfig()
{
    label = "RASPICAM";

    settings.push_back(std::unique_ptr<Setting>{std::move(&width)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&height)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&fps)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&shutterSpeed)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&exposureMode)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&horizontalFOV)});
}

void parseConfigs(std::vector<std::unique_ptr<Config>> &configs)
{
    std::ifstream file{"config"};
    std::stringstream stringStream;
    if (file.is_open())
    {
        stringStream << file.rdbuf();
        parseConfigs(configs, stringStream.str());
    }
    else
    {
        std::cout << "Failed to open configuration file\n";
    }
}

void parseConfigs(std::vector<std::unique_ptr<Config>> &configs, std::string string)
{
    std::vector<bool> parsedConfigs;
    parsedConfigs.resize(configs.size());

    for (int c{0}; c < configs.size(); ++c)
    {
        if (string.find(configs.at(c)->label + ":") != std::string::npos)
        {
            if (parsedConfigs.at(c))
                std::cout << "Config " + configs.at(c)->label + " was parsed twice\n";

            for (std::string pair : split(string, ";"))
            {
                std::string setting = split(pair, ":").at(0);
                std::string value = split(pair, ":").at(1);

                for (int s = 0; s < configs.at(c)->settings.size(); ++s)
                {
                    if (configs.at(c)->settings.at(s)->label == setting)
                    {
                        configs.at(c)->settings.at(s)->setValue(value);
                    }
                }
            }

            parsedConfigs.at(c) = true;
        }
    }

    for (int c{0}; c < parsedConfigs.size(); ++c)
    {
        if (!parsedConfigs.at(c))
            std::cout << "Config " + configs.at(c)->label + " was not found\n";
    }
}
