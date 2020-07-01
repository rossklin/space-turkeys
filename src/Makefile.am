bin_PROGRAMS = st_server st_client test

st_server_SOURCES = terrain_object.cpp game_base_data.cpp game_data.cpp server.cpp game_handler.cpp com_server.cpp grid_tree.cpp fleet.cpp solar.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp cost.cpp game_object.cpp ship.cpp waypoint.cpp upgrades.cpp interaction.cpp development_tree.cpp ship_stats.cpp server_handler.cpp

st_client_SOURCES = terrain_object.cpp game_base_data.cpp client.cpp graphics.cpp client_game.cpp com_client.cpp selector.cpp research.cpp fixed_star.cpp fleet.cpp solar.cpp game_data.cpp grid_tree.cpp command_gui.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp types.cpp target_gui.cpp cost.cpp game_object.cpp ship.cpp waypoint.cpp upgrades.cpp interaction.cpp animation.cpp development_tree.cpp desktop.cpp ship_stats.cpp utility.cpp choice_gui.cpp

# solar_system_test_SOURCES = game_base_data.cpp solar_system_test.cpp game_data.cpp game_handler.cpp com_server.cpp grid_tree.cpp fleet.cpp solar.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp cost.cpp game_object.cpp ship.cpp waypoint.cpp upgrades.cpp interaction.cpp development_tree.cpp ship_stats.cpp server_handler.cpp

test_SOURCES = terrain_object.cpp game_base_data.cpp test.cpp game_data.cpp game_handler.cpp com_server.cpp grid_tree.cpp fleet.cpp solar.cpp research.cpp game_settings.cpp serialization.cpp socket_t.cpp command.cpp utility.cpp types.cpp cost.cpp game_object.cpp ship.cpp waypoint.cpp upgrades.cpp interaction.cpp development_tree.cpp ship_stats.cpp server_handler.cpp

all-local:
	mv test st_client st_server ../
