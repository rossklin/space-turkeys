#include <iostream>
#include <unistd.h>

#include "game_data.h"
#include "research.h"

using namespace std;
using namespace st3;

typedef unsigned long long int long_t;

long_t check_memory() {
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    return pages * page_size;
}

game_data *test_setup(game_settings set = game_settings()) {
  game_data *g = new game_data();
  g -> settings = set;
  player p;
  g -> players[0] = p;
  g -> players[1] = p;
  g -> build();
  return g;
}

void setup_fleet_for(game_data *g, idtype pid, point at, point targ, hm_t<string, int> scc = hm_t<string, int>()) {
  list<combid> ships;
  for (auto x : g -> all_owned_by(pid)) {
    if (x -> isa(ship::class_id)) {
      if (scc.empty() || scc[g -> get_ship(x -> id) -> ship_class]-- > 0) {
	ships.push_back(x -> id);
      }
    }
  }

  waypoint::ptr w = waypoint::create(pid);
  w -> position = targ;
  command c;
  c.target = w -> id;
  c.action = fleet_action::go_to;
  c.policy = fleet::policy_aggressive;

  g -> add_entity(w);
  g -> generate_fleet(at, pid, c, ships);
  for (auto sid : ships) g -> get_ship(sid) -> is_landed = false;
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

cost::res_t get_tech_cost(string tech);

cost::res_t get_facility_cost(string fac, int lev) {
  facility f = solar::facility_table().at(fac);
  cost::res_t c = f.cost_resources;
  c.scale(2 * cost::expansion_multiplier(lev)); // approx cumsum

  for (auto v : f.depends_techs) c.add(get_tech_cost(v));
  for (auto v : f.depends_facilities) c.add(get_facility_cost(v.first, v.second));

  return c;
}

cost::res_t get_tech_cost(string tech) {
  research::tech t = research::data::table().at(tech);
  cost::res_t c;

  for (auto v : t.depends_techs) c.add(get_tech_cost(v));
  for (auto v : t.depends_facilities) c.add(get_facility_cost(v.first, v.second));
  return c;
}

float fair_ship_count(string ship_class, float invest_ratio = 0.25) {
  cost::res_t investment_cost;
  ship_stats s = ship::table().at(ship_class);
  investment_cost.add(get_tech_cost(s.depends_tech));
  investment_cost.add(get_facility_cost("shipyard", s.depends_facility_level));

  float investment = investment_cost.count();
  float allowed_expenditure = investment / invest_ratio - investment;
  float can_build = floor(allowed_expenditure / s.build_cost.count());
  float per_ship = (investment + can_build * s.build_cost.count()) / can_build;

  // reference: allow spending total 1000 resource
  return 1000 / per_ship;
}

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(string c0, string c1, float req_win){
  game_settings set;
  set.starting_fleet = "massive";
  game_data *g = test_setup(set);
  
  point a(100, 100);
  point b(200, 100);

  float count0 = fair_ship_count(c0);
  float count1 = fair_ship_count(c1);
  float highest = fmax(count0, count1);
  if (highest > 99) {
    float ratio = 99 / highest;
    count0 *= ratio;
    count1 *= ratio;
  }

  hm_t<string, int> scc_0 = {{c0, ceil(count0)}};
  hm_t<string, int> scc_1 = {{c1, ceil(count1)}};
  
  setup_fleet_for(g, 0, a, b, scc_0);
  setup_fleet_for(g, 1, b, a, scc_1);

  cout << "test combat: " << scc_0[c0] << " " << c0 << " vs " << scc_1[c1] << " " << c1 << endl;

  // run game mechanics until one fleet is destroyed
  int count = 0;
  while (g -> all<fleet>().size() > 1 && count++ < 1000){
    g -> increment();
  }

  float r0 = 0, r1 = 0;
  for (auto f : g -> all<fleet>()) {
    if (f -> owner == 0) {
      r0 = f -> ships.size() / (float)scc_0[c0];
    } else if (f -> owner == 1) {
      r1 = f -> ships.size() / (float)scc_1[c1];
    }
  }

  bool result = r0 / r1 >= req_win;

  cout << (result ? "SUCCESS" : "FAILURE") << "resulting ratios: " << r0 << " : " << r1 << " : " << result << endl;

  delete g;
  return result;
}

int main(){
  test_space_combat("corsair", "fighter", 4);
  test_space_combat("battleship", "fighter", 1.5);
  test_space_combat("destroyer", "battleship", 1.5);
  test_space_combat("destroyer", "corsair", 2);
  test_space_combat("fighter", "destroyer", 1.2);  
  return 0;
}
