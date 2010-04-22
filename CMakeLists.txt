project(LIBCOW)
cmake_minimum_required(VERSION 2.8)

IF(UNIX)
    include(../FindCurl.cmake)
    include(../FindLibTorrent.cmake)
    
    include_directories(
        ${CURL_INCLUDE_DIR} 
        ${LIBTORRENT_INCLUDE_DIR}
    )

    set(LIBCOW_LIBRARY ${LIBCOW_SOURCE_DIR}/bin/libcowd.so)
ENDIF(UNIX)

IF(UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
ENDIF(UNIX)

set(CMAKE_BUILD_TYPE Debug)

# Use static MT version of Boost
#SET(Boost_USE_STATIC_LIBS ON)

find_package(Boost COMPONENTS system thread log filesystem REQUIRED)
    
set(EXECUTABLE_OUTPUT_PATH ${LIBCOW_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${LIBCOW_SOURCE_DIR}/bin)

set(UTILS_INCLUDE_DIR ${LIBCOW_SOURCE_DIR}/../utils/include)
set(PACKETIZER_INCLUDE_DIR ${LIBCOW_SOURCE_DIR}/../packetizer/include)
set(TINYXML_SOURCE_DIR ${LIBCOW_SOURCE_DIR}/../external_libraries/tinyxml)
set(PACKETIZER_SOURCE_DIR ${LIBCOW_SOURCE_DIR}/../packetizer/src)

set(LIBCOW_SOURCE
    ${LIBCOW_SOURCE_DIR}/src/cow_client.cpp
    ${LIBCOW_SOURCE_DIR}/src/curl_instance.cpp
    ${LIBCOW_SOURCE_DIR}/src/dispatcher.cpp
    ${LIBCOW_SOURCE_DIR}/src/download_control.cpp
    ${LIBCOW_SOURCE_DIR}/src/download_device.cpp
    ${LIBCOW_SOURCE_DIR}/src/download_device_manager.cpp
    ${LIBCOW_SOURCE_DIR}/src/multicast_server_connection.cpp
    ${LIBCOW_SOURCE_DIR}/src/multicast_server_connection_factory.cpp    
    ${LIBCOW_SOURCE_DIR}/src/on_demand_server_connection.cpp
    ${LIBCOW_SOURCE_DIR}/src/on_demand_server_connection_factory.cpp    
    ${LIBCOW_SOURCE_DIR}/src/program_sources.cpp
    ${LIBCOW_SOURCE_DIR}/src/program_table_server_connection.cpp
    ${LIBCOW_SOURCE_DIR}/src/system.cpp
    ${LIBCOW_SOURCE_DIR}/src/tinyxml.cpp
    ${LIBCOW_SOURCE_DIR}/src/tinyxmlerror.cpp
    ${LIBCOW_SOURCE_DIR}/src/tinyxmlparser.cpp
    ${PACKETIZER_SOURCE_DIR}/packetizer.cpp
)

set(LIBCOW_HEADERS
    ${LIBCOW_SOURCE_DIR}/include/cow/cow.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/libcow_types.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/cow_client.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/curl_instance.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/dispatcher.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/download_control.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/download_device.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/download_device_description.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/download_device_factory.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/download_device_manager.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/libcow_def.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/multicast_server_connection.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/multicast_server_connection_factory.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/on_demand_server_connection.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/on_demand_server_connection_factory.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/piece_data.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/piece_request.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/program_info.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/program_sources.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/program_table_server_connection.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/progress_info.hpp
    ${LIBCOW_SOURCE_DIR}/include/cow/system.hpp
)

set(DOWNLOAD_CONTROL_TEST_SOURCE
    ${LIBCOW_SOURCE_DIR}/test/download_control_tests.cpp
)
set(DOWNLOAD_DEVICE_TEST_SOURCE
    ${LIBCOW_SOURCE_DIR}/test/download_device_tests.cpp
)
set(MULTICAST_SERVER_CONNECTION_TEST_SOURCE
    ${LIBCOW_SOURCE_DIR}/test/multicast_server_connection_tests.cpp
)

set(CURL_INSTANCE_TEST_SOURCE
    ${LIBCOW_SOURCE_DIR}/test/curl_instance_tests.cpp
    ${TINYXML_SOURCE_DIR}/tinyxml.cpp
    ${TINYXML_SOURCE_DIR}/tinyxmlerror.cpp
    ${TINYXML_SOURCE_DIR}/tinyxmlparser.cpp    
)

include_directories(
    ${LIBCOW_SOURCE_DIR}/include
    ${UTILS_INCLUDE_DIR}
    ${PACKETIZER_INCLUDE_DIR}
    ${PACKETIZER_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${TINYXML_SOURCE_DIR}
)
    
link_directories(
    ${Boost_LIBRARY_DIRS}
    ${LIBRARY_OUTPUT_PATH}
)

if (WIN32)    
    set_source_files_properties(${LIBCOW_SOURCE_DIR}/src/libcow_def.cpp
        PROPERTIES
        COMPILE_FLAGS "/Yccow/libcow_def.hpp"
        )
    foreach(src_file ${LIBCOW_SOURCE})
        set_source_files_properties(
            ${src_file}
            PROPERTIES
            COMPILE_FLAGS "/Yucow/libcow_def.hpp"
            )
    endforeach(src_file ${LIBCOW_SOURCE})
    list(APPEND LIBCOW_SOURCE ${LIBCOW_SOURCE_DIR}/src/libcow_def.cpp)
endif (WIN32)

IF(WIN32)
    add_definitions(
        -DCURL_STATICLIB 
        -DBOOST_ALL_NO_LIB
        -DTORRENT_BUILDING_SHARED
        -DTORRENT_DEBUG
        -DTORRENT_DISABLE_ENCRYPTION
        -DTORRENT_DISABLE_GEO_IP
        -DTORRENT_DISABLE_INVARIANT_CHECKS
    )
ENDIF(WIN32)

add_definitions(-DTIXML_USE_STL)


# set                  
IF(WIN32)
    SET(LIBCOW_DEPS 
        ${Boost_LIBRARIES}

        torrent
        ws2_32
        winmm
        wldap32

        debug libcurl
        optimized libcurl
        )
    SET(TEST_DEPS ${LIBCOW_DEPS} cowd.lib)
ELSEIF(UNIX)
    SET(LIBCOW_DEPS
        ${CURL_LIBRARY}
        ${LIBTORRENT_LIBRARY}
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_LOG_LIBRARY}
    )
    SET(TEST_DEPS ${LIBCOW_DEPS} ${LIBCOW_LIBRARY})
ELSE(WIN32)
    MESSAGE(FATAL_ERROR "Unsupported OS")
ENDIF(WIN32)

SET(CMAKE_DEBUG_POSTFIX d)

# libcow library target
add_library(cow SHARED ${LIBCOW_SOURCE} ${LIBCOW_HEADERS})
target_link_libraries(cow ${LIBCOW_DEPS})

IF(WIN32)
# Set exception handling model
set_target_properties(cow PROPERTIES COMPILE_FLAGS "/EHa")
ENDIF(WIN32)

# download_control test target
add_executable(download_control_tests ${DOWNLOAD_CONTROL_TEST_SOURCE} ${HEADERS})
target_link_libraries(download_control_tests ${TEST_DEPS}) 
add_dependencies(download_control_tests cow)

# download_device test target
add_executable(download_device_tests ${DOWNLOAD_DEVICE_TEST_SOURCE} ${HEADERS})
target_link_libraries(download_device_tests ${TEST_DEPS})
add_dependencies(download_device_tests cow)

# multicast_server_connection test target
add_executable(multicast_server_connection_tests ${MULTICAST_SERVER_CONNECTION_TEST_SOURCE} ${HEADERS})
target_link_libraries(multicast_server_connection_tests ${TEST_DEPS})
add_dependencies(multicast_server_connection_tests cow)

# curl_instance test target
add_executable(curl_instance_tests ${CURL_INSTANCE_TEST_SOURCE} ${HEADERS})
target_link_libraries(curl_instance_tests ${TEST_DEPS})
add_dependencies(curl_instance_tests cow)
