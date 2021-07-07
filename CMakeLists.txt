cmake_minimum_required(VERSION 3.5)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

if (CMAKE_VERSION VERSION_LESS "3.0")
	project(sda C CXX)
	set(PROJECT_VERSION 2.7.5.0 LANGUAGES C CXX)
else ()
	cmake_policy(SET CMP0042 NEW)
	cmake_policy(SET CMP0048 NEW)
	project(sda VERSION 2.7.5.0 LANGUAGES C CXX)
endif()

include(CheckCSourceCompiles)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(ExternalProject)
include(FindPackageHandleStandardArgs)
include(GNUInstallDirs)

if (POLICY CMP0068)
	cmake_policy(SET CMP0068 NEW)
endif ()

if (POLICY CMP0025)
	cmake_policy(SET CMP0025 NEW)
endif()

if(CMAKE_GENERATOR MATCHES "Ninja")
	if(CMAKE_VERSION VERSION_LESS "3.2")
		message(FATAL_ERROR "The ninja generator require CMake 3.2+. Try the \"Unix MakeFiles\" generator instead.")
	endif()
endif()

#
# Build option
#
option(SDA_WITH_OPENSSL "Option to build libsda with openssl support" OFF)
option(SDA_BUILD_SAMPLES "Option to test sample" OFF)
option(SDA_BUILD_TESTS "Option to build tests" OFF)

if(NOT DEFINED SDA_OPENSSL_VERSION OR SDA_OPENSSL_VERSION STREQUAL "")
	set(SDA_OPENSSL_VERSION "1.1.1h" CACHE STRING "OpenSSL version for platform doesn't have system OpenSSL")
endif()

# Plat form detection ########################################################################
if(NOT DEFINED TARGET_SYSTEM_NAME)
	if(CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(OS_LINUX ON)
		add_definitions(-DOS_LINUX -DLINUX)
	elseif(CMAKE_SYSTEM_NAME MATCHES "Android")
		set(OS_ANDROID ON)
		add_definitions(-DOS_ANDROID -DANDROID)
	elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
		set(OS_WINDOWS ON)
	endif()
endif()

if(SDA_SKIP_SYSTEM_PROCESSOR_DETECTION)
	message(STATUS "SDA: skip system processor detecton")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
	set(X86_64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*")
	set(X86 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*|arm64.*|ARM64.*)")
	set(AARCH64 1)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
	set(ARM 1)
else()
	if(NOT SDA_SUPPRESS MESSAGE_UNRECOGNIZED_SYSTEM_PROCESSOR)
		message(WARNING "SDA: unrecognized target processor configuration")
	endif()
endif()

# Platform specific configuration ##############################################################
if(WIN32)
	# Support Windows 7 or later
	set(CMAKE_SYSTEM_VERSION 7 CACHE STRING INTERNAL FORCE)
endif()

if(NOT ANDROID AND NOT WIN32)
	# Android does not need these setting because they are already set by toolchain file
	# PIE/PIC
	if(CMAKE_VERSION VERSION_GREATER 2.8.8)
		set(CMAKE_POSITON_INDEPENDENT_CODE TRUE)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")
	else()
		set(CMAKE_POSITON_INDEPENDENT_CODE FALSE)
		set(CMAKE_CXX_FLAGS "-fPIC ${CMAKE_CXX_FLAGS}")
		set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
	endif()
endif()

if(MSVC)
	if(CMAKE_GENERATOR STREQUAL "Ninja")
		SET(CMAKE_C_FLAGS " /FS ${CMAKE_C_FLAGS}")
		SET(CMAKE_CXX_FLAGS "/FS ${CMAKE_CXX_FLAGS}")
	endif()
endif()

if(NOT DEFINED SDA_ALLOW_UNDEFINED_SYSBOLS OR NOT SDA_ALLOW_UNDEFINED_SYSBOLS AND NOT OS_IOS)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--no-undefined")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,error")
		set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,-undefined,error")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-undefined,error")
	endif()
endif()

# Target specification
if(OS_ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
	set(SDA_WITH_JNI ON)
endif()

# Dependencies #####################################################################
#
# Find PkgConfig
#
if(UNIX OR MINGW AND NOT ANDROID)
	find_package(PkgConfig REQUIRED)
endif()

#
# Find Threads
#
find_package(Threads REQUIRED)

#
# Find JNI
#
if(SDA_WITH_JNI AND NOT ANDROID)
	message(STATUS "JAVA_HOME= $ENV{JAVVA_HOME}")
	find_package(JNI REQUIRED)
	list(APPEND SDA_LIBS "-ljawt -ljvm")
	#	JNI_INCLUDE_DIRS	= the include dirs to use
	#	JNI_LIBRARY			= the lib to use 
	#	JNI_FOUND			= TRUE if JNI headers and libs were found 
	#	JAVA_AWT_LIBRARY	= the path to the jawt library
	#	JAVA_JVM_LIBRARY	= the path to the jvm library
	#	JAVA_INCLUDE_PATH	= the include path to jni.h 
	#	JAVA_INCLUDE_PATH2	= the include path to jni_md.h 
	#	JAVA_AWT_INCLUDE_PATH = the include path to jawt.h 
	message(STATUS "JNI_INCLUDE_DIRS= ${JNI_INCLUDE_DIRS}")
	message(STATUS "JNI_LIBRARY= ${JNI_LIBRARY}")
	message(STATUS "JAVA_AWT_LIBRARY= ${JAVA_AWT_LIBRARY}")
	message(STATUS "JAVA_JVM_LIBRARY= ${JAVA_JVM_LIBRARY}")
	message(STATUS "JAVA_INCLUDE_PATH= ${JAVA_INCLUDE_PATH}")
	message(STATUS "JAVA_INCLUDE_PATH2= ${JAVA_INCLUDE_PATH2}")
	message(STATUS "JAVA_AWT_INCLUDE_PATH= ${JAVA_AWT_INCLUDE_PATH}")
endif()

#
# Find Windows system libraries
#
if(WIN32)
	find_library(OSLIBS_IPHLPAPI_LIBRARY NAMES IPHLPAPI)
	mark_as_advanced(OSLIBS_IPHLPAPI_LIBRARY)
	
	find_library(OSLIBS_Ws32_32_LIBRARY NAMES Ws2_32.lib)
	mark_as_advanced(OSLIBS_Ws32_32_LIBRARY)
	
	find_library(OSLIBS_WINMM_LIBRARY NAMES Winmm.lib)
	mark_as_advanced(OSLIBS_WINMM_LIBRARY)
	
	find_library(OSLIBS_ADVAPI_32_LIBRARY NAMES advapi32.lib)
	mark_as_advanced(OSLIBS_ADVAPI_32_LIBRARY)
	
	find_library(OSLIBS_USER_32_LIBRARY NAMES user32.lib)
	mark_as_advanced(OSLIBS_USER_32_LIBRARY)
	
	find_library(OSLIBS_CRYPT_32_LIBRARY NAMES crypt32.lib)
	mark_as_advanced(OSLIBS_CRYPT_32_LIBRARY)

	find_package_handle_standard_args(OSLibs REQUIRED_VARS 
			OSLIBS_IPHLPAPI_LIBRARY
			OSLIBS_Ws32_32_LIBRARY
			OSLIBS_WINMM_LIBRARY
			OSLIBS_ADVAPI_32_LIBRARY
			OSLIBS_USER_32_LIBRARY
			OSLIBS_CRYPT_32_LIBRARY)
	
	if(NOT OSLIBS_FOUND)
		SET(OSLIBS_IPHLPAPI_LIBRARY iphlpapi)
		SET(OSLIBS_Ws32_32_LIBRARY Ws2_32)
		SET(OSLIBS_WINMM_LIBRARY winmm)
		SET(OSLIBS_ADVAPI_32_LIBRARY advapi32)
		SET(OSLIBS_USER_32_LIBRARY user32)
		SET(OSLIBS_CRYPT_32_LIBRARY crypt32)
	endif()
	set(HAVE_OSLIBS 1)
	set(OSLIBS_INCLUDE_DIRS)
	
	set(OSLIBS_LIBRARIES
		${OSLIBS_IPHLPAPI_LIBRARY}
		${OSLIBS_Ws32_32_LIBRARY}
		${OSLIBS_ADVAPI_32_LIBRARY}
		${OSLIBS_USER_32_LIBRARY}
		${OSLIBS_WINMM_LIBRARY}
                ${OSLIBS_CRYPT_32_LIBRARY})
	list(APPEND SDA_LIBS "${OSLIBS_LIBRARIES}")
	message(STATUS "Setting Windows system libraries manually then..")
endif()

#
# Find MINGW runtime
#
if(MINGW)
	find_program(MINGW_LIBGCC NAMES libgcc_s_sjlj-1.dll libgcc_s_seh-1.dll libgcc_s_dw2-1.dll)
	if(MINGW_LIBGCC)
		message(STATUS "Found libgcc at ${MINGW_LIBGCC}")
	else()
		message(SEND_ERROR "Could not find libgcc!")
	endif()
	
	find_program(MINGW_LIBSTDCPP libstdc++-6.dll)
	if(MINGW_LIBSTDCPP)
		message(SEND_ERROR "Found libstdc++ at ${MINGW_LIBSTDCPP}")
	else()
		message(SEND_ERROR "Could not find libstdc++!")
	endif()
	
	find_program(MINGW_LIBWINPTHREAD libwinpthread-1.dll)
	if(MINGW_LIBWINPTHREAD)
		message(STATUS "Found libwinpthread at ${MINGW_LIBWINPTHREAD}")
	else()
		message(SEND_ERROR "Could not find libwinpthread!")
        endif()
endif()

#
# Find or install OpenSSL
#

####### libsda configuration ###########################################################################
if(SDA_WITH_OPENSSL)
	include(FindOrInstallOpenSSL)
	add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
endif()
#
###### libsda
set(SDA_INCLUDE_DIR sda)
set(SDA_SRCS
		sda/Config.cpp
                sda/CommunicationManager.cpp
		sda/LocalVersionManager.cpp
		sda/SystemInfo.cpp
		sda/UpdateService.cpp
		sda/Utils.cpp
	)

set(SQLITE_SRCS
		lib/sqlite/sqlite3.c)

set(PUGIXML_SRCS
		lib/pugixml/src/pugixml.cpp)
		
set(CXXURL_SRCS
		lib/CxxUrl/url.cpp)
		
set(WHEREAMI_SRCS
		lib/whereami/whereami.c)
		
	
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/cpp-httplib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/json/single_include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/sqlite)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/pugixml/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/CxxUrl)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/spdlog/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/whereami)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/filesystem/include)
include_directories(${SDA_INCLUDE_DIR})

add_library(sda STATIC ${PUGIXML_SRCS} ${CXXURL_SRCS} ${WHEREAMI_SRCS} ${SDA_SRCS})
target_link_libraries(sda
	${CONAN_LIBS} ${SSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
	
#
##### sca-cli
add_executable(sda-cli cli/CommandLineClient.cpp)
target_link_libraries(sda-cli
	sda ${CONAN_LIBS} ${SSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${OSLIBS_LIBRARIES})
	
	
#
######## sda-qt
# Qr=tcreator support the following variable for Android, which are identical to qmake Android variable.
# Check http://doc.qt.io/qt-5/deployment-android.html for more infomation.
# They need to be set before the find_package(QT5...) call.

if(ANDROID)
		set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
		if(ANDROID_ABI STREQUAL "armeabi-v7a")
			set(ANDROID_EXTRA_LIBS
				${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so 
				${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
		endif()
endif()


find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Network REQUIRED)
	
if(ANDROID)
	add_library(sda-qt SHARED
		app/main.cpp
		app/mainwindow.cpp
		app/mainwindow.h
		app/mainwindow.ui
	)
elseif(WIN32)
	add_executable(sda-qt WIN32
		app/win/stdafx.h
		app/win/CAgentWIndow.cpp
		app/win/icon.icon
		app/win/sda-qt.rc
	)
endif()

if(TARGET sda-qt)
	target_link_libraries(sda-qt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network
		sda ${CONAN_LIBS} ${SSL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${OSLIBS_LIBRARIES}) 
endif()

#
# Installation
#

if(MINGW)
	add_custom_target(MingWCopyBinaries
			COMMAND ${CMAKE_COMMAND} -E copy ${MINGW_LIBGCC} ${CMAKE_BINARY_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy ${MINGW_LIBSTDCPP} ${CMAKE_BINARY_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy ${MINGW_LIBWINPTHREAD} ${CMAKE_BINARY_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy ${MINGW_LIBCRYPTO} ${CMAKE_BINARY_DIR}
                        COMMAND ${CMAKE_COMMAND} -E copy ${MINGW_LIBSSL} ${CMAKE_BINARY_DIR})
	add_dependencies(sda MingWCopyBinaries)
endif()

# Qt deployment
if(CMAKE_SYSTEM_NAME STREQUAL Windows)
	include(WinDeployQt)
	WinDeployQt(sda-qt ${CMAKE_BINARY_DIR}/bin ${CMAKE_CURRENT_SOURCE_DIR}/bin)
endif()

if(NOT SKIP_INSTALL_LIBRARIES AND NOT SKIP_INSTALL_ALL)
	if(NOT ANDROID)
		install(TARGETS sda
				RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
				ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
		endif()
		
		if(SDA_WITH_JNI)
			install(TARGETS sda-jni
				RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
				ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
				LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
		endif()
		install(FILES ${SDA_STATIC_LIBRARY} DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

# Build configuration summary ######################################################
message(STATUS "")
message(STATUS "				---( libsda " ${VERSION} " )---")
message(STATUS "")
message(STATUS "CMAKE_INSTALL_PREFIX:			${CMAKE_INSTALL_PREFIX}")
message(STATUS "CMAKE_INSTALL_LIBDIR:			${CMAKE_INSTALL_LIBDIR}")
message(STATUS "CMAKE_BINARY_DIR:			${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR:			${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR:			${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR:			${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "PROJECT_BINARY_DIR:			${PROJECT_BINARY_DIR}")
message(STATUS "PROJECT_SOURCE_DIR:			${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_MODULE_PATH:			${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_COMMAND:			${CMAKE_COMMAND}")
message(STATUS "CMAKE_ROOT:			${CMAKE_ROOT}")
message(STATUS "CMAKE_SYSTEM:			${CMAKE_SYSTEM}")
message(STATUS "CMAKE_SYSTEM_NAME:			${CMAKE_SYSTEM_NAME}")
message(STATUS "CMAKE_SYSTEM_VERSION:			${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMAKE_SYSTEM_PROCESSOR:			${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "CMAKE_SKIP_RPATH:			${CMAKE_SKIP_RPATH}")
message(STATUS "CMAKE_VERBOSE_MAKEFILE:			${CMAKE_VERBOSE_MAKEFILE}")
message(STATUS "CMAKE_BUILD_TYPE:			${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_C_COMPILER:			${CMAKE_C_COMPILER}  (id ${CMAKE_C_COMPILER_ID}, clang ${CLANG}, GNUC ${GNUC})")
if(CMAKE_COMPILER_IS_GNUCXX)
	message(STATUS "CMAKE_CXX_COMPILER:		${CMAKE_CXX_COMPILER}  (ver ${CMAKE_CXX_COMPILER_VERSION})")
else()
	message(STATUS "CMAKE_CXX_COMPILER:		${CMAKE_CXX_COMPILER}")
endif()
message(STATUS "CMAKE_AR:			${CMAKE_AR}")
message(STATUS "CMAKE_RANLIB:			${CMAKE_RANLIB}")
message(STATUS "CMAKE_CXX_FLAGS:			${CMAKE_CXX_FLAGS}")
message(STATUS "C++ flags (Release):			${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "C++ flags (Debug):			${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_C_FLAGS:			${CMAKE_C_FLAGS}")
message(STATUS "C flags (Release):			${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "C flags (Debug):			${CMAKE_C_FLAGS_DEBUG}")
if(WIN32)
		message(STATUS "Linker flags (Release):			${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
		message(STATUS "Linker flags (Debug):			${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
else()
		message(STATUS "Linker flags (Release):			${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
		message(STATUS "Linker flags (Debug):			${CMAKE_SHARED_LINKER_FLAGS} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
endif()
message(STATUS "CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES}")
message(STATUS "")
