bin_PROGRAMS = st_server st_client solar_system_test

st_server_SOURCES = game_data.cpp server.cpp game_handler.cpp com_server.cpp grid_tree.cpp fleet.cpp solar.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp cost.cpp choice.cpp game_object.cpp ship.cpp waypoint.cpp

st_client_SOURCES = client.cpp graphics.cpp client_game.cpp com_client.cpp selector.cpp research.cpp fixed_star.cpp fleet.cpp solar.cpp game_data.cpp grid_tree.cpp command_gui.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp target_gui.cpp choice.cpp cost.cpp game_object.cpp ship.cpp waypoint.cpp

solar_system_test_SOURCES = choice.cpp solar.cpp utility.cpp cost.cpp solar_system_test.cpp

all-local:
	mv st_client st_server solar_system_test ../
