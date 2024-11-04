cmake_minimum_required(VERSION 3.18)

# ##############################################################################
# Check submodules are here
# ##############################################################################
find_path(CTAGS_SUBMODULE CMakeLists.txt PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ctags/)
if(NOT CTAGS_SUBMODULE)
	message(STATUS "Could not locate `${CMAKE_CURRENT_SOURCE_DIR}/ctags/CMakeLists.txt` file")
	message(FATAL_ERROR "Please make sure you have run `git submodule update --init`")
endif()

# ##############################################################################
# asio
# ##############################################################################
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include)

# ##############################################################################
# cc-wrapper
# ##############################################################################
add_subdirectory(cc-wrapper)

install(TARGETS cc-wrapper DESTINATION ${CL_INSTALL_BIN})

# ##############################################################################
# cJSON
# ##############################################################################
add_library(cJSON STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/cJSON/cJSON.c
	${CMAKE_CURRENT_SOURCE_DIR}/cJSON/cJSON.h
)
target_include_directories(cJSON PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cJSON)

# ##############################################################################
# ctags
# ##############################################################################
if(NOT WXC_APP)
    add_subdirectory(ctags)
    install(TARGETS ctags DESTINATION ${CL_INSTALL_BIN})
    set_target_properties(ctags PROPERTIES OUTPUT_NAME "codelite-ctags")
    if(APPLE)
        install(
            TARGETS ctags
            DESTINATION ${CMAKE_BINARY_DIR}/codelite.app/Contents/MacOS
            PERMISSIONS ${EXE_PERM})
    else()
        install(
            TARGETS ctags
            DESTINATION ${CL_PREFIX}/bin
            PERMISSIONS ${EXE_PERM})
    endif()
endif()

# ##############################################################################
# dtl
# ##############################################################################
add_library(dtl INTERFACE)
target_include_directories(dtl INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/dtl)

# ##############################################################################
# llama.cpp
# ##############################################################################
if(WITH_CHATAI AND NOT WXC_APP)
    add_subdirectory("llama.cpp")
    add_subdirectory("llama.cpp/examples/main")
    install(TARGETS llama-cli DESTINATION ${CL_INSTALL_BIN})
endif()

# ##############################################################################
# websocketpp
# ##############################################################################
add_library(websocketpp INTERFACE)
target_include_directories(websocketpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/websocketpp)

# ##############################################################################
# yaml-cpp
# ##############################################################################
if(NOT APPLE)
    add_subdirectory(yaml-cpp)
    set(LIBYAML_CPP "yaml-cpp::yaml-cpp")
endif()

# ##############################################################################
# wx-config-msys2
# ##############################################################################

if(NOT WXC_APP AND MINGW)
    # build wx-config for Windows
    add_subdirectory(wx-config-msys2)
    install(TARGETS wx-config DESTINATION ${CL_INSTALL_BIN})
endif()

# ##############################################################################
# For wxWidgets dependencies
# ##############################################################################
include("${wxWidgets_USE_FILE}")

# ##############################################################################
# wxshapeframework
# ##############################################################################
file(GLOB WXSF_SRCS
	"${CMAKE_CURRENT_SOURCE_DIR}/wxsf-code/src/*.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/wxsf-code/src/wxxmlserializer/*.cpp"
)
add_library(wxshapeframework SHARED ${WXSF_SRCS})

target_include_directories(wxshapeframework PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/wxsf-code/src")
target_include_directories(wxshapeframework PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/wxsf-code/include")

if(WIN32)
	target_compile_definitions(wxshapeframework PRIVATE WXMAKINGDLL_WXXS WXMAKINGDLL_WXSF)
	target_compile_definitions(wxshapeframework INTERFACE WXUSINGDLL)
endif(WIN32)

if(UNIX OR APPLE)
    set_target_properties(wxshapeframework PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

target_link_libraries(wxshapeframework PRIVATE ${LINKER_OPTIONS} ${wxWidgets_LIBRARIES})

codelite_install_library_target(wxshapeframework)
