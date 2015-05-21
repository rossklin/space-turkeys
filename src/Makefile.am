bin_PROGRAMS = st_server st_client

st_server_SOURCES = game_data.cpp server.cpp game_handler.cpp com_server.cpp grid_tree.cpp fleet.cpp solar.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp ship.cpp

st_client_SOURCES = client.cpp graphics.cpp client_game.cpp com_client.cpp selector.cpp research.cpp fleet.cpp solar.cpp game_data.cpp grid_tree.cpp command_gui.cpp solar_gui.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp ship.cpp

all-local:
	mv st_client st_server ../
