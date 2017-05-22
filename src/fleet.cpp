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

fleet::fleet(){}

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

fleet::~fleet(){}

void fleet::on_remove(game_data *g) {
  for (auto sid : ships) {
    g -> get_ship(sid) -> fleet_id = identifier::source_none;
  }
  game_object::on_remove(g);
}

void fleet::pre_phase(game_data *g){
  check_in_sight(g);
  update_data(g);
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

fleet::ptr fleet::clone(){
  return dynamic_cast<fleet::ptr>(clone_impl());
}

game_object::ptr fleet::clone_impl(){
  return ptr(new fleet(*this));
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
  }
}

fleet::suggestion fleet::suggest(combid sid, game_data *g) {
  ship::ptr s = g -> get_ship(sid);
  float pref_density = 0.2;
  float pref_maxrad = fmax(sqrt(ships.size() / (M_PI * pref_density)), 10);
  
  if (stats.enemies.size()) {
    suggestion s_evade(suggestion::evade, stats.path);
    if (!stats.can_evade) s_evade.id = suggestion::scatter;
    
    if (com.policy & policy_maintain_course) {
      return suggestion(suggestion::travel, stats.target_position);
    }else if (com.policy & policy_evasive) {
      return s_evade;
    }else{    
      if (s -> tags.count("spacecombat")) {
	if (com.policy & policy_aggressive) {
	  return suggestion(suggestion::engage, stats.enemies.front().first);
	} else if (com.policy & policy_reasonable && stats.enemies.front().second < 1){
	  return suggestion(suggestion::engage, s -> position);
	} else {
	  return s_evade;
	}
      }else{
	return s_evade;
      }
    }
  }else{
    // peaceful times
    if (utility::l2norm(s -> position - position) > pref_maxrad) {
      if (is_idle()) {
	return suggestion::summon;
      } else {
	return suggestion(suggestion::summon | suggestion::travel, stats.target_position);
      }
    }else if (is_idle()) {
      return suggestion::hold;
    }else if (stats.converge) {
      return suggestion(suggestion::activate, stats.target_position);
    }else{
      return suggestion(suggestion::travel, stats.target_position);
    }
  }
}

// cluster enemy ships
void fleet::analyze_enemies(game_data *g) {
  stats.enemies.clear();
  float r = stats.spread_radius + vision();
  target_condition cond(target_condition::enemy, ship::class_id);
  list<combid> t = g -> search_targets_nophys(id, position, r, cond.owned_by(owner));
  int n = 10;
  int rep = 10;
  vector<point> x(n);

  if (t.empty()) {
    stats.can_evade = false;
    return;
  }

  for (int i = 0; i < n; i++) x[i] = g -> get_entity(utility::uniform_sample(t)) -> position;

  // identify cluster points
  for (int i = 0; i < rep; i++) {
    // select a random enemy ship s
    combid sid = utility::uniform_sample(t);
    ship::ptr s = g -> get_ship(sid);

    // sort points by distance to s
    std::sort(x.begin(), x.end(), [=] (point a, point b) -> bool {
	return utility::l2d2(s -> position - a) < utility::l2d2(s -> position - b);
      });

    // move points towards s
    for (int j = 0; j < x.size(); j++){
      point d = s -> position - x[j];
      float f = exp(-utility::l2norm(d)) / (i + 1);
      x[j] = x[j] + utility::scale_point(d, f);
    }

    // join adjacent points
    for (auto j = x.begin(); j != x.end(); j++) {
      for (auto k = j + 1; k != x.end(); k++) {
	if (utility::l2d2(*j - *k) < 100) x.erase(k--);
      }
    }
  }

  if (x.empty()) throw runtime_error("Failed to build enemy clusters!");
  if (x.size() == n) throw runtime_error("Analyze enemies: clusters failed to form!");

  int na = 10;

  // assign ships to clusters
  hm_t<int, float> cc;
  vector<float> scatter_data(na, 0);
  for (auto sid : t) {
    ship::ptr s = g -> get_ship(sid);
    float strength_value = s -> stats[sskey::key::mass] * s -> stats[sskey::key::ship_damage];

    // scatter value
    int scatter_idx = utility::angle2index(na,utility::point_angle(s -> position - position));
    scatter_data[scatter_idx] += strength_value;

    // cluster assignment
    int idx = utility::vector_min<point>(x, [s] (point y) -> float {return utility::l2d2(y - s -> position);});
    cc[idx] += strength_value;
  }

  // build enemy fleet stats
  for (auto i : cc) stats.enemies.push_back(make_pair(x[i.first], i.second / stats.self_strength));
  stats.enemies.sort([](pair<point, float> a, pair<point, float> b) {return a.second > b.second;});

  // find best evade direction

  // circular kernel smooth enemy strength data
  vector<float> s_buf = utility::circular_kernel(scatter_data);

  // preferred directions
  float val_prev = s_buf[utility::angle2index(na,utility::point_angle(stats.path - position))];
  if (!stats.can_evade) val_prev = INFINITY;
  solar::ptr closest_solar = g -> closest_solar(position, owner);
  point p_solar;
  float val_solar = INFINITY;
  if (closest_solar) {
    p_solar = closest_solar -> position;
    val_solar = s_buf[utility::angle2index(na,utility::point_angle(p_solar - position))];
  }
  float val_target = s_buf[utility::angle2index(na,utility::point_angle(stats.target_position - position))];

  // direction with lowest threat
  float best = INFINITY;
  float angle = -1;
  for (int i = 0; i < na; i++) {
    float v = s_buf[i];
    if (v < best) {
      best = v;
      angle = utility::index2angle(na,i);
    }
  }

  // weight directions by preference and danger
  point p_best = position + utility::scale_point(utility::normv(angle), 100);
  vector<pair<float, point> > priority(4);
  priority[0] = {best, p_best};
  priority[1] = {val_target / 2, stats.target_position};
  priority[2] = {val_prev / 1.6, stats.path};
  priority[3] = {val_solar / 1.4, p_solar};
    
  pair<float, point> select = priority[utility::vector_min<pair<float, point> >(priority, [](pair<float, point> x) -> float {return x.first;})];

  // only allow evasion if there exists a path with low enemy strength
  if (select.first < 0.8) {
    stats.can_evade = true;
    stats.path = select.second;
  } else {
    stats.can_evade = false;
  }
}

void fleet::update_data(game_data *g, bool force_refresh){

  // need to update fleet data?
  if (((update_counter++) % fleet::update_period) && !force_refresh) return;
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

  stats.self_strength = 0;
  for (auto k : ships){
    ship::ptr s = g -> get_ship(k);
    speed = fmin(speed, s -> base_stats.stats[sskey::key::speed]);
    r2 = fmax(r2, utility::l2d2(s -> position - position));
    stats.vision_buf = fmax(stats.vision_buf, s -> vision());
    stats.self_strength += s -> stats[sskey::key::mass] * s -> stats[sskey::key::ship_damage];
  }
  
  stats.spread_radius = fmax(sqrt(r2), fleet::min_radius);
  stats.spread_density = ships.size() / (M_PI * pow(stats.spread_radius, 2));
  stats.speed_limit = speed;

  analyze_enemies(g);

  // the below only applies if the fleet has a target
  if (com.target == identifier::target_idle) return;

  // check target status valid if action is an interaction
  auto itab = interaction::table();
  if (itab.count(com.action)) {
    target_condition c = itab[com.action].condition.owned_by(owner);
    if (!c.valid_on(g -> get_entity(com.target))) {
      cout << "target " << com.target << " no longer valid for " << id << endl;
      set_idle();
    }
  }

  // have arrived?
  if (!is_idle()){
    if (g -> target_position(com.target, stats.target_position)) {
      stats.converge = utility::l2d2(stats.target_position - position) < fleet::interact_d2;
    }
  }
}

void fleet::check_waypoint(game_data *g){
  // set to idle and 'land' ships if converged to waypoint
  if (stats.converge && identifier::get_type(com.target) == waypoint::class_id && com.action == fleet_action::go_to){
    com.action = fleet_action::idle;
    cout << "set fleet " << id << " idle target: " << com.target << endl;
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

// todo: more advanced fleet action policies
bool fleet::confirm_ship_interaction(string a, combid t){
  // always allow "space combat"
  if (a == interaction::space_combat) return true;

  bool action_match = a == com.action;
  bool target_match = t == com.target;

  return action_match && target_match;
}

void fleet::copy_from(const fleet &s){
  (*this) = s;
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

