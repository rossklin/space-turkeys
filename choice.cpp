#include "choice.h"

using namespace std;
using namespace st3;

st3::alter_command::alter_command(){
  delete_c = false;
  lock = false;
  unlock = false;
  increment = 0;
}

void delete_selected(hm_t<idtype, command> &m){
  hm_t<idtype, command> buf;

  for (auto i = m.begin(); i != m.end(); i++){
    if (!i -> second.selected){
      buf[i -> first] = i -> second;
    }
  }

  m.swap(buf);
  return;
}

void alter(alter_command conf, hm_t<idtype, command> &m){

  if (conf.delete_c){
    delete_selected(m);
    return;
  }

  for (auto i = m.begin(); i != m.end(); i++){
    if (i -> second.selected){
      i -> second.locked |= conf.lock;
      i -> second.locked &= !conf.unlock;
      i -> second.quantity += conf.increment;
    }
  }
}

void clear(hm_t<idtype, command> &m){
  for (auto i = m.begin(); i != m.end(); i++){
    i -> second.selected = false;
  }
}

void st3::choice::alter_selected(alter_command conf){
  alter(conf, fleet_commands);
  alter(conf, solar_commands);
}

void st3::choice::clear_selection(){
  clear(fleet_commands);
  clear(solar_commands);
}
