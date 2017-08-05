#include <iostream>
#include <unistd.h>
#include <thread>

#include "types.h"
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
    server::output(to_string(j) + ": free: " + to_string(mtest) + ", used: " + to_string(mem_start - mtest));
  }

  delete g;

  return true;
}

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

float fair_ship_count(string ship_class, float limit, stringstream &ss) {
  cost::res_t investment_cost;
  ship_stats s = ship::table().at(ship_class);
  investment_cost.add(get_tech_cost(s.depends_tech));
  investment_cost.add(get_facility_cost("shipyard", s.depends_facility_level));

  float investment = investment_cost.count();
  int can_build = max(floor((limit - investment) / s.build_cost.count()), 0);

  ss << ship_class << ": investment: " << investment << ", can build: " << can_build << endl;

  return can_build;
}

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(string c0, string c1, float limit, float win_lower, float win_upper, set<string> add_upgrades = {}){
  stringstream ss;
  ss << "----------------------------------------" << endl;

  float count0 = fair_ship_count(c0, limit, ss);
  float count1 = fair_ship_count(c1, limit, ss);
  int min_units = 10;
  int max_units = 100;
  
  float highest = fmax(count0, count1);
  if (highest > max_units) {
    float ratio = max_units / highest;
    count0 *= ratio;
    count1 *= ratio;
  }

  float lowest = fmin(count0, count1);
  if (lowest > min_units) {
    float ratio = min_units / lowest;
    count0 *= ratio;
    count1 *= ratio;
  }

  hm_t<string, int> scc_0 = {{c0, ceil(count0)}};
  hm_t<string, int> scc_1 = {{c1, ceil(count1)}};
  string game_stage = "early";
  if (limit > 4000) game_stage = "mid";
  if (limit > 10000) game_stage = "late";
  if (add_upgrades.count("hive mind")) game_stage += "[HM]";

  ss << "test combat: " << game_stage << " game: " << scc_0[c0] << " " << c0 << " vs " << scc_1[c1] << " " << c1 << endl;

  auto test = [scc_0, scc_1, c0, c1, add_upgrades] () {
    game_settings set;
    set.starting_fleet = "massive";

    game_data *g = test_setup(set);
  
    point a(100, 100);
    point b(200, 100);  
    setup_fleet_for(g, 0, a, b, scc_0);
    setup_fleet_for(g, 1, b, a, scc_1);

    for (auto s : g -> all<ship>()) s -> upgrades += add_upgrades;

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
  float dev = 0;
  int max_sample = 40;
  bool significant = false;

  ss << "sample: ";
  while (nsample < max_sample && (nsample < 10 || !significant)) {
    float r = test();
    if (r > 10 || !isfinite(r)) r = 10;
    
    rmean = (nsample * rmean + r) / (nsample + 1);
    dev = (nsample * dev + abs(r - rmean)) / (nsample + 1);
    nsample++;

    float test_value = fmin(abs(rmean - win_lower), abs(rmean - win_upper));
    significant = test_value > 3 * dev;

    ss << r << ": " << test_value << "[" << dev << "] ";
  }
  ss << endl;
  
  bool result = rmean >= win_lower && rmean <= win_upper;

  ss << rmean << ": "  << (result ? "SUCCESS" : "FAILURE") << endl;

  if (!result) {
    ss << "########################################" << endl;
    if (rmean < win_lower) {
      ss << ">> [" << win_lower << "] IMPROVE " << c0 << " VS " << c1 << "<<" << endl;
    } else {
      ss << ">>[" << win_upper << "] overkill, NERF " << c0 << " VS " << c1 << "<<" << endl;
    }
    ss << "########################################" << endl;
  }

  server::output(ss.str(), true);
  
  return result;
}

int main(int argc, char **argv){
  float limit = 2000;

  game_data::confirm_data();

  if (argc == 6) {
    test_space_combat(argv[1], argv[2], stof(argv[3]), stof(argv[4]), stof(argv[5]));
    return 0;
  } else if (argc > 1) {
    cout << "Usage: " << argv[0] << " ship1 ship2 res_limit win_lower win_upper" << endl;
    return 0;
  }

  typedef function<void(void)> test_f;
  list<thread> tests;

  // test early game balance
  tests.push_back(thread([limit] () {test_space_combat("fighter", "battleship", limit, 4, 16);}));

  set<string> add_upgrades = {
    "warp drive",
    "shield",
    "plasma laser"
  };

  // test mid game balance
  limit = 5000;
  tests.push_back(thread([limit] () {test_space_combat("corsair", "fighter", limit, 5, 16);}));
  tests.push_back(thread([limit] () {test_space_combat("battleship", "fighter", limit, 1.5, 4);}));
  tests.push_back(thread([limit] () {test_space_combat("battleship", "corsair", limit, 3, 6);}));

  // test late game balance
  limit = 15000;
  add_upgrades = {
    "warp drive",
    "shield",
    "nano hull",
    "nano torpedos",
    "plasma laser"
  };
  
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("corsair", "fighter", limit, 6, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("fighter", "destroyer", limit, 2, 6, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("destroyer", "battleship", limit, 6, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("destroyer", "corsair", limit, 6, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("battleship", "corsair", limit, 2, 8, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("battleship", "fighter", limit, 6, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("voyager", "fighter", limit, 6, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("voyager", "corsair", limit, 1.2, 4, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("destroyer", "voyager", limit, 8, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("battleship", "voyager", limit, 4, 8, add_upgrades);}));

  // late game with hive mind
  add_upgrades.insert("hive mind");
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("corsair", "fighter", limit, 2, 6, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("battleship", "fighter", limit, 1.5, 3, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("fighter", "destroyer", limit, 6, 16, add_upgrades);}));
  tests.push_back(thread([limit, add_upgrades] () {test_space_combat("fighter", "voyager", limit, 2, 6, add_upgrades);}));
  
  for (auto &t : tests) t.join();
  
  return 0;
}
