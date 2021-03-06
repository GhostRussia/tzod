cmake_minimum_required (VERSION 3.3)
project(TZodMain CXX)

include_directories(
	${App_SOURCE_DIR}/inc
	${AS_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${PlatGLFW_SOURCE_DIR}/inc

	${LibUI_SOURCE_DIR}/inc # logger
)

link_directories(${Boost_LIBRARY_DIR})

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501) # Windows XP
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DNOMINMAX)
	add_executable(tzodmain WIN32 Main.cpp)
elseif(APPLE)
	add_executable(tzodmain Main.cpp) # hack to enable retina display support
#	add_executable(tzodmain MACOSX_BUNDLE Main.cpp)
else(UNIX)
	add_executable(tzodmain Main.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(tzodmain
	app
	platglfw
)
