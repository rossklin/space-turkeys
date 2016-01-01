#include <vector>

#include <SFGUI/SFGUI.hpp>
#include <SFGUI/Widgets.hpp>
#include <SFML/Graphics.hpp>

#include "research.h"

using namespace std;
using namespace st3;
using namespace research;
using namespace sfg;

data::data(){
  field.resize(r_num, 1);
}

// gui
// wrap in shared pointer
gui::Create(choice::c_research c, research r){return Ptr(new gui(c, r));}

// constructor
gui::gui(choice::c_research c, data r) : response(c){
  
};
