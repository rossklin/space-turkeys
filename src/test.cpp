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

typedef pair<set<string>, hm_t<string, int> > cost_tracker;
cost_tracker gather(cost_tracker x) {
  if (x.first.empty() && x.second.empty()) return x;
  
  cost_tracker buf;

  auto add = [&buf] (development::node &t) {
    buf.first += t.depends_techs;
    for (auto v : t.depends_facilities) buf.second[v.first] = v.second;
  };
  
  // gather sub techs
  for (auto v : x.first) {
    research::tech t = research::data::table().at(v);
    add(t);
  }

  // gather sub facilities
  for (auto v : x.second) {
    facility f = solar::facility_table().at(v.first);
    add(f);
  }

  buf = gather(buf);
  buf.first += x.first;
  for (auto v : x.second) buf.second[v.first] = max(buf.second[v.first], v.second);

  return buf;
}

float get_cost(cost_tracker x) {
  cost_tracker data = gather(x);  
  float res = 0;
  
  for (auto v : data.second) {
    facility f = solar::facility_table().at(v.first);
    float lm = cost::expansion_multiplier(v.second);
    float fc = f.cost_resources.count();
    res += v.second * fc * lm / 2;
  }  

  return res;
}

float fair_ship_count(string ship_class, set<string> techs, float limit, stringstream &ss) {
  ship_stats s = ship::table().at(ship_class);
  hm_t<string, int> facilities;
  
  if (!s.depends_tech.empty()) techs.insert(s.depends_tech);
  if (s.depends_facility_level > 0) facilities["shipyard"] = s.depends_facility_level;
  
  float investment = get_cost(make_pair(techs, facilities));
  int can_build = max(floor((limit - investment) / s.build_cost.count()), 0);

  ss << ship_class << ": investment: " << investment << ", can build: " << can_build << endl;

  return can_build;
}

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(string c0, string c1, float limit, float win_lower, float win_upper, set<string> add_techs = {}){
  stringstream ss;
  ss << "----------------------------------------" << endl;

  float count0 = fair_ship_count(c0, add_techs, limit, ss);
  float count1 = fair_ship_count(c1, add_techs, limit, ss);
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
  hm_t<string, set<string> > add_upgrades;

  for (auto t : add_techs) {
    add_upgrades[c0] += research::data::get_tech_upgrades(c0, t);
    add_upgrades[c1] += research::data::get_tech_upgrades(c1, t);
  }
  
  string game_stage = "early";
  if (limit > 4000) game_stage = "mid";
  if (limit > 10000) game_stage = "late";
  if (add_techs.count("hive fleet")) game_stage += "[HM]";

  ss << "test combat: " << game_stage << " game: " << scc_0[c0] << " " << c0 << " vs " << scc_1[c1] << " " << c1 << endl;

  auto test = [scc_0, scc_1, c0, c1, add_upgrades] () {
    game_settings set;
    set.starting_fleet = "massive";

    game_data *g = test_setup(set);
  
    point a(100, 100);
    point b(200, 100);  
    setup_fleet_for(g, 0, a, b, scc_0);
    setup_fleet_for(g, 1, b, a, scc_1);

    for (auto s : g -> all<ship>()) {
      if (add_upgrades.count(s -> ship_class)) {
	s -> upgrades += add_upgrades.at(s -> ship_class);
      }
    }

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

  set<string> add_techs = {
    "warp technology",
    "shields",
    "plasma technology"
  };

  // test mid game balance
  limit = 5000;
  tests.push_back(thread([limit] () {test_space_combat("corsair", "fighter", limit, 5, 16);}));
  tests.push_back(thread([limit] () {test_space_combat("battleship", "fighter", limit, 1.5, 4);}));
  tests.push_back(thread([limit] () {test_space_combat("battleship", "corsair", limit, 3, 6);}));

  // test late game balance
  limit = 15000;
  add_techs.insert("nano plating");
  add_techs.insert("nano weapons");
  
  tests.push_back(thread([limit, add_techs] () {test_space_combat("corsair", "fighter", limit, 6, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("fighter", "destroyer", limit, 2, 6, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("destroyer", "battleship", limit, 6, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("destroyer", "corsair", limit, 6, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("battleship", "corsair", limit, 2, 8, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("battleship", "fighter", limit, 6, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("voyager", "fighter", limit, 6, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("voyager", "corsair", limit, 1.2, 4, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("destroyer", "voyager", limit, 8, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("battleship", "voyager", limit, 4, 8, add_techs);}));

  // late game with hive mind
  add_techs.insert("hive fleet");
  tests.push_back(thread([limit, add_techs] () {test_space_combat("corsair", "fighter", limit, 2, 6, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("battleship", "fighter", limit, 1.5, 3, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("fighter", "destroyer", limit, 6, 16, add_techs);}));
  tests.push_back(thread([limit, add_techs] () {test_space_combat("fighter", "voyager", limit, 2, 6, add_techs);}));
  
  for (auto &t : tests) t.join();
  
  return 0;
}
