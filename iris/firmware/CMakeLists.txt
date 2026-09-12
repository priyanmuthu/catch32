cmake_minimum_required(VERSION 3.22)

# This file is deliberately ignored by Git. It lets a local build use private
# credentials without putting them in the repository's shared defaults.
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.private.defaults")
    set(SDKCONFIG_DEFAULTS "sdkconfig.defaults;sdkconfig.private.defaults")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(iris)
