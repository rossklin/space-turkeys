#include <iostream>
#include <unistd.h>

#include "game_data.h"
#include "research.h"

#define max(a,b) ((a) > (b) ? (a) : (b))

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
  return c;
}

cost::res_t get_tech_cost(string tech) {
  research::tech t = research::data::table().at(tech);
  cost::res_t c;

  for (auto v : t.depends_facilities) c.add(get_facility_cost(v.first, v.second));
  return c;
}

float fair_ship_count(string ship_class, float limit) {
  cost::res_t investment_cost;
  ship_stats s = ship::table().at(ship_class);
  investment_cost.add(get_tech_cost(s.depends_tech));
  investment_cost.add(get_facility_cost("shipyard", s.depends_facility_level));

  float investment = investment_cost.count();
  int can_build = max(floor((limit - investment) / s.build_cost.count()), 0);

  cout << ship_class << ": investment: " << investment << ", can build: " << can_build << endl;

  return can_build;
}

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(string c0, string c1, float limit, float req_win){
  cout << "****************************************" << endl;

  float count0 = fair_ship_count(c0, limit);
  float count1 = fair_ship_count(c1, limit);
  float highest = fmax(count0, count1);
  if (highest > 99) {
    float ratio = 99 / highest;
    count0 *= ratio;
    count1 *= ratio;
  }

  hm_t<string, int> scc_0 = {{c0, ceil(count0)}};
  hm_t<string, int> scc_1 = {{c1, ceil(count1)}};

  cout << "test combat: " << scc_0[c0] << " " << c0 << " vs " << scc_1[c1] << " " << c1 << endl;

  auto test = [scc_0, scc_1, c0, c1] () {
    game_settings set;
    set.starting_fleet = "massive";

    game_data *g = test_setup(set);
  
    point a(100, 100);
    point b(200, 100);  
    setup_fleet_for(g, 0, a, b, scc_0);
    setup_fleet_for(g, 1, b, a, scc_1);

    auto get_ratio = [&] (int pid) -> float {
      float ref = pid == 0 ? scc_0.at(c0) : scc_1.at(c1);
      for (auto f : g -> all<fleet>()) {
	if (f -> owner == pid) {
	  return f -> ships.size() / ref;
	} 
      }
      return 0;
    };

    // run game mechanics until one fleet is destroyed
    int count = 0;
    float escape = 0.1;
    while (get_ratio(0) > escape && get_ratio(1) > escape && count++ < 1000){
      g -> increment();
    }

    float r0 = get_ratio(0);
    float r1 = get_ratio(1);

    delete g;
    return r0 / r1;
  };

  float rmean = 0;
  int nsample = 0;
  float dev = 1;
  int max_sample = 10;
  int count = 0;

  cout << "sample: ";
  while (count++ < max_sample && (rmean == 0 || dev >= 0.25 * rmean)) {
    float r = test();
    if (!isfinite(r)) r = 100;
    
    dev = (nsample * dev + abs(r - rmean)) / (nsample + 1);
    rmean = (nsample * rmean + r) / (nsample + 1);
    nsample++;
    cout << r << " (" << dev << "), " << flush;
  }
  
  cout << endl;
  bool result = rmean >= req_win;

  cout << (result ? "SUCCESS" : "FAILURE") << ": result: " << rmean << " (req: " << req_win << ")" << endl;
  return result;
}

int main(){
  float limit = 2000;

  // test early game balance
  cout << "EARLY GAME" << endl;
  test_space_combat("fighter", "battleship", limit, 1.5);

  // test mid game balance
  cout << "MID GAME" << endl;
  limit = 5000;
  test_space_combat("corsair", "fighter", limit, 4);
  test_space_combat("fighter", "battleship", limit, 1.5);
  test_space_combat("battleship", "corsair", limit, 4);

  // test late game balance
  cout << "LATE GAME" << endl;
  limit = 15000;
  test_space_combat("corsair", "fighter", limit, 4);
  test_space_combat("fighter", "destroyer", limit, 4);
  test_space_combat("destroyer", "battleship", limit, 4);
  test_space_combat("destroyer", "corsair", limit, 8);
  test_space_combat("battleship", "corsair", limit, 4);
  test_space_combat("battleship", "fighter", limit, 1.5);
  test_space_combat("voyager", "fighter", limit, 1.5);
  test_space_combat("voyager", "corsair", limit, 1.5);
  test_space_combat("destroyer", "voyager", limit, 8);
  test_space_combat("battleship", "voyager", limit, 4);
  
  return 0;
}
