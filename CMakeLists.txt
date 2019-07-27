cmake_minimum_required(VERSION 2.8)
project( NewVision )

set( BOOST_ROOT ~/boost_1_58_0 )
set( OpenCV_DIR ~/opencv-2.4.13.6 )
add_definitions( -std=c++17 )

set( OpenCV_FOUND 1 )

find_package( PkgConfig )
find_package( OpenCV 2.4.13.6 REQUIRED )
find_package( Boost COMPONENTS system REQUIRED )
pkg_check_modules( GST REQUIRED gstreamer-1.0>=1.4
                               gstreamer-sdp-1.0>=1.4
                               gstreamer-video-1.0>=1.4
                               gstreamer-app-1.0>=1.4 )

INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} ${GST_INCLUDE_DIRS} /usr/include/gstreamer-1.0 include )

file (GLOB NewVision_SRC
    "main.cpp"
    "http/server/*.cpp"
    "opencv-code/*.cpp"
)

add_executable( NewVision ${NewVision_SRC} )
target_link_libraries( NewVision ${OpenCV_LIBS} ${Boost_LIBRARIES} ${GST_LIBRARIES} gstapp-1.0 gstriff-1.0 gstbase-1.0 gstvideo-1.0 gstpbutils-1.0 X11 pthread )

