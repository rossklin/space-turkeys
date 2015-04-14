#include <iostream>
#include <cmath>

#include "types.h"
#include "game_data.h"
#include "utility.h"

using namespace std;
using namespace st3;

idtype st3::ship::id_counter = 0;
idtype st3::solar::id_counter = 0;
idtype st3::fleet::id_counter = 0;

st3::game_settings::game_settings(){
  frames_per_round = 100;
  width = 400;
  height = 400;
  ship_speed = 3;
  solar_minrad = 5;
  solar_maxrad = 20;
  num_solars = 20;
  fleet_default_radius = 10;
}

point st3::game_data::target_position(target_t t){
  string type = identifier::get_type(t);
  if (!type.compare(identifier::solar)){
    return solars[identifier::get_id(t)].position;
  }else if (!type.compare(identifier::fleet)){
    return fleets[identifier::get_id(t)].position;
  }else if (!type.compare(identifier::point)){
    return identifier::get_point(t);
  }else{
    cout << "target_position: " << t << ": unknown type!" << endl;
    exit(-1);
  }
}

void st3::game_data::generate_fleet(point p, idtype owner, command c){
  fleet f;
  f.com = c;
  f.position = p;
  f.radius = settings.fleet_default_radius;
  f.owner = owner;

  for (int i = 0; i < c.quantity; i++){
    ship s;
    idtype sid = ship::id_counter++;
    s.position = p + utility::random_point_polar(p, f.radius);
    s.angle = utility::point_angle(target_position(c.target) - p);
    s.speed = settings.ship_speed;
    s.owner = owner;
    s.hp = 1;
    s.was_killed = false;
    ships[sid] = s;
    f.ships.push_back(sid);
  }

  fleets[fleet::id_counter++] = f;
  
}

void st3::game_data::set_fleet_commands(idtype id, list<command> commands){
  fleet &s = fleets[id];

  // check quantity sum
  int sum = 0;
  for (auto &x : commands) sum += x.quantity;
  if (sum > s.ships.size()){
    cout << "fleet " << id << " command quantity sum exceeds limit!" << endl;
    exit(-1);
  }

  // split fleets
  for (auto &x : commands){
    fleet f;
    f.com = x;
    f.position = s.position;
    f.radius = s.radius;
    f.owner = s.owner;
    for (int i = 0; i < x.quantity; i++){
      int sid = s.ships.front();
      s.ships.pop_front();
      f.ships.push_back(sid);
    }

    fleets[fleet::id_counter++] = f;
  }

  // check remove
  if (s.ships.empty()){
    fleets.erase(id);
  }
}

void st3::game_data::set_solar_commands(idtype id, list<command> commands){
  solar &s = solars[id];

  // check quantity sum
  int sum = 0;
  for (auto x : commands) sum += x.quantity;
  if (sum > (int)s.quantity){
    cout << "solar " << id << " command quantity sum exceeds limit!" << endl;
    exit(-1);
  }

  // create fleets
  for (auto x : commands){
    s.quantity -= x.quantity;
    generate_fleet(s.position, s.owner, x);
  }
}
 
void st3::game_data::apply_choice(choice c, idtype id){
  cout << "game_data: running dummy apply choice" << endl;

  for (auto x : c.commands){
    if (!identifier::get_type(x.first).compare(identifier::solar)){
      // commands for solar
      idtype sid = identifier::get_id(x.first);

      // check ownership
      if (solars[sid].owner != id){
	cout << "apply_choice: invalid owner: " << solars[sid].owner << ", should be " << id << endl;
	exit(-1);
      }

      // apply the commands
      set_solar_commands(sid, x.second);
    }else if (!identifier::get_type(x.first).compare(identifier::fleet)){
      // command for fleet
      idtype fid = identifier::get_id(x.first);
      
      // check ownerships
      if (fleets[fid].owner != id){
	cout << "apply_choice: invalid owner: " << fleets[fid].owner << ", should be " << id << endl;
	exit(-1);
      }

      // apply the commands
      set_fleet_commands(fid, x.second);
    }else{
      cout << "apply_choice: invalid source: " << x.first << endl;
      exit(-1);
    }
  }
}

void st3::game_data::increment(){
  cout << "game_data: running dummy increment" << endl;
  for (auto i = ships.begin(); i != ships.end(); i++){
    i -> second.position.x += i -> second.speed * cos(i -> second.angle);
    i -> second.position.y += i -> second.speed * sin(i -> second.angle);
    i -> second.angle += (rand() % 100) / (sfloat)1000;
    i -> second.was_killed |= !(rand() % 100);
  }
}

// produce a number of random solars with no owner according to
// settings
hm_t<idtype, solar> st3::game_data::random_solars(){
  hm_t<idtype, solar> buf;
  int q_start = 20;

  for (int i = 0; i < settings.num_solars; i++){
    solar s;
    s.radius = settings.solar_minrad + rand() % (int)(settings.solar_maxrad - settings.solar_minrad);
    s.position.x = s.radius + rand() % (int)(settings.width - 2 * s.radius);
    s.position.y = s.radius + rand() % (int)(settings.height - 2 * s.radius);
    s.owner = -1;
    s.quantity = rand() % q_start;
    buf[i] = s;
  }

  // shake the solars around till they don't overlap
  bool overlap = true;
  int margin = 10;

  cout << "game_data: shaking solars..." << endl;
  for (int n = 0; overlap && n < 100; n++){
    overlap = false;
    for (int i = 0; i < settings.num_solars; i++){
      for (int j = i + 1; j < settings.num_solars; j++){
	point delta = buf[i].position - buf[j].position;
	if (utility::l2norm(delta) < buf[i].radius + buf[j].radius + margin){
	  overlap = true;
	  buf[i].position = buf[i].position + utility::scale_point(delta, 0.1) + point(rand() % 4 - 2, rand() % 4 - 2);
	}
      }
    }
  }

  return buf;
}

// find the fairest selection of (player.id, start_solar.id) and store
// in start_solars, return unfairness estimate
float st3::game_data::heuristic_homes(hm_t<idtype, solar> solar_buf, hm_t<idtype, idtype> &start_solars){
  int ntest = 100;
  float umin = INFINITY;
  hm_t<idtype, idtype> test_solars;
  vector<idtype> solar_ids, player_ids;
  vector<bool> reach;
  int count = 0;
  
  if (players.size() > solar_buf.size()){
    cout << "heuristic homes: to many players: " << players.size() << " > " << solar_buf.size() << endl;
    exit(-1);
  }

  float range = settings.ship_speed * settings.frames_per_round;
  
  for (auto x : solar_buf) solar_ids.push_back(x.first);
  for (auto x : players) player_ids.push_back(x.first);
  reach.resize(player_ids.size());

  for (int i = 0; i < ntest; i++){
    // random test setup
    for (auto j : player_ids){
      test_solars[j] = solar_ids[rand() % solar_ids.size()];
    }

    // compute number of owned solars in reach of at least one other
    // owned solar
    
    for (int j = 0; j < player_ids.size(); j++){
      reach[j] = false;
    }

    count = 0;
    for (int j = 0; j < player_ids.size(); j++){
      for (int k = j + 1; k < player_ids.size(); k++){
	solar a = solar_buf[test_solars[player_ids[j]]];
	solar b = solar_buf[test_solars[player_ids[k]]];
	bool r = pow(a.position.x - b.position.x, 2) + pow(a.position.y - b.position.y, 2) < range;
	reach[j] = reach[j] || r;
	reach[k] = reach[k] || r;
      }
      count += reach[j];
    }

    if (count < umin){
      cout << "heuristic homes: new best reach count: " << count << endl;
      umin = count;
      start_solars = test_solars;
    }
  }

  return umin;
}

// players and settings should be set before build is called
void st3::game_data::build(){
  if (players.empty()){
    cout << "game_data: build: no players!" << endl;
    exit(-1);
  }

  cout << "game_data: running dummy build" << endl;

  // build solars
  int ntest = 100;
  float unfairness = INFINITY;
  hm_t<idtype, idtype> test_homes;
  int q_start = 20;

  for (int i = 0; i < ntest && unfairness > 0; i++){
    hm_t<idtype, solar> solar_buf = random_solars();
    float u = heuristic_homes(solar_buf, test_homes);
    if (u < unfairness){
      cout << "game_data::initialize: new best homes, u = " << u << endl;
      unfairness = u;
      solars = solar_buf;
      for (auto x : test_homes){
	solars[x.second].owner = x.first;
	solars[x.second].quantity = q_start;
      }
    }
  }

  cout << "resulting solars:" << endl;
  for (auto x : solars){
    solar s = x.second;
    cout << "solar " << x.first << ": p = " << s.position.x << "," << s.position.y << ": r = " << s.radius << ", quantity = " << s.quantity << endl;
  }
}

// find all dead ships and remove them
void st3::game_data::cleanup(){
  hm_t<idtype, ship> buf;
  cout << "cleanup: start: " << ships.size() << " ships." << endl;
  for (auto x : ships){
    if (!x.second.was_killed) buf[x.first] = x.second;
  }
  ships.swap(buf);
  cout << "cleanup: end: " << ships.size() << " ships." << endl;
}
