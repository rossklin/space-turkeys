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

s_military::Create(choice::c_military c, research r){return Ptr(new s_military(c, r));}
s_mining::Create(choice::c_mining c, research r){return Ptr(new s_mining(c, r));}
main_window::Create(choice::choice_t c, research r, solar s){return Ptr(new main_window(c, r, s));}

// constructors

// sub window for military choice
s_military::s_military(choice::c_military c, research r) : response(c){

};

// sub window for mining choice
s_mining::s_mining(choice::c_mining c, research r) : response(c){

};

// main window for solar choice
main_window::main_window(choice::choice_t c, research r, solar s) : response(c){
  int max_allocation;
  auto layout = Box::Create(Box::Orientation::VERTICAL);
  auto l_help = Label::Create("Click left/right to add/reduce");

  // culture allocation and sub menu
  auto b_culture = Button::Create("CULTURE");
  auto e_culture = Button::Create(">");

  b_culture -> GetSignal(Widget::OnLeftClick).Connect([&response, max_allocation](){
      if (response.count_allocation() < max_allocation) response.allocation.culture++;
    };);

  b_culture -> GetSignal(Widget::OnRightClick).Connect([&response](){
      if (response.allocation.culture > 0) response.allocation.culture--;
    };);

  e_culture -> GetSignal(Widget::OnLeftClick).Connect([
  
  auto b_research = Button::Create("RESEARCH");
  auto b_military = Button::Create("MILITARY");
  auto b_mining = Button::Create("MINING");
  auto b_accept = Button::Create("ACCEPT");
  auto b_cancel = Button::Create("CANCEL");
}



