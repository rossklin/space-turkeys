#include <iostream>

#include "interaction.h"
#include "fleet.h"
#include "ship.h"
#include "utility.h"
#include "game_data.h"
#include "serialization.h"
#include "upgrades.h"

using namespace std;
using namespace st3;

const string fleet::class_id = "fleet";

const string fleet_action::go_to = "go to";
const string fleet_action::idle = "idle";

const sint fleet::policy_aggressive = 1;
const sint fleet::policy_reasonable = 2;
const sint fleet::policy_evasive = 4;
const sint fleet::policy_maintain_course = 8;

fleet::fleet(idtype pid){
  static int idc = 0;
  if (pid < 0){
    id = identifier::make(class_id, "S#" + to_string(idc++));
  }else{
    id = identifier::make(class_id, to_string(pid) + "#" + to_string(idc++));
  }

  radius = 0;
  position = point(0,0);
  update_counter = 0;
  owner = -1;
  remove = false;
  com.action = fleet_action::idle;
}

fleet::fleet(const fleet &x) : game_object(x) {
  *this = x;
}

sint fleet::default_policy(string action) {
  if (action == interaction::space_combat) {
    return policy_aggressive;
  } else {
    return policy_reasonable;
  }
}

void fleet::on_remove(game_data *g) {
  for (auto sid : ships) {
    g -> get_ship(sid) -> fleet_id = identifier::source_none;
  }
  game_object::on_remove(g);
}

void fleet::pre_phase(game_data *g){
  check_in_sight(g);
  update_data(g);
  check_action(g);
  check_waypoint(g);
}

void fleet::move(game_data *g){
  // linear extrapolation of position estimate
  if (!is_idle()) position += stats.speed_limit * utility::normv(utility::point_angle(stats.target_position - position));
}

void fleet::post_phase(game_data *g){}

float fleet::vision(){
  return stats.vision_buf;
}

fleet::ptr fleet::create(idtype pid){
  return ptr(new fleet(pid));
}

game_object::ptr fleet::clone(){
  return new fleet(*this);
}

bool fleet::serialize(sf::Packet &p){
  return p << class_id << *this;
}

bool st3::fleet::is_idle(){
  return com.action == fleet_action::idle;
}

void fleet::set_idle(){
  com.target = identifier::target_idle;
  com.action = fleet_action::idle;
}

void fleet::give_commands(list<command> c, game_data *g){
  vector<command> buf(c.begin(), c.end());
  random_shuffle(buf.begin(), buf.end());

  for (auto &x : buf){
    ptr f = create(fleet::server_pid);
    f -> com = x;
    f -> com.source = f -> id;
    if (f -> com.origin.empty()) f -> com.origin = com.origin;
    f -> position = position;
    f -> radius = radius;
    f -> owner = owner;
    for (auto i : x.ships){
      if (ships.count(i)){
	ship::ptr buf = g -> get_ship(i);
	buf -> fleet_id = f -> id;
	remove_ship(i);
	f -> ships.insert(i);
      }
    }

    g -> add_entity(f);
    f -> update_data(g, true);
  }
}

fleet::suggestion fleet::suggest(combid sid, game_data *g) {
  ship::ptr s = g -> get_ship(sid);
  float pref_density = 0.1;
  float pref_maxrad = fmax(sqrt(ships.size() / (M_PI * pref_density)), 20);

  auto local_output = [this] (string v) {
#ifdef VERBOSE
    output(id + ": suggest: " + v);
#endif
  };

  int unmask = 0;
  if (stats.converge) unmask |= suggestion::activate;
  
  if (stats.enemies.size()) {
    suggestion s_evade(suggestion::evade | unmask, stats.path);
    if (!stats.can_evade) s_evade.id = suggestion::scatter;
    
    if (com.policy & policy_maintain_course) {
      local_output("maintain course");
      return suggestion(suggestion::travel | unmask, stats.target_position);
    }else if (com.policy & policy_evasive) {
      local_output("evade");
      return s_evade;
    }else{    
      if (s -> tags.count("spacecombat")) {
	point p = stats.enemies.front().first;
	if (com.policy & policy_aggressive) {
	  local_output("aggressive engage: " + utility::point2string(p));
	  return suggestion(suggestion::engage | unmask, p);
	} else if (com.policy & policy_reasonable && stats.enemies.front().second < 1){
	  local_output("local engage");
	  return suggestion(suggestion::engage | unmask, p);
	} else {
	  local_output("evade");
	  return s_evade;
	}
      }else{
	local_output("evade");
	return s_evade;
      }
    }
  }else{
    // peaceful times
    if (stats.converge) {
      local_output("activate");
      return suggestion(suggestion::activate | unmask, stats.target_position);
    }else if (utility::l2norm(s -> position - position) > pref_maxrad) {
      if (is_idle()) {
	local_output("summon");
	return suggestion(suggestion::summon | unmask);
      } else {
	local_output("summon and travel");
	return suggestion(suggestion::summon | suggestion::travel | unmask, stats.target_position);
      }
    }else if (is_idle()) {
      local_output("hold");
      return suggestion(suggestion::hold | unmask);
    }else{
      local_output("travel");
      return suggestion(suggestion::travel | unmask, stats.target_position);
    }
  }
}

// cluster enemy ships
void fleet::analyze_enemies(game_data *g) {
  stats.enemies.clear();
  float r = stats.spread_radius + vision();
  target_condition cond(target_condition::enemy, ship::class_id);
  list<combid> t = g -> search_targets_nophys(id, position, r, cond.owned_by(owner));
  vector<point> pos = utility::fmap<vector<point> >(t, (function<point(combid)>) [g] (combid sid) -> point {return g -> get_ship(sid) -> position;});
  vector<point> x;

  auto local_output = [this] (string v) {
#ifdef VERBOSE
    output(id + ": analyze enemies: " + v);
#endif
  };
  
  if (t.empty()) {
    stats.can_evade = false;
    local_output("no enemies");
    return;
  }

  x = utility::cluster_points(pos);

  local_output("identified " + to_string(x.size()) + " clusters from " + to_string(t.size()) + " ships.");

  int na = 10;

  // assign ships to clusters
  hm_t<int, float> cc;
  vector<float> scatter_data(na, 1);
  float dps_scale = g -> settings.frames_per_round / get_hp();
  for (auto sid : t) {
    ship s(ship::table().at(g -> get_ship(sid) -> ship_class));
    point p = g -> get_ship(sid) -> position;

    // scatter value
    int scatter_idx = utility::angle2index(na,utility::point_angle(p - position));
    scatter_data[scatter_idx] += s.get_dps() * dps_scale;

    // cluster assignment
    int idx = utility::vector_min<point>(x, [p] (point y) -> float {return utility::l2d2(y - p);});
    cc[idx] += s.get_strength();

    local_output("assigned enemy " + sid + " to cluster " + to_string(idx) + " at " + utility::point2string(x[idx]));
  }

  // build enemy fleet stats
  for (auto i : cc) {
    stats.enemies.push_back(make_pair(x[i.first], i.second / get_strength()));
    local_output("added enemy cluster value: " + utility::point2string(x[i.first]) + ": " + to_string(i.second / get_strength()));
  }
  
  stats.enemies.sort([](pair<point, float> a, pair<point, float> b) {return a.second > b.second;});

  // find best evade direction

  // define prioritized directions
  vector<float> dw(na, 1);
  int idx_previous = utility::angle2index(na, utility::point_angle(stats.path - position));
  int idx_target = utility::angle2index(na, utility::point_angle(stats.target_position - position));
  int idx_closest = 0;

  solar::ptr closest_solar = g -> closest_solar(position, owner);
  if (closest_solar) {
    idx_closest = utility::angle2index(na, utility::point_angle(closest_solar -> position - position));
    dw[idx_closest] = 0.8;
  }

  if (stats.can_evade) dw[idx_previous] = 0.7;

  dw[idx_target] = 0.5;

  // merge direction priorities with enemy strength data via circular kernel
  vector<float> heuristics = utility::elementwise_product(utility::circular_kernel(scatter_data, 2), utility::circular_kernel(dw, 1));

  int prio_idx = utility::vector_min<float>(heuristics, utility::identity_function<float>());
  float evalue = heuristics[prio_idx];

  // only allow evasion if there exists a path with low enemy strength
  if (evalue < 1) {
    stats.can_evade = true;
    stats.path = position + utility::scale_point(utility::normv(utility::index2angle(na, prio_idx)), 100);
    local_output("selected evasion angle: " + to_string(utility::index2angle(na, prio_idx)));
  } else {
    stats.can_evade = false;
    local_output("no good evasion priority index (best was: " + to_string(utility::index2angle(na, prio_idx)) + " at " + to_string(evalue));
  }
}

void fleet::update_data(game_data *g, bool force_refresh){

  // need to update fleet data?
  bool should_update = force_refresh || utility::random_uniform() < 1 / (float)fleet::update_period;
  if (!should_update) return;
  
  float speed = INFINITY;
  int count;

  // position, radius, speed and vision
  point p(0,0);
  float r2 = 0;
  stats.vision_buf = 0;

  for (auto k : ships) {
    ship::ptr s = g -> get_ship(k);
    p = p + s -> position;
  }
  
  radius = g -> settings.fleet_default_radius;
  position = utility::scale_point(p, 1 / (float)ships.size());

  stats.average_ship = ssfloat_t();
  for (auto k : ships){
    ship::ptr s = g -> get_ship(k);
    speed = fmin(speed, s -> base_stats.stats[sskey::key::speed]);
    r2 = fmax(r2, utility::l2d2(s -> position - position));
    stats.vision_buf = fmax(stats.vision_buf, s -> vision());
    for (int i = 0; i < sskey::key::count; i++) stats.average_ship.stats[i] += s -> stats[i];
  }
  
  for (int i = 0; i < sskey::key::count; i++) stats.average_ship.stats[i] /= ships.size();
  stats.spread_radius = fmax(sqrt(r2), fleet::min_radius);
  stats.spread_density = ships.size() / (M_PI * pow(stats.spread_radius, 2));
  stats.speed_limit = 0.9 * speed;

  analyze_enemies(g);
  check_action(g);
  refresh_ships(g);
}

void fleet::refresh_ships(game_data *g) {
  // make ships update data
  for (auto sid : ships) g -> get_ship(sid) -> force_refresh = true;
}

void fleet::check_action(game_data *g) {
  bool should_refresh = false;
  
  if (com.target != identifier::target_idle) {
    // check target status valid if action is an interaction
    auto itab = interaction::table();
    if (itab.count(com.action)) {
      target_condition c = itab[com.action].condition.owned_by(owner);
      if (!c.valid_on(g -> get_entity(com.target))) {
	cout << "target " << com.target << " no longer valid for " << id << endl;
	set_idle();
	should_refresh = true;
      }
    }

    // have arrived?
    if (g -> target_position(com.target, stats.target_position)) {
      bool buf = utility::l2d2(stats.target_position - position) < fleet::interact_d2;
      should_refresh |= buf != stats.converge;
      stats.converge = buf;
    }
  }
  
  if (should_refresh) refresh_ships(g);
}

void fleet::check_waypoint(game_data *g){
  // set to idle and 'land' ships if converged to waypoint
  if (identifier::get_type(com.target) == waypoint::class_id){
    if (stats.converge) {
      com.action = fleet_action::idle;
    } else {
      com.action = fleet_action::go_to;
    }
  }
}

void fleet::check_in_sight(game_data *g){
  if (com.target == identifier::target_idle) return;

  bool seen = g -> evm[owner].count(com.target);
  bool solar = identifier::get_type(com.target) == solar::class_id;
  bool owned = g -> entity.count(com.target) && g -> get_entity(com.target) -> owner == owner;

  // target left sight?
  if (!(seen || solar || owned)){
    cout << "fleet " << id << " looses sight of " << com.target << endl;

    // creating a waypoint here causes id collision with waypoints
    // created on client side
    
    // // create a waypoint and reset target
    // waypoint::ptr w = waypoint::create(owner);

    // // if the target exists, set waypoint at  target position
    // g -> target_position(com.target, w -> position);
    // g -> add_entity(w);

    set_idle();
  }
}

void fleet::remove_ship(combid i){
  ships.erase(i);
  remove = ships.empty();
}

bool fleet::isa(string c) {
  return c == fleet::class_id || c == commandable_object::class_id;
}

fleet::analytics::analytics() {
  converge = false;
  speed_limit = 0;
  spread_radius = 0;
  spread_density = 0;
  target_position = point(0,0);
  path = point(0,0);
  can_evade = false;
  vision_buf = 0;
}

float fleet::get_dps() {
  return stats.average_ship.get_dps() * ships.size();
}

float fleet::get_hp() {
  return stats.average_ship.get_hp() * ships.size();
}

float fleet::get_strength() {
  return stats.average_ship.get_dps() * stats.average_ship.get_hp() * ships.size();
}

fleet::suggestion::suggestion() {
  id = 0;
}

fleet::suggestion::suggestion(sint i) {
  id = i;
}

fleet::suggestion::suggestion(sint i, point x) {
  id = i;
  p = x;
}

