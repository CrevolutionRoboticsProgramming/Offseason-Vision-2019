#include <iostream>
#include <string>
#include <fstream>
#include <functional>
#include <opencv2/opencv.hpp>
#include <boost/asio.hpp>
#include "Config.hpp"

SystemConfig systemConfig{};
VisionConfig visionConfig{};
UVCCameraConfig uvcCameraConfig{};
RaspiCameraConfig raspiCameraConfig{};
std::vector<Config *> configs{&systemConfig,
                              &visionConfig,
                              &uvcCameraConfig,
                              &raspiCameraConfig};

std::string fileDirectory{"/home/pi"};
std::string fileName{"image.jpg"};
std::string address{"localhost"};
int port{8080};

int main()
{
    parseConfigs(configs);

    // Camera setup
    char buffer[1000];
    sprintf(buffer, "rpicamsrc shutter-speed=%d exposure-mode=%d ! video/x-raw,width=%d,height=%d,framerate=%d/1 ! appsink",
            raspiCameraConfig.shutter_speed, raspiCameraConfig.exposure_mode, raspiCameraConfig.width, raspiCameraConfig.height, raspiCameraConfig.fps);
    cv::VideoCapture processingCamera(buffer, CV_CAP_GSTREAMER);
    sprintf(buffer, "v4l2src /dev/video0 ! video/x-raw,width=%d,height=%d,framerate=%d/1 ! appsink",
            uvcCameraConfig.width, uvcCameraConfig.height, uvcCameraConfig.fps);
    //cv::VideoCapture viewingCamera(buffer, CV_CAP_GSTREAMER);
    if (uvcCameraConfig.exposure != 0 && uvcCameraConfig.exposure_auto != 1)
    {
        sprintf(buffer, "v4l2-ctl -c exposure_auto=%d -c exposure_absolute=%d",
                uvcCameraConfig.exposure_auto, uvcCameraConfig.exposure);
    }
    else
    {
        sprintf(buffer, "v4l2-ctl -c exposure_absolute=%d",
                uvcCameraConfig.exposure);
    }
    system(buffer);

    // Start streaming
    sprintf(buffer, "LD_LIBRARY_PATH=/usr/local/lib mjpg_streamer -i \"input_file.so -f %d -n %s -d 0\" -o \"output_http.so -w /tmp -p %d\" &",
            fileDirectory, fileName, port);
    system(buffer);

    sprintf(buffer, "appsrc ! video/x-raw,width=%d,height=%d,framerate=%d/1 ! videoconvert ! video/x-raw,format=I420 ! jpegenc ! multifilesink location=%s max-files=1",
            uvcCameraConfig.width, uvcCameraConfig.height, uvcCameraConfig.fps, (fileDirectory + '/' + fileName));
    cv::VideoWriter writer{buffer, CV_CAP_GSTREAMER, uvcCameraConfig.fps, cv::Size{uvcCameraConfig.width, uvcCameraConfig.height}};

    while (true)
    {
        cv::Mat processingFrame, viewingFrame;
        if (!processingCamera.grab() /* || !viewingCamera.grab()*/)
            continue;
        processingCamera.read(processingFrame);
        //viewingCamera.read(viewingFrame);
        if (processingFrame.empty() || viewingFrame.empty())
            continue;

        // Writes frame to be streamed
        cv::line(viewingFrame, cv::Point{uvcCameraConfig.width / 2, 0}, cv::Point{uvcCameraConfig.width / 2, uvcCameraConfig.height}, cv::Scalar{0, 0, 0});
        writer.write(viewingFrame);

        cv::cvtColor(processingFrame, processingFrame, cv::COLOR_BGR2HSV);
        //cv::inRange(processingFrame, )

        cv::imshow("Processing Frame", processingFrame);
        //cv::imshow("Viewing Frame", viewingFrame);
        cv::waitKey(1);
    }

    return 0;
}