root_dir = $(shell pwd)
tgui_dir = external/src/TGUI
sfml_dir = external/src/SFML

cflags = -ggdb --std=c++11 -Wreturn-type -isystem external/include
lflags = -Lexternal/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread

server_objects = game_data.o server.o game_handler.o com_server.o grid_tree.o fleet.o solar.o research.o 
tgui_objects = command_gui.o
client_objects = client.o graphics.o client_game.o com_client.o selector.o research.o fleet.o solar.o $(tgui_objects)
common_objects = game_settings.o serialization.o socket_t.o command.o utility.o types.o ship.o
all_objects = $(server_objects) $(client_objects) $(common_objects)

all: server client

server : sfml $(server_objects) $(common_objects)
	g++ $(cflags) -o server $(server_objects) $(common_objects) $(lflags)

client : sfml tgui $(client_objects) $(common_objects)
	g++ $(cflags) -o client $(client_objects) $(common_objects) $(lflags) -ltgui

command_gui.o : tgui

tgui : sfml
	echo building $(tgui_dir)
	cd $(tgui_dir); cmake -DCMAKE_INSTALL_PREFIX:PATH=$(root_dir)/external -DSFML_INCLUDE_DIR=$(root_dir)/external/include .
	cd $(tgui_dir); make all install

sfml : force_check
	echo building $(sfml_dir)
	cd $(sfml_dir); cmake -DCMAKE_INSTALL_PREFIX:PATH=$(root_dir)/external .
	cd $(sfml_dir); make all install

-include $(all_objects:.o=.d)

%.o: %.cpp
	g++ -c $(cflags) $*.cpp -o $*.o
	g++ -MM $(cflags) $*.cpp > $*.d

clean: 
	rm -f server client *.o *.d
	rm -f external/lib/*
	cd $(tgui_dir); make clean
	cd $(sfml_dir); make clean

force_check:
	true
