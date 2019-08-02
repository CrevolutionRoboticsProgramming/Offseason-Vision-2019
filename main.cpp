#include <iostream>
#include <string>
#include <fstream>
#include <functional>
#include <opencv2/opencv.hpp>
#include <boost/asio.hpp>
#include "Config.hpp"
#include "Contour.hpp"
#include "UDPHandler.hpp"

int main()
{
    SystemConfig systemConfig{};
    VisionConfig visionConfig{};
    UVCCameraConfig uvcCameraConfig{};
    RaspiCameraConfig raspiCameraConfig{};

    std::vector<std::unique_ptr<Config>> configs;
    configs.push_back(std::unique_ptr<Config>{std::move(&systemConfig)});
    configs.push_back(std::unique_ptr<Config>{std::move(&visionConfig)});
    configs.push_back(std::unique_ptr<Config>{std::move(&uvcCameraConfig)});
    configs.push_back(std::unique_ptr<Config>{std::move(&raspiCameraConfig)});

    cv::Mat morphElement{cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3))};

    std::string fileDirectory{"/home/pi"};
    std::string fileName{"image.jpg"};

    parseConfigs(configs);

    // Camera setup
    std::string buffer{};
    buffer = "rpicamsrc shutter-speed=" + std::to_string(raspiCameraConfig.shutterSpeed.value) + " exposure-mode=" + std::to_string(raspiCameraConfig.exposureMode.value) + " ! video/x-raw,width=" + std::to_string(raspiCameraConfig.width.value) + ",height=" + std::to_string(raspiCameraConfig.height.value) + ",framerate=" + std::to_string(raspiCameraConfig.fps.value) + "/1 ! appsink";
    cv::VideoCapture processingCamera(buffer, cv::CAP_GSTREAMER);

    buffer = "v4l2src device=/dev/video0 ! video/x-raw,width=" + std::to_string(uvcCameraConfig.width.value) + ",height=" + std::to_string(uvcCameraConfig.height.value) + " ! videoconvert ! videorate ! video/x-raw,format=BGR,framerate=" + std::to_string(uvcCameraConfig.fps.value) + "/1 ! appsink";
    cv::VideoCapture viewingCamera(buffer, cv::CAP_GSTREAMER);

    if (uvcCameraConfig.exposure.value != 0 && uvcCameraConfig.exposureAuto.value != 1)
    {
        buffer = "v4l2-ctl -c exposure_auto=" + std::to_string(uvcCameraConfig.exposureAuto.value) + " -c exposure_absolute=" + std::to_string(uvcCameraConfig.exposure.value);
    }
    else
    {
        buffer = "v4l2-ctl -c exposure_auto=" + std::to_string(uvcCameraConfig.exposureAuto.value);
    }
    system(buffer.c_str());

    // Start streaming
    buffer = "LD_LIBRARY_PATH=/usr/local/lib mjpg_streamer -i \"input_file.so -f " + fileDirectory + " -n " + fileName + " -d 0\" -o \"output_http.so -w /tmp -p " + std::to_string(systemConfig.videoPort.value) + "\" &",
    system(buffer.c_str());

    buffer = "appsrc ! video/x-raw,width=" + std::to_string(uvcCameraConfig.width.value) + ",height=" + std::to_string(uvcCameraConfig.height.value) + ",framerate=" + std::to_string(uvcCameraConfig.fps.value) + "/1 ! videoconvert ! video/x-raw,format=I420 ! jpegenc ! multifilesink location=" + fileDirectory + '/' + fileName + " max-files=1";
    cv::VideoWriter writer{buffer, cv::CAP_GSTREAMER, uvcCameraConfig.fps.value, cv::Size{uvcCameraConfig.width.value, uvcCameraConfig.height.value}};

    UDPHandler udpHandler{systemConfig.address.value, systemConfig.sendPort.value, systemConfig.receivePort.value};

    bool streamUVC{true};
    while (true)
    {
        cv::Mat processingFrame, viewingFrame;
        if (!processingCamera.grab() || !viewingCamera.grab())
            continue;
        processingCamera.read(processingFrame);
        viewingCamera.read(viewingFrame);
        if (processingFrame.empty() || viewingFrame.empty())
            continue;

        if (udpHandler.getMessage() != "")
        {
            std::string configsLabel{"CONFIGS:"};

            if (udpHandler.getMessage().find(configsLabel) != std::string::npos)
            {
                parseConfigs(configs, udpHandler.getMessage().substr(configsLabel.length()));
            }
            else if (udpHandler.getMessage() == "get config")
            {
                std::string configString{"CONFIGS:"};
                for (int c{0}; c < configs.size(); ++c)
                {
                    configString += '\n' + configs.at(c)->label;
                    for (int s{0}; s < configs.at(c)->settings.size(); ++s)
                    {
                        configString += configs.at(c)->settings.at(s)->label + "=" + configs.at(c)->settings.at(s)->asString() + ";";
                    }
                }
                udpHandler.send(configString);
            }
            else if (udpHandler.getMessage() == "switch camera")
            {
                streamUVC = !streamUVC;
            }
            else if (udpHandler.getMessage() == "reboot")
            {
                system("sudo reboot -h now");
            }
        }

        //Writes frame to be streamed
        if (streamUVC)
        {
            cv::line(viewingFrame, cv::Point{uvcCameraConfig.width.value / 2, 0}, cv::Point{uvcCameraConfig.width.value / 2, uvcCameraConfig.height.value}, cv::Scalar{0, 0, 0});
            writer.write(viewingFrame);
        }
        else if (!systemConfig.tuning.value)
        {
            writer.write(processingFrame);
        }

        //Extracts the contours
        std::vector<std::vector<cv::Point>> rawContours;
        std::vector<Contour> contours;
        cv::cvtColor(processingFrame, processingFrame, cv::COLOR_BGR2HSV);
        cv::inRange(processingFrame, cv::Scalar{visionConfig.lowHue.value, visionConfig.lowSaturation.value, visionConfig.lowValue.value}, cv::Scalar{visionConfig.highHue.value, visionConfig.highSaturation.value, visionConfig.highValue.value}, processingFrame);
        cv::erode(processingFrame, processingFrame, morphElement, cv::Point(-1, -1), 2);
        cv::dilate(processingFrame, processingFrame, morphElement, cv::Point(-1, -1), 2);

        if (!streamUVC && systemConfig.tuning.value)
            writer.write(processingFrame);

        cv::Canny(processingFrame, processingFrame, 0, 0);
        cv::findContours(processingFrame, rawContours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

        for (std::vector<cv::Point> pointsVector : rawContours)
        {
            if (Contour{pointsVector}.isValid(visionConfig.minArea.value, visionConfig.minRotation.value, visionConfig.allowableError.value))
                contours.push_back(Contour{pointsVector});
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
                        //We test if it's closer to the original contour after checking if the
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
        double centerX{((std::max(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x) - std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)) / 2) + std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)};
        //double centerY{((std::max(closestPair.at(0).rotatedBoundingBox.center.y, closestPair.at(1).rotatedBoundingBox.center.y) - std::min(closestPair.at(0).rotatedBoundingBox.center.y, closestPair.at(1).rotatedBoundingBox.center.y)) / 2) + std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.y)};

        //The original contour will always be the left one since that's what we've specified
        double horizontalAngleError = -((processingFrame.cols / 2.0) - centerX) / processingFrame.cols * raspiCameraConfig.horizontalFOV.value;

        udpHandler.send(std::to_string(horizontalAngleError));
    }

    return 0;
}
