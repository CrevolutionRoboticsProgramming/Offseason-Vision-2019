while true
do
    gst-launch-1.0 udpsrc port=1181 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoscale ! video/x-raw,width=640,height=480 ! autovideosink
done
