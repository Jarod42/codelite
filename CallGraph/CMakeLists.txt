# set the plugin name here
set(PLUGIN_NAME "CallGraph")

# Our project is called 'plugin' this is how it will be called in visual studio, and in our makefiles.
project(${PLUGIN_NAME})

# It was noticed that when using MinGW gcc it is essential that 'core' is mentioned before 'base'.

# wxWidgets include (this will do all the magic to configure everything)
include("${wxWidgets_USE_FILE}")

if(USE_PCH)
    add_definitions(-include "${CL_PCH_FILE}")
    add_definitions(-Winvalid-pch)
endif(USE_PCH)

file(GLOB SRCS "*.cpp")

# Define the output
add_library(${PLUGIN_NAME} SHARED ${SRCS})

if(UNIX OR APPLE)
    set_target_properties(${PLUGIN_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# Remove the "lib" prefix from the plugin name
set_target_properties(${PLUGIN_NAME} PROPERTIES PREFIX "")
target_link_libraries(${PLUGIN_NAME} ${LINKER_OPTIONS} ${wxWidgets_LIBRARIES} -L"${CL_LIBPATH}" libcodelite plugin)
# Make sure that the plugin will not start build before 'plugin.so' is ready
add_dependencies(${PLUGIN_NAME} plugin)

if(NOT APPLE)
    install(TARGETS ${PLUGIN_NAME} DESTINATION ${PLUGINS_DIR})

    if(MINGW)
        find_program (dot_exe NAMES dot PATHS "${MSYS2_BASE}/clang64/bin" "${MSYS2_BASE}/mingw64/bin" NO_CACHE)
        find_program (gprof_exe NAMES gprof PATHS "${MSYS2_BASE}/clang64/bin" "${MSYS2_BASE}/mingw64/bin" NO_CACHE)

        if(NOT dot_exe)
             message(WARNING "\n** Could not locate dot in ${MSYS2_BASE}/clang64/bin")
        elseif(NOT gprof_exe)
             message(WARNING "\n** Could not locate gprof in ${MSYS2_BASE}/mingw64/bin")
        else()
            msys_list_deps(gprof_exe gprof_DEPS)
            msys_list_deps(dot_exe dot_DEPS)

            install(FILES ${gprof_exe} DESTINATION "${CMAKE_INSTALL_PREFIX}")
            install(FILES ${dot_exe} DESTINATION "${CMAKE_INSTALL_PREFIX}")

            foreach(DLL ${gprof_DEPS})
                install(FILES "${DLL}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
            endforeach()
            foreach(DLL ${dot_DEPS})
                install(FILES "${DLL}" DESTINATION "${CMAKE_INSTALL_PREFIX}")
            endforeach()
        endif()
    endif()
endif()
