COMMON_SOURCES=game_data.cpp terrain_object.cpp game_base_data.cpp fleet.cpp solar.cpp ship.cpp waypoint.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp cost.cpp game_object.cpp upgrades.cpp interaction.cpp development_tree.cpp ship_stats.cpp
SERVER_SOURCES=server.cpp game_handler.cpp com_server.cpp grid_tree.cpp server_handler.cpp
CLIENT_SOURCES=client.cpp graphics.cpp client_game.cpp com_client.cpp selector.cpp fixed_star.cpp command_gui.cpp target_gui.cpp animation.cpp desktop.cpp utility.cpp choice_gui.cpp
TESTER_SOURCES=test.cpp game_handler.cpp com_server.cpp grid_tree.cpp server_handler.cpp
RSG_OBJECTS=button.o component.o container.o panel.o textfield.o slider.o utility.o
SRC_DIR=./src
BUILD_DIR=./build
DEBUG_BUILD_DIR=./debug_build
RSG_DIR=./rsg

COMMON_OBJ=$(COMMON_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
SERVER_OBJ=$(SERVER_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
CLIENT_OBJ=$(CLIENT_SOURCES:%.cpp=$(BUILD_DIR)/%.o)
TESTER_OBJ=$(TESTER_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

DEBUG_COMMON_OBJ=$(COMMON_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)
DEBUG_SERVER_OBJ=$(SERVER_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)
DEBUG_CLIENT_OBJ=$(CLIENT_SOURCES:%.cpp=$(DEBUG_BUILD_DIR)/%.o)

ALL_OBJ=$(COMMON_OBJ) $(SERVER_OBJ) $(CLIENT_OBJ)
DEBUG_ALL_OBJ=$(DEBUG_COMMON_OBJ) $(DEBUG_SERVER_OBJ) $(DEBUG_CLIENT_OBJ)

DEP=$(ALL_OBJ:%.o=%.d) $(DEBUG_ALL_OBJ:%.o=%.d)

CC=g++
CPPFLAGS=--std=c++17 -I $(RSG_DIR)/src -O3
DEBUG_CPPFLAGS=--std=c++17 -I $(RSG_DIR)/src -ggdb
LDFLAGS=-lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread

# Disable default rules
.SUFFIXES:

default: test st_client st_server debug_st_client debug_st_server

RSG:
	cd $(RSG_DIR) && $(MAKE)

test : $(COMMON_OBJ) $(TESTER_OBJ)
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

st_server: $(COMMON_OBJ) $(SERVER_OBJ) $(SRC_DIR)/server.cpp
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $^ -o $@ $(LDFLAGS)

st_client: RSG $(COMMON_OBJ) $(CLIENT_OBJ) $(SRC_DIR)/client.cpp
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(RSG_DIR)/build/$(RSG_OBJECTS) $^ -o $@ $(LDFLAGS)

debug_st_server: $(COMMON_OBJ) $(SERVER_OBJ) $(SRC_DIR)/server.cpp
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) $^ -o $@ $(LDFLAGS)

debug_st_client: RSG $(COMMON_OBJ) $(CLIENT_OBJ) $(SRC_DIR)/client.cpp
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) $(RSG_DIR)/debug_build/$(RSG_OBJECTS) $^ -o $@ $(LDFLAGS)

-include $(DEP)

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -MMD -c $< -o $@ 

$(DEBUG_BUILD_DIR)/%.o : $(SRC_DIR)/%.cpp
	mkdir -p $(@D)
	$(CC) $(DEBUG_CPPFLAGS) -MMD -c $< -o $@ 

.PHONY : clean

clean :
	-rm -rf test st_client st_server debug_st_client debug_st_server $(ALL_OBJ) $(DEBUG_ALL_OBJ) $(DEP)
	cd $(RSG_DIR) && $(MAKE) clean

