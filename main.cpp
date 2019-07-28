#include <iostream>
#include <string>
#include <fstream>
#include <functional>
#include <opencv2/opencv.hpp>
#include <boost/asio.hpp>
#include "Config.hpp"
#include "Contour.hpp"
#include "UDPHandler.hpp"

SystemConfig systemConfig{};
VisionConfig visionConfig{};
UVCCameraConfig uvcCameraConfig{};
RaspiCameraConfig raspiCameraConfig{};
std::vector<Config *> configs{&systemConfig,
                              &visionConfig,
                              &uvcCameraConfig,
                              &raspiCameraConfig};

cv::Mat morphElement{cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3))};

std::string fileDirectory{"/home/pi"};
std::string fileName{"image.jpg"};

int main()
{
    parseConfigs(configs);

    // Camera setup
    char buffer[1000];
    sprintf(buffer, "rpicamsrc shutter-speed=%d exposure-mode=%d ! video/x-raw,width=%d,height=%d,framerate=%d/1 ! appsink",
            raspiCameraConfig.shutter_speed, raspiCameraConfig.exposure_mode, raspiCameraConfig.width, raspiCameraConfig.height, raspiCameraConfig.fps);
    cv::VideoCapture processingCamera(buffer, cv::CAP_GSTREAMER);
    sprintf(buffer, "v4l2src device=/dev/video0 ! video/x-raw,width=%d,height=%d ! videoconvert ! videorate ! video/x-raw,format=BGR,framerate=%d/1 ! appsink",
            uvcCameraConfig.width, uvcCameraConfig.height, uvcCameraConfig.fps);
    cv::VideoCapture viewingCamera(buffer, cv::CAP_GSTREAMER);
    if (uvcCameraConfig.exposure != 0 && uvcCameraConfig.exposure_auto != 1)
    {
        sprintf(buffer, "v4l2-ctl -c exposure_auto=%d -c exposure_absolute=%d",
                uvcCameraConfig.exposure_auto, uvcCameraConfig.exposure);
    }
    else
    {
        sprintf(buffer, "v4l2-ctl -d /dev/video0 -c exposure_auto=%d",
                uvcCameraConfig.exposure_auto);
    }
    system(buffer);

    // Start streaming
    sprintf(buffer, "LD_LIBRARY_PATH=/usr/local/lib mjpg_streamer -i \"input_file.so -f %d -n %s -d 0\" -o \"output_http.so -w /tmp -p %d\" &",
            fileDirectory, fileName, systemConfig.videoPort);
    //system(buffer);

    sprintf(buffer, "appsrc ! video/x-raw,width=%d,height=%d,framerate=%d/1 ! videoconvert ! video/x-raw,format=I420 ! jpegenc ! multifilesink location=%s max-files=1",
            uvcCameraConfig.width, uvcCameraConfig.height, uvcCameraConfig.fps, (fileDirectory + '/' + fileName));
    cv::VideoWriter writer{buffer, cv::CAP_GSTREAMER, uvcCameraConfig.fps, cv::Size{uvcCameraConfig.width, uvcCameraConfig.height}};

    UDPHandler udpHandler{systemConfig.address, systemConfig.sendPort, systemConfig.receivePort};

    while (true)
    {
        cv::Mat processingFrame, viewingFrame;
        while (true)
        {
            if (!processingCamera.grab() || !viewingCamera.grab())
                continue;
            processingCamera.read(processingFrame);
            viewingCamera.read(viewingFrame);
            if (processingFrame.empty() || viewingFrame.empty())
                continue;
            cv::imshow("Processing Frame", processingFrame);
            cv::imshow("Viewing Frame", viewingFrame);
            cv::waitKey(1);
        }

        //Writes frame to be streamed
        cv::line(viewingFrame, cv::Point{uvcCameraConfig.width / 2, 0}, cv::Point{uvcCameraConfig.width / 2, uvcCameraConfig.height}, cv::Scalar{0, 0, 0});
        writer.write(viewingFrame);

        //Extracts the contours
        std::vector<std::vector<cv::Point>> rawContours;
        std::vector<Contour> contours;
        cv::cvtColor(processingFrame, processingFrame, cv::COLOR_BGR2HSV);
        cv::inRange(processingFrame, cv::Scalar{visionConfig.lowHue, visionConfig.lowSaturation, visionConfig.lowValue}, cv::Scalar{visionConfig.highHue, visionConfig.highSaturation, visionConfig.highValue}, processingFrame);
        cv::erode(processingFrame, processingFrame, morphElement, cv::Point(-1, -1), 2);
        cv::dilate(processingFrame, processingFrame, morphElement, cv::Point(-1, -1), 2);
        cv::Canny(processingFrame, processingFrame, 0, 0);
        cv::findContours(processingFrame, rawContours, cv::noArray(), cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

        for (std::vector<cv::Point> pointsVector : rawContours)
        {
            if (Contour{pointsVector}.isValid(visionConfig.minArea, visionConfig.minRotation, visionConfig.allowableError))
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
        {
            continue;
        }

        std::array<Contour, 2> closestPair{pairs.back()};
        for (int p{0}; p < pairs.size(); ++p)
        {
            double comparePairCenter{((std::max(pairs.at(p).at(0).rotatedBoundingBox.center.x, pairs.at(p).at(1).rotatedBoundingBox.center.x) - std::min(pairs.at(p).at(0).rotatedBoundingBox.center.x, pairs.at(p).at(1).rotatedBoundingBox.center.x)) / 2) + std::min(pairs.at(p).at(0).rotatedBoundingBox.center.x, pairs.at(p).at(1).rotatedBoundingBox.center.x)};
            double closestPairCenter{((std::max(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x) - std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)) / 2) + std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)};

            if (std::abs(comparePairCenter) - (raspiCameraConfig.width / 2) <
                std::abs(closestPairCenter) - (raspiCameraConfig.width / 2))
            {
                closestPair = std::array<Contour, 2>{pairs.at(p).at(0), pairs.at(p).at(1)};
            }
        }

        //For clarity
        double centerX{((std::max(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x) - std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)) / 2) + std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.x)};
        //double centerY{((std::max(closestPair.at(0).rotatedBoundingBox.center.y, closestPair.at(1).rotatedBoundingBox.center.y) - std::min(closestPair.at(0).rotatedBoundingBox.center.y, closestPair.at(1).rotatedBoundingBox.center.y)) / 2) + std::min(closestPair.at(0).rotatedBoundingBox.center.x, closestPair.at(1).rotatedBoundingBox.center.y)};

        //The original contour will always be the left one since that's what we've specified
        double horizontalAngleError = -((processingFrame.cols / 2.0) - centerX) / processingFrame.cols * raspiCameraConfig.horizontalFOV;

        udpHandler.send(std::to_string(horizontalAngleError));
    }

    return 0;
}