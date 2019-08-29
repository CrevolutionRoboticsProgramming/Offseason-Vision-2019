#include <iostream>
#include <string>
#include <fstream>
#include <functional>
#include <opencv2/opencv.hpp>
#include <boost/asio.hpp>
#include "Config.hpp"
#include "Contour.hpp"
#include "UDPHandler.hpp"

std::vector<std::unique_ptr<Config>> configs;
SystemConfig systemConfig{};
VisionConfig visionConfig{};
UVCCameraConfig uvcCameraConfig{};
RaspiCameraConfig raspiCameraConfig{};

bool streamUVC{true};

cv::VideoWriter streamer;

void stream()
{
    cv::VideoCapture viewingCamera{"/dev/video0", cv::CAP_V4L2};
    viewingCamera.set(cv::CAP_PROP_FRAME_WIDTH, uvcCameraConfig.width.value);
    viewingCamera.set(cv::CAP_PROP_FRAME_HEIGHT, uvcCameraConfig.height.value);
    viewingCamera.set(cv::CAP_PROP_FPS, uvcCameraConfig.fps.value);
    viewingCamera.set(cv::CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));

    cv::Mat viewingFrame;
    while (true)
    {
        if (streamUVC)
        {
            if (viewingCamera.grab())
                viewingCamera.read(viewingFrame);
            else
                continue;

            cv::line(viewingFrame, cv::Point{uvcCameraConfig.width.value / 2, 0}, cv::Point{uvcCameraConfig.width.value / 2, uvcCameraConfig.height.value}, cv::Scalar{0, 0, 0});
            streamer.write(viewingFrame);
        }
    }
}

void handleCommunicatorUDP()
{
    UDPHandler communicatorUDPHandler{systemConfig.address.value, systemConfig.communicatorPort.value, systemConfig.receivePort.value};
    while (true)
    {
        if (communicatorUDPHandler.getMessage() != "")
        {
            std::string configsLabel{"CONFIGS:"};

            // If we were sent configs
            if (communicatorUDPHandler.getMessage().find(configsLabel) != std::string::npos)
            {
                parseConfigs(configs, communicatorUDPHandler.getMessage().substr(configsLabel.length()));

                writeConfigs(configs);

                if (systemConfig.verbose.value)
                    std::cout << "Updated Configurations\n";
            }
            else if (communicatorUDPHandler.getMessage() == "get config")
            {
                std::string configString{"CONFIGS:"};
                for (int c{0}; c < configs.size(); ++c)
                {
                    configString += '\n' + configs.at(c)->label + ':';
                    for (int s{0}; s < configs.at(c)->settings.size(); ++s)
                    {
                        configString += configs.at(c)->settings.at(s)->label + "=" + configs.at(c)->settings.at(s)->asString() + ";";
                    }
                }
                communicatorUDPHandler.send(configString);

                if (systemConfig.verbose.value)
                    std::cout << "Sent Configurations\n";
            }
            else if (communicatorUDPHandler.getMessage() == "switch camera")
            {
                streamUVC = !streamUVC;

                if (systemConfig.verbose.value)
                    std::cout << "Switched Camera Stream\n";
            }
            else if (communicatorUDPHandler.getMessage() == "restart program")
            {
                if (systemConfig.verbose.value)
                    std::cout << "Restarting program...\n";

                system("sudo pkill Offseason-Visio");
            }
            else if (communicatorUDPHandler.getMessage() == "reboot")
            {
                if (systemConfig.verbose.value)
                    std::cout << "Rebooting...\n";

                system("sudo reboot -h now");
            }
            else
            {
                std::cout << "Received unknown command via UDP\n";
            }

            communicatorUDPHandler.clearMessage();
        }
    }
}

int main()
{
    configs.push_back(std::unique_ptr<Config>{std::move(&systemConfig)});
    configs.push_back(std::unique_ptr<Config>{std::move(&visionConfig)});
    configs.push_back(std::unique_ptr<Config>{std::move(&uvcCameraConfig)});
    configs.push_back(std::unique_ptr<Config>{std::move(&raspiCameraConfig)});

    cv::Mat morphElement{cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3))};

    parseConfigs(configs);

    if (systemConfig.verbose.value)
        std::cout << "Parsed Configs\n";

    // Begins monitoring for messages from the VisionCommunicator on a separate thread so it doesn't stop if the other threads do
    std::thread handleCommunicatorUDPThread(&handleCommunicatorUDP);

    std::string streamingPipeline = "appsrc ! videoconvert ! video/x-raw,format=YUY2 ! jpegenc ! rtpjpegpay ! udpsink host=" + systemConfig.address.value + " port=" + std::to_string(systemConfig.videoPort.value);
    streamer = cv::VideoWriter{streamingPipeline, cv::CAP_GSTREAMER, 0, 120, cv::Size{uvcCameraConfig.width.value, uvcCameraConfig.height.value}};

    // Camera setup
    std::string buffer{};
    if (uvcCameraConfig.exposure.value != 0 && uvcCameraConfig.exposureAuto.value != 1)
    {
        buffer = "v4l2-ctl -c exposure_auto=" + std::to_string(uvcCameraConfig.exposureAuto.value) + " -c exposure_absolute=" + std::to_string(uvcCameraConfig.exposure.value);
    }
    else
    {
        buffer = "v4l2-ctl -c exposure_auto=" + std::to_string(uvcCameraConfig.exposureAuto.value);
    }
    system(buffer.c_str());

    if (systemConfig.verbose.value)
        std::cout << "Configured Exposure\n";

    // Begins streaming on a separate thread to shorten delays between frames
    std::thread streamThread{&stream};

    buffer = "rpicamsrc shutter-speed=" + std::to_string(raspiCameraConfig.shutterSpeed.value) + " exposure-mode=" + std::to_string(raspiCameraConfig.exposureMode.value) + " ! video/x-raw,width=" + std::to_string(raspiCameraConfig.width.value) + ",height=" + std::to_string(raspiCameraConfig.height.value) + ",framerate=" + std::to_string(raspiCameraConfig.fps.value) + "/1 ! appsink";
    cv::VideoCapture processingCamera(buffer, cv::CAP_GSTREAMER);

    UDPHandler robotUDPHandler{"10.28.51.2", systemConfig.robotPort.value, 9999};

    cv::Mat streamFrame;
    while (true)
    {
        cv::Mat processingFrame;

        if (processingCamera.grab())
            processingCamera.read(processingFrame);
        else
            continue;

        if (processingFrame.empty())
            continue;

        if (systemConfig.verbose.value)
            std::cout << "Grabbed Frame\n";

        //Writes frame to be streamed when not tuning
        if (!streamUVC && !systemConfig.tuning.value)
            streamer.write(processingFrame);

        //Extracts the contours
        std::vector<std::vector<cv::Point>> rawContours;
        std::vector<Contour> contours;
        cv::cvtColor(processingFrame, processingFrame, cv::COLOR_BGR2HSV);
        cv::inRange(processingFrame, cv::Scalar{visionConfig.lowHue.value, visionConfig.lowSaturation.value, visionConfig.lowValue.value}, cv::Scalar{visionConfig.highHue.value, visionConfig.highSaturation.value, visionConfig.highValue.value}, processingFrame);
        cv::erode(processingFrame, processingFrame, morphElement, cv::Point(-1, -1), 2);
        cv::dilate(processingFrame, processingFrame, morphElement, cv::Point(-1, -1), 2);

        //Writes frame to be streamed when tuning
        if (!streamUVC && systemConfig.tuning.value)
            streamer.write(streamFrame);

        processingFrame.copyTo(streamFrame);
        cv::cvtColor(streamFrame, streamFrame, cv::COLOR_GRAY2BGR);

        cv::Canny(processingFrame, processingFrame, 0, 0);
        cv::findContours(processingFrame, rawContours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

        for (std::vector<cv::Point> pointsVector : rawContours)
        {
            Contour newContour{pointsVector};
            if (newContour.isValid(visionConfig.minArea.value, visionConfig.minRotation.value, visionConfig.allowableError.value))
            {
                contours.push_back(newContour);
            }
        }

        std::vector<std::array<Contour, 2>> pairs{};

        //Least distant contour initialized with -1 so it's not confused for an actual contour and can be tested for not being valid
        int leastDistantContour{-1};

        //Now that we've identified compliant targets, we find their match (if they have one)
        for (int origContour{0}; origContour < contours.size(); ++origContour)
        {
            //We identify the left one first because why not
            if (contours.at(origContour).angle > 0)
            {
                //Iterates through all of the contours and compares them against the original
                for (int compareContour{0}; compareContour < contours.size(); ++compareContour)
                {
                    //If the contour to compare against isn't the original
                    // and the contour is angled left
                    // and the contour is right of the original
                    // and (if the least distant contour hasn't been set
                    // OR this contour is closer than the last least distant contour)
                    // then this contour is the new least distant contour
                    if (compareContour != origContour && contours.at(compareContour).angle < 0 && contours.at(origContour).rotatedBoundingBoxPoints[0].x < contours.at(compareContour).rotatedBoundingBoxPoints[0].x)
                    {
                        //We viewingCamera if it's closer to the original contour after checking if the
                        // index is negative since passing a negative number to a vector will
                        // throw an OutOfBounds exception
                        if (leastDistantContour == -1)
                        {
                            leastDistantContour = compareContour;
                        }
                        else if (contours.at(compareContour).rotatedBoundingBoxPoints[0].x - contours.at(origContour).rotatedBoundingBoxPoints[0].x < contours.at(leastDistantContour).rotatedBoundingBoxPoints[0].x)
                        {
                            leastDistantContour = compareContour;
                        }
                    }
                }

                //If we found the second contour, add the pair to the list
                if (leastDistantContour != -1)
                {
                    pairs.push_back(std::array<Contour, 2>{contours.at(origContour), contours.at(leastDistantContour)});
                    break;
                }
            }
        }

        if (pairs.size() == 0)
            continue;

        std::array<Contour, 2> closestPair{pairs.back()};
        for (int p{0}; p < pairs.size(); ++p)
        {
            double comparePairCenter{((std::max(pairs.at(p).at(0).rotatedBoundingBox.center.x, pairs.at(p).at(1).rotatedBoundingBox.center.x) - std::min(pairs.at(p).at(0).rotatedBoundingBox.center.x, pairs.at(p).at(1).rotatedBoundingBox.center.x)) / 2) + std::min(pairs.at(p).at(0).rotatedBoundingBox.center.x, pairs.at(p).at(1).rotatedBoundingBox.center.x)};
            double closestPairCenter{((std::max(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x) - std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)) / 2) + std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)};

            if (std::abs(comparePairCenter) - (raspiCameraConfig.width.value / 2) <
                std::abs(closestPairCenter) - (raspiCameraConfig.width.value / 2))
            {
                closestPair = std::array<Contour, 2>{pairs.at(p).at(0), pairs.at(p).at(1)};
            }
        }

        //For clarity
        double centerX{closestPair.at(0).rotatedBoundingBox.center.x + ((closestPair.at(1).rotatedBoundingBox.center.x - closestPair.at(0).rotatedBoundingBox.center.x) / 2)};
        double centerY{closestPair.at(0).rotatedBoundingBox.center.y + ((closestPair.at(1).rotatedBoundingBox.center.y - closestPair.at(0).rotatedBoundingBox.center.y) / 2)};

        double horizontalAngleError{-((processingFrame.cols / 2.0) - centerX) / processingFrame.cols * raspiCameraConfig.horizontalFOV.value};

        robotUDPHandler.send(std::to_string(horizontalAngleError));

        //Preps frame to be streamed
        if (!streamUVC && systemConfig.tuning.value)
        {
            cv::rectangle(streamFrame, closestPair.at(0).boundingBox, cv::Scalar{0, 127.5, 255}, 2);
            cv::rectangle(streamFrame, closestPair.at(1).boundingBox, cv::Scalar{0, 127.5, 255}, 2);
            cv::rectangle(streamFrame, cv::Rect{cv::Point2i{std::min(closestPair.at(0).boundingBox.x, closestPair.at(1).boundingBox.x), std::min(closestPair.at(0).boundingBox.y, closestPair.at(1).boundingBox.y)}, cv::Point2i{std::max(closestPair.at(0).boundingBox.x + closestPair.at(0).boundingBox.width, closestPair.at(1).boundingBox.x + closestPair.at(1).boundingBox.width), std::max(closestPair.at(0).boundingBox.y + closestPair.at(0).boundingBox.height, closestPair.at(1).boundingBox.y + closestPair.at(1).boundingBox.height)}}, cv::Scalar{0, 255, 0}, 2);
            cv::line(streamFrame, cv::Point{centerX, centerY - 10}, cv::Point{centerX, centerY + 10}, cv::Scalar{0, 255, 0}, 2);
            cv::line(streamFrame, cv::Point{centerX - 10, centerY}, cv::Point{centerX + 10, centerY}, cv::Scalar{0, 255, 0}, 2);
            cv::putText(streamFrame, "Horizontal Angle of Error: " + std::to_string(horizontalAngleError), cv::Point{0, 10}, cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar{255, 255, 255});
        }
    }

    return 0;
}
