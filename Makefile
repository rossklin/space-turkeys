COMMON_SOURCES=game_data.cpp terrain_object.cpp game_base_data.cpp fleet.cpp solar.cpp ship.cpp waypoint.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp cost.cpp game_object.cpp upgrades.cpp interaction.cpp development_tree.cpp ship_stats.cpp
SERVER_SOURCES=server.cpp game_handler.cpp com_server.cpp server_handler.cpp
CLIENT_SOURCES=client.cpp graphics.cpp client_game.cpp com_client.cpp selector.cpp fixed_star.cpp command_gui.cpp target_gui.cpp animation.cpp utility.cpp choice_gui.cpp style.cpp solar_gui.cpp
TESTER_SOURCES=test.cpp game_handler.cpp com_server.cpp server_handler.cpp
RSG_SOURCES=button.cpp component.cpp container.cpp panel.cpp textfield.cpp slider.cpp utility.cpp button_options.cpp progress_bar.cpp
SRC_DIR=./src
BUILD_DIR=./build
DEBUG_BUILD_DIR=./debug_build
RSG_DIR=./rsg

RSG_OBJECTS=$(RSG_SOURCES:%.cpp=$(RSG_DIR)/build/%.o)
COMMON_OBJ=$(COMMON_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
SERVER_OBJ=$(SERVER_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJ=$(CLIENT_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
TESTER_OBJ=$(TESTER_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

DEBUG_RSG_OBJECTS=$(RSG_SOURCES:%.cpp=$(RSG_DIR)/debug_build/%.o)
DEBUG_COMMON_OBJ=$(COMMON_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)
DEBUG_SERVER_OBJ=$(SERVER_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)
DEBUG_CLIENT_OBJ=$(CLIENT_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)
DEBUG_TESTER_OBJ=$(TESTER_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)

ALL_OBJ=$(COMMON_OBJ) $(SERVER_OBJ) $(CLIENT_OBJ)
DEBUG_ALL_OBJ=$(DEBUG_COMMON_OBJ) $(DEBUG_SERVER_OBJ) $(DEBUG_CLIENT_OBJ)

DEP=$(ALL_OBJ:%.o=%.d) $(DEBUG_ALL_OBJ:%.o=%.d)

CC=g++
CPPFLAGS=--std=c++17 -I $(RSG_DIR)/src -I . -O3
DEBUG_CPPFLAGS=--std=c++17 -I $(RSG_DIR)/src -I . -ggdb
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread

# Disable default rules
.SUFFIXES:

default: RSG test st_client st_server debug_st_client debug_st_server debug_test

RSG:
	cd $(RSG_DIR) && $(MAKE)

test : $(COMMON_OBJ) $(TESTER_OBJ)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

st_server: $(COMMON_OBJ) $(SERVER_OBJ)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

st_client: $(COMMON_OBJ) $(CLIENT_OBJ)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(RSG_OBJECTS) $^ -o $@ $(LDFLAGS)

debug_st_server: $(DEBUG_COMMON_OBJ) $(DEBUG_SERVER_OBJ)
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) $^ -o $@ $(LDFLAGS)

debug_st_client: $(DEBUG_COMMON_OBJ) $(DEBUG_CLIENT_OBJ)
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) $(DEBUG_RSG_OBJECTS) $^ -o $@ $(LDFLAGS)

debug_test : $(DEBUG_COMMON_OBJ) $(DEBUG_TESTER_OBJ)
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) $^ -o $@ $(LDFLAGS)

-include $(DEP)

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -MMD -c $< -o $@ 

$(DEBUG_BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) -MMD -c $< -o $@ 

.PHONY : clean

remove_binaries:
	-rm -rf test st_client st_server debug_st_client debug_st_server

clean :
	-rm -rf test st_client st_server debug_st_client debug_st_server $(ALL_OBJ) $(DEBUG_ALL_OBJ) $(DEP)
	cd $(RSG_DIR) && $(MAKE) clean


