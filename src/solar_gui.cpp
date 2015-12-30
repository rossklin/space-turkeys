#include <list>
#include <string>

#include "types.h"
#include "solar.h"
#include "solar_gui.h"
#include "graphics.h"
#include "utility.h"

using namespace std;
using namespace sfg;

using namespace st3;
using namespace solar;
using namespace gui;

// build and wrap in shared ptr

s_research::Create(choice::c_research c, research r){return Ptr(new s_research(c, r));}
s_military::Create(choice::c_military c, research r){return Ptr(new s_military(c, r));}
s_mining::Create(choice::c_mining c, research r){return Ptr(new s_mining(c, r));}
main_window::Create(choice::choice_t c, research r, solar s){return Ptr(new main_window(c, r, s));}

// constructors

s_research::s_research(choice::c_research c, research r) : response(c){
  
};

s_military::s_military(choice::c_military c, research r) : response(c){

};

s_mining::s_mining(choice::c_mining c, research r) : response(c){

};

main_window::main_window(choice::choice_t c, research r, solar s) : response(c){
  auto layout = Box::Create(Box::Orientation::VERTICAL);
}



