:start
    C:\gstreamer\1.0\x86_64\bin\gst-launch-1.0.exe -e udpsrc port=1181 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoscale ! video/x-raw,width=640,height=480 ! autovideosink
    goto :start
