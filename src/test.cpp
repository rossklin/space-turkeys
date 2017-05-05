#include <iostream>

#include "game_data.h"

using namespace std;
using namespace st3;

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(){
  game_data g;
  player p;
  g.players[0] = p;
  g.players[1] = p;
  g.build();

  // put ships in fleets
  auto ships = g.all<ship>();
  point x(100,100);
  command c;
  c.action = fleet_action::go_to;
  for (auto s : ships){
    auto buf = list<combid>(1, s -> id);
    g.generate_fleet(x, s -> owner, c, buf);
  }

  // order fleets to attack each other
  auto fleets = g.all<fleet>();
  fleet::ptr f0 = fleets.front();
  fleet::ptr f1 = fleets.back();
  f0 -> com.action = interaction::space_combat;
  f0 -> com.target = f1 -> id;
  f1 -> com.action = interaction::space_combat;
  f1 -> com.target = f0 -> id;

  // run game mechanics until one fleet is destroyed
  int count = 0;
  while (g.all<fleet>().size() > 1 && count++ < 1000){
    g.increment();
  }

  return g.all<fleet>().size() < 2;
}

int main(){
  if (!test_space_combat()) cout << "test space combat failed!" << endl;
  return 0;
}
