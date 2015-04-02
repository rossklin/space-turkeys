cflags = -ggdb --std=c++11
lflags = -lsfml-graphics -lsfml-window -lsfml-system -lsfml-network -lpthread

server_objects = server.o game_handler.o
client_objects = client.o graphics.o
common_objects = com.o game_data.o choice.o serialization.o
all_objects = $(server_objects) $(client_objects) $(common_objects)

all: server client

server : $(server_objects) $(common_objects)
	g++ $(cflags) -o server $(server_objects) $(common_objects) $(lflags)

client : $(client_objects) $(common_objects)
	g++ $(cflags) -o client $(client_objects) $(common_objects) $(lflags)

-include $(all_objects:.o=.d)

%.o: %.cpp
	g++ -c $(cflags) $*.cpp -o $*.o
	g++ -MM $(cflags) $*.cpp > $*.d

clean: 
	rm -f server client *.o *.d
