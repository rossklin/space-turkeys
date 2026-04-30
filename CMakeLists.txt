cmake_minimum_required(VERSION 3.14)
project(SpaceTurkeys CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define _USE_MATH_DEFINES for M_PI on Windows
add_compile_definitions(_USE_MATH_DEFINES)

# Silence RapidJSON C++17 deprecation warning
add_compile_definitions(_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

# Handle threading (replaces -lpthread)
find_package(Threads REQUIRED)

# Fetch SFML automatically
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.1
)

# Configure SFML build options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(SFML_BUILD_EXAMPLES OFF CACHE BOOL "Build SFML examples" FORCE)
set(SFML_BUILD_DOC OFF CACHE BOOL "Build SFML documentation" FORCE)
set(SFML_BUILD_AUDIO OFF CACHE BOOL "Build SFML audio" FORCE) # Not used in Makefile

FetchContent_MakeAvailable(SFML)

# Fetch RapidJSON automatically
FetchContent_Declare(rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG v1.1.0
)
set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Build rapidjson docs" FORCE)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Build rapidjson examples" FORCE)
set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Build rapidjson tests" FORCE)
FetchContent_MakeAvailable(rapidjson)

# --- RSG Library ---
# We use GLOB here to pick up the existing sources in the submodule.
file(GLOB RSG_SOURCES "rsg/src/*.cpp")
list(REMOVE_ITEM RSG_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/rsg/src/test.cpp")
add_library(rsg STATIC ${RSG_SOURCES})
target_include_directories(rsg PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/rsg/src)
target_link_libraries(rsg PRIVATE sfml-graphics sfml-window sfml-system)

# --- Common Library ---
set(COMMON_SOURCES
    src/game_data.cpp 
    src/terrain_object.cpp 
    src/game_base_data.cpp 
    src/grid_tree.cpp 
    src/fleet.cpp 
    src/solar.cpp 
    src/ship.cpp 
    src/waypoint.cpp 
    src/research.cpp 
    src/game_settings.cpp 
    src/serialization.cpp 
    src/socket_t.cpp 
    src/command.cpp 
    src/utility.cpp 
    src/types.cpp 
    src/cost.cpp 
    src/game_object.cpp 
    src/upgrades.cpp 
    src/interaction.cpp 
    src/development_tree.cpp 
    src/ship_stats.cpp
)
add_library(common STATIC ${COMMON_SOURCES})
target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR} ${rapidjson_SOURCE_DIR}/include)
target_link_libraries(common PUBLIC sfml-network sfml-system Threads::Threads)

# --- Server Executable ---
set(SERVER_SOURCES
    src/server.cpp 
    src/game_handler.cpp 
    src/com_server.cpp 
    src/server_handler.cpp
)
add_executable(st_server ${SERVER_SOURCES})
target_link_libraries(st_server PRIVATE common)

# --- Client Executable ---
set(CLIENT_SOURCES
    src/client.cpp 
    src/graphics.cpp 
    src/client_game.cpp 
    src/com_client.cpp 
    src/selector.cpp 
    src/fixed_star.cpp 
    src/command_gui.cpp 
    src/target_gui.cpp 
    src/animation.cpp 
    src/choice_gui.cpp 
    src/style.cpp 
    src/solar_gui.cpp
)
add_executable(st_client ${CLIENT_SOURCES})
target_link_libraries(st_client PRIVATE common rsg sfml-graphics sfml-window sfml-system sfml-network)

# --- Test Executable ---
# Renamed from 'test' to 'st_test' to avoid conflicts with CMake's CTest
set(TESTER_SOURCES
    src/test.cpp 
    src/game_handler.cpp 
    src/com_server.cpp 
    src/server_handler.cpp
)
add_executable(st_test ${TESTER_SOURCES})
target_link_libraries(st_test PRIVATE common)
