#include "Config.hpp"

SystemConfig::SystemConfig()
{
    label = "SYSTEM";

    settings.push_back(std::unique_ptr<Setting>{std::move(&verbose)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&tuning)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&address)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&videoPort)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&communicatorPort)});
    settings.push_back(std::unique_ptr<Setting>{std::move(&robotPort)});
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
    settings.push_back(std::unique_ptr<Setting>{std::move(&maxArea)});
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

    for (std::string line : split(string, "\n"))
    {
        for (int c{0}; c < configs.size(); ++c)
        {
            if (line.find(configs.at(c)->label + ":") != std::string::npos)
            {
                line = line.substr((configs.at(c)->label + ":").length());

                if (parsedConfigs.at(c))
                    std::cout << "Config " + configs.at(c)->label + " was parsed twice\n";

                std::vector<bool> parsedSettings;
                parsedSettings.resize(configs.at(c)->settings.size());
                for (std::string pair : split(line, ";"))
                {
                    if (split(pair, "=").size() != 2)
                        continue;

                    std::string setting = split(pair, "=").at(0);
                    std::string value = split(pair, "=").at(1);

                    for (int s = 0; s < configs.at(c)->settings.size(); ++s)
                    {
                        if (configs.at(c)->settings.at(s)->label == setting)
                        {
                            if (configs.at(c)->settings.at(s)->setValue(value))
                                parsedSettings.at(s) = true;
                            else
                                std::cout << "New value " + value + " for setting " + configs.at(c)->settings.at(s)->label + " in config " + configs.at(c)->label + " is improperly formatted; disregarding\n";
                        }
                    }
                }

                for (int s{0}; s < configs.at(c)->settings.size(); ++s)
                {
                    if (!parsedSettings.at(s))
                        std::cout << "Setting " + configs.at(c)->settings.at(s)->label + " of config " + configs.at(c)->label + " was not found\n";
                }

                parsedConfigs.at(c) = true;
            }
        }
    }

    for (int c{0}; c < parsedConfigs.size(); ++c)
    {
        if (!parsedConfigs.at(c))
            std::cout << "Config " + configs.at(c)->label + " was not found\n";
    }
}

void writeConfigs(std::vector<std::unique_ptr<Config>> &configs)
{
    remove("config");
    std::ofstream file;
    file.open("config");

    if (!file.is_open())
        std::cout << "Failed to open configuration file\n";

    for (int c{0}; c < configs.size(); ++c)
    {
        file << configs.at(c)->label << ':';
        for (int s{0}; s < configs.at(c)->settings.size(); ++s)
        {
            file << configs.at(c)->settings.at(s)->label << '=' << configs.at(c)->settings.at(s)->toString() << ';';
        }
        file << '\n';
    }

    file.close();
}
