#include <unistd.h>

#include <iostream>
#include <thread>

#include "fleet.h"
#include "game_data.h"
#include "research.h"
#include "ship.h"
#include "types.h"
#include "waypoint.h"

#define max(a, b) ((a) > (b) ? (a) : (b))

using namespace std;
using namespace st3;

typedef unsigned long long int long_t;

long_t check_memory() {
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGE_SIZE);
  return pages * page_size;
}

game_data *test_setup(game_settings set = game_settings(), research::data r = research::data()) {
  game_data *g = new game_data();
  player p;
  p.research_level = r;
  g->settings = set;
  g->players[0] = p;
  g->players[1] = p;
  g->allocate_grid();
  return g;
}

void setup_fleet_for(game_data *g, idtype pid, point at, point targ, hm_t<string, float> scc = hm_t<string, float>()) {
  static int idc = 0;

  list<combid> ships;
  auto rbase = g->players[pid].research_level;

  for (auto sc : scc) {
    int count = max(round(utility::random_normal(sc.second, 0.5)), 1);
    for (int j = 0; j < count; j++) {
      ship_ptr sh = rbase.build_ship(g->next_id(ship::class_id), sc.first);
      ships.push_back(sh->id);
      sh->owner = pid;
      sh->states.insert("landed");
      g->register_entity(sh);
    }
  }

  waypoint_ptr w = waypoint::create(idc++, pid);
  w->position = targ;
  command c;
  c.target = w->id;
  c.action = fleet_action::go_to;
  c.policy = fleet::policy_aggressive;

  g->register_entity(w);
  g->generate_fleet(at, pid, c, ships, true);
  for (auto sid : ships) g->get_ship(sid)->states.erase("landed");
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
  vector<game_base_data> frames(n);
  game_data *g = test_setup();
  test_setup_fleets(g);

  long_t mem_start = check_memory();

  for (int j = 0; j < rep; j++) {
    frames.resize(n);
    for (int i = 0; i < n; i++) {
      g->increment();
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

  auto add = [&buf](development::node &t) {
    buf.first += t.depends_techs;
    for (auto v : t.depends_facilities) buf.second[v.first] = v.second;
  };

  // gather sub techs
  for (auto v : x.first) {
    research::tech t = research::data::table().at(v);
    add(t);
  }

  // // gather sub facilities
  // for (auto v : x.second) {
  //   facility f = solar::facility_table().at(v.first);
  //   add(f);
  // }

  buf = gather(buf);
  buf.first += x.first;
  for (auto v : x.second) buf.second[v.first] = max(buf.second[v.first], v.second);

  return buf;
}

float get_cost(cost_tracker x) {
  cost_tracker data = gather(x);
  float res = 0;

  for (auto v : data.second) {
    for (int l = 1; l < v.second; l++) res += 4 * pow(4, l);
  }

  return res;
}

float fair_ship_count(string ship_class, set<string> techs, float limit, stringstream &ss) {
  ship_stats s = ship::table().at(ship_class);
  hm_t<string, int> facilities;

  if (!s.depends_tech.empty()) techs.insert(s.depends_tech);
  if (s.depends_facility_level > 0) facilities["shipyard"] = s.depends_facility_level;

  // 3000 res = 1000 t + 100 res + 500 t
  // 1 time = 2 res

  float investment = get_cost(make_pair(techs, facilities));
  float can_build = max((limit - investment) / (s.build_time + 0.5 * s.build_cost.count()), 0);

  ss << ship_class << ": investment: " << investment << ", can build: " << can_build << endl;

  return can_build;
}

// test two initial fleets can kill each other within 1000 increments
bool test_space_combat(string c0, string c1, float limit, pair<float, float> margins, set<string> add_techs = {}) {
  float win_lower = margins.first;
  float win_upper = margins.second;

  stringstream ss;
  ss << "----------------------------------------" << endl;

  float count0 = fair_ship_count(c0, add_techs, limit, ss);
  float count1 = fair_ship_count(c1, add_techs, limit, ss);
  int min_units = 10;

  float highest = fmax(count0, count1);
  float lowest = fmin(count0, count1);

  if (lowest < 1) {
    ss << c0 << " vs " << c1 << " limit " << limit << ": less than one ship, skipping";
    server::output(ss.str(), true);
    return false;
  }

  float ratio = min_units / lowest;
  count0 *= ratio;
  count1 *= ratio;

  hm_t<string, float> scc_0 = {{c0, count0}};
  hm_t<string, float> scc_1 = {{c1, count1}};
  hm_t<string, set<string> > add_upgrades;

  for (auto t : add_techs) {
    add_upgrades[c0] += research::data::get_tech_upgrades(c0, t);
    add_upgrades[c1] += research::data::get_tech_upgrades(c1, t);
  }

  string game_stage = "EARLY";
  if (limit > 300) game_stage = "MID";
  if (limit > 1000) game_stage = "LATE";
  if (add_techs.count("hive fleet")) game_stage += "HM";
  if (add_techs.count("A.M. Laser")) game_stage += "SPLASH";

  ss << "TEST COMBAT: " << game_stage << " GAME: " << scc_0[c0] << " " << c0 << " vs " << scc_1[c1] << " " << c1 << endl;

  auto test = [scc_0, scc_1, c0, c1, add_techs]() {
    game_settings set;
    set.enable_extend = false;

    research::data r;
    for (auto v : add_techs) r.access(v).level = 1;

    game_data *g = test_setup(set, r);

    point a(100, 100);
    point b(200, 100);
    setup_fleet_for(g, 0, a, b, scc_0);
    setup_fleet_for(g, 1, b, a, scc_1);

    // for (auto s : g -> all<ship>()) {
    //   if (add_upgrades.count(s -> ship_class)) {
    // 	s -> upgrades += add_upgrades.at(s -> ship_class);
    //   }
    // }

    auto get_ratio = [&](int pid) -> float {
      float ref = pid == 0 ? scc_0.at(c0) : scc_1.at(c1);
      for (auto f : g->filtered_entities<fleet>()) {
        if (f->owner == pid) {
          return f->ships.size() / ref;
        }
      }
      return 0;
    };

    // run game mechanics until one fleet is destroyed
    int count = 0;
    float escape = 0.1;
    while (get_ratio(0) > escape && get_ratio(1) > escape && count++ < 1000) {
      g->increment();
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
    if (r > 100 || !isfinite(r)) r = 100;

    rmean = (nsample * rmean + r) / (nsample + 1);
    dev = (nsample * dev + abs(r - rmean)) / (nsample + 1);
    nsample++;

    float test_value = fmin(abs(rmean - win_lower), abs(rmean - win_upper));
    significant = test_value > 3 * dev;

    ss << r << ": " << test_value << "[" << dev << "] ";
  }
  ss << endl;

  bool result = rmean >= win_lower && rmean <= win_upper;

  ss << rmean << ": " << (result ? "SUCCESS" : "FAILURE") << endl;

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

int main(int argc, char **argv) {
  float limit;

  game_data::confirm_data();

  if (argc == 6) {
    test_space_combat(argv[1], argv[2], stof(argv[3]), make_pair(stof(argv[4]), stof(argv[5])));
    return 0;
  } else if (argc > 1) {
    cout << "Usage: " << argv[0] << " ship1 ship2 res_limit win_lower win_upper" << endl;
    return 0;
  }

  typedef function<void(void)> test_f;
  list<thread> tests;
  pair<float, float> v_tiny = {1.1, 1.4}, v_small = {1.4, 1.7}, v_medium = {1.7, 2}, v_large = {2, 3}, v_huge = {3, 10}, v_extreme = {10, 100};

  // test early game balance
  limit = 1000;
  tests.push_back(thread([=]() { test_space_combat("fighter", "scout", limit, v_huge); }));

  set<string> add_techs = {
      "ionized hulls",
      "plasma weapons",
      "shield technology",
      "warp technology"};

  // test mid game balance
  limit = 10000;
  tests.push_back(thread([=]() { test_space_combat("corsair", "fighter", limit, v_large, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("fighter", "battleship", limit, v_medium, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("battleship", "corsair", limit, v_medium, add_techs); }));

  // test late game balance
  limit = 40000;
  add_techs.insert("micro torpedos");

  tests.push_back(thread([=]() { test_space_combat("corsair", "fighter", limit, v_extreme, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("fighter", "destroyer", limit, v_huge, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("destroyer", "battleship", limit, v_huge, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("destroyer", "corsair", limit, v_huge, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("battleship", "corsair", limit, v_large, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("battleship", "fighter", limit, v_large, add_techs); }));

  tests.push_back(thread([=]() { test_space_combat("voyager", "fighter", limit, v_small, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("voyager", "corsair", limit, v_small, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("destroyer", "voyager", limit, v_extreme, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("battleship", "voyager", limit, v_huge, add_techs); }));

  // late game with hive mind
  add_techs.insert("hive fleet");
  limit = 60000;
  tests.push_back(thread([=]() { test_space_combat("corsair", "fighter", limit, v_medium, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("fighter", "battleship", limit, v_medium, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("fighter", "destroyer", limit, v_extreme, add_techs); }));

  // late game with splash
  add_techs.insert("A.M. Laser");
  add_techs.erase("hive fleet");
  tests.push_back(thread([=]() { test_space_combat("battleship", "fighter", limit, v_extreme, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("destroyer", "fighter", limit, v_huge, add_techs); }));

  // late game with hive mind and splash
  add_techs.insert("hive fleet");
  tests.push_back(thread([=]() { test_space_combat("battleship", "fighter", limit, v_large, add_techs); }));
  tests.push_back(thread([=]() { test_space_combat("fighter", "destroyer", limit, v_huge, add_techs); }));

  for (auto &t : tests) t.join();

  return 0;
}
