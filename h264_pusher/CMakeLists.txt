SET(EXE_NAME h264_pusher)
SET(EXECUTABLE_FILES easypusher.cpp easy_client.cpp)

Message(${CMAKE_CURRENT_SOURCE_DIR})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/x64)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
NEW_EXECUTABLE(${EXE_NAME} SHARED)

TARGET_LINK_LIBRARIES(${EXE_NAME} ${FFMPEG_LIBRARIES} ${Boost_LIBRARIES} ${OpenCV_LIBRARIES})
TARGET_LINK_LIBRARIES(${EXE_NAME} pthread)

TARGET_COMPILE_OPTIONS(${EXE_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)