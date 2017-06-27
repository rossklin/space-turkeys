#include <iostream>
#include <unistd.h>

#include "game_data.h"

using namespace std;
using namespace st3;

typedef unsigned long long int long_t;

long_t check_memory() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

game_data *test_setup() {
  game_data *g = new game_data();
  player p;
  g -> players[0] = p;
  g -> players[1] = p;
  g -> build();
  return g;
}

void setup_fleet_for(game_data *g, idtype pid, point at, point targ) {
  list<combid> ships;
  for (auto x : g -> all_owned_by(pid)) {
    if (x -> isa(ship::class_id)) {
      ships.push_back(x -> id);
    }
  }

  waypoint::ptr w = waypoint::create(pid);
  w -> position = targ;
  command c;
  c.target = w -> id;
  c.action = fleet_action::go_to;

  g -> add_entity(w);
  g -> generate_fleet(at, pid, c, ships);
}

void test_setup_fleets(game_data *g) {
  point a(100, 100);
  point b(200, 100);
  setup_fleet_for(g, 0, a, b);
  setup_fleet_for(g, 1, b, a);
}

bool test_memory() {
  int n = 100;
  int rep = 100;
  vector<entity_package> frames(n);
  game_data *g = test_setup();
  test_setup_fleets(g);

  long_t mem_start = check_memory();
  
  for (int j = 0; j < rep; j++) {
    frames.resize(n);
    for (int i = 0; i < n; i++) {
      g -> increment();
      frames[i].copy_from(*g);
    }

    for (auto f : frames) f.clear_entities();
    frames.clear();
    long_t mtest = check_memory();
    cout << j << ": free: " << mtest << ", used: " << (mem_start - mtest) << endl;
  }

  delete g;

  return true;
}

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(){
  game_data *g = test_setup();
  test_setup_fleets(g);

  // run game mechanics until one fleet is destroyed
  int count = 0;
  while (g -> all<fleet>().size() > 1 && count++ < 1000){
    g -> increment();
  }

  bool result = g -> all<fleet>().size() < 2;
  delete g;
  return result;
}

int main(){
  // if (!test_space_combat()) cout << "test space combat failed!" << endl;
  test_memory();
  return 0;
}
