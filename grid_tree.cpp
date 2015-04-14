#include "grid_tree.h"

using namespace std;
using namespace st3;

grid_tree::node::node(grid_tree::value_type v){
  parent = 0;
  lower_child = 0;
  upper_child = 0;
  count = 0;
  value = v;
}

grid_tree::node::~node(){
  if (lower_child) delete lower_child;
  if (upper_child) delete upper_child;
}

void grid_tree::node::insert_value(grid_tree::value_type v){
  if (count){
    if (!(upper_child && lower_child)){
      cout << "missing child" << endl;
      exit(-1);
    }
    
    float x = align ? v.second.y : v.second.x;
    if (x < split){
      lower_child -> insert_value(v);
    }else{
      upper_child -> insert_value(v);
    }
  }else{
    // todo: find best split
    // todo: make children
    // todo: place v in one child, value in the other
  }

  count++;
}
