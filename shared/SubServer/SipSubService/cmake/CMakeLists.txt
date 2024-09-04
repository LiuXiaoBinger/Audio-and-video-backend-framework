#设置编译后的可执行名称并指定变量名
SET(EXE_NAME SipSubService)
#判断当前使用的操作系统
MESSAGE(STATUS "operation system is ${CMAKE_SYSTEM}")
#在编译选项中开启所有警告和添加c++11支持
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=gnu++0x")
#在编译时使用-g选项添加调试信息 编译后的程序可以使用gbd调试
ADD_DEFINITIONS(-g)
#指定需要编译的源文件路径

INCLUDE_DIRECTORIES(../include)
AUX_SOURCE_DIRECTORY(../src SRC)
INCLUDE_DIRECTORIES(../../../3rd/include)
INCLUDE_DIRECTORIES(../../../3rd/include/pjsip)
INCLUDE_DIRECTORIES(../../../3rd/include/jrtplib3)
INCLUDE_DIRECTORIES(../../../3rd/include/jthread)
INCLUDE_DIRECTORIES(../../../3rd/include/timyxml2)
INCLUDE_DIRECTORIES(../../../3rd/include/libevent)
INCLUDE_DIRECTORIES(../../../3rd/include/mpege)

#链接库
LINK_DIRECTORIES(../../../3rd/lib)

ADD_EXECUTABLE(${EXE_NAME} ${SRC})

TARGET_LINK_LIBRARIES(${EXE_NAME} libglog.a)
TARGET_LINK_LIBRARIES(${EXE_NAME} -lpthread libgflags.a)
TARGET_LINK_LIBRARIES(${EXE_NAME} 
-lpjsua2-x86_64-unknown-linux-gnu 
-lpjsua-x86_64-unknown-linux-gnu 
-lpjsip-ua-x86_64-unknown-linux-gnu 
-lpjsip-simple-x86_64-unknown-linux-gnu 
-lpjsip-x86_64-unknown-linux-gnu 
-lpjmedia-codec-x86_64-unknown-linux-gnu 
-lpjmedia-videodev-x86_64-unknown-linux-gnu 
-lpjmedia-audiodev-x86_64-unknown-linux-gnu 
-lpjmedia-x86_64-unknown-linux-gnu 
-lpjnath-x86_64-unknown-linux-gnu 
-lpjlib-util-x86_64-unknown-linux-gnu 
-lsrtp-x86_64-unknown-linux-gnu 
-lresample-x86_64-unknown-linux-gnu 
-lgsmcodec-x86_64-unknown-linux-gnu 
-lspeex-x86_64-unknown-linux-gnu 
-lilbccodec-x86_64-unknown-linux-gnu 
-lg7221codec-x86_64-unknown-linux-gnu 
-lyuv-x86_64-unknown-linux-gnu 
-lwebrtc-x86_64-unknown-linux-gnu 
-lpj-x86_64-unknown-linux-gnu 
-lpjsdp-x86_64-unknown-linux-gnu 
-lasound)
TARGET_LINK_LIBRARIES(${EXE_NAME}  libjrtp.a libjthread.a)
TARGET_LINK_LIBRARIES(${EXE_NAME} libtinyxml2.a)
TARGET_LINK_LIBRARIES(${EXE_NAME} libjsoncpp.a)
TARGET_LINK_LIBRARIES(${EXE_NAME} libevent.a libevent_pthreads.a)
TARGET_LINK_LIBRARIES(${EXE_NAME} libmpeg.a)