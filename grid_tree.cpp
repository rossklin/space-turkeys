#include "grid_tree.h"

using namespace std;
using namespace st3;

grid::grid(){
  max_leaves = 10;
  root = new node(&index, 0);
}

grid::~grid(){
  delete root;
}

grid::node::node(grid::grid *i, grid::node *p){
  g = i;
  parent = p;
  is_leaf = true;
  bounds = bound_type(point(-Infinity, -Infinity), point(Infinity, Infinity));
}

grid::node::~node(){
  if (!is_leaf){
    for (int i = 0; i < 4; i++) if (children[i]) delete children[i];
  }
}

void grid::node::make_split(){
  // compute mid point
  split.x = split.y = 0;

  for (auto x : leaves) split = split + x.second;
  split = utility::scale_point(split, 1 / (float)leaves.size());

  // make child nodes
  for (int i = 0; i < 4; i++) children[i] = new node(g, this);
  children[0] -> bounds = {bounds.first, split};
  children[1] -> bounds = {point(split.x, bounds.first.y), point(bounds.second.x, split.y)};
  children[2] -> bounds = {point(bounds.first.x, split.y), point(split.x, bounds.second.y)};
  children[3] -> bounds = {split, bounds.second};

  // insert child values
  for (auto x : leaves){
    point v = x.second;
    int idx = (v.x > split.x) + 2 * (v.y > split.y);
    children[idx] -> insert(x.first, x.second);
  }

  leaves.clear();
  is_leaf = false;
}

void grid::node::insert(grid::key_type k, grid::value_type v){
  if (is_leaf){
    leaves[k] = v;
    g -> index[k] = this;

    if (leaves.size() > g -> max_leaves){
      make_split();
    }
  }else{
    int idx = (v.x > split.x) + 2 * (v.y > split.y);
    children[idx] -> insert(k, v);
  }
}

void grid::node::move(grid::key_type k, grid::value_type v){
  if (!leaves.count(k)){
    cout << "node: missing leaf: " << k << endl;
    exit(-1);
  }

  if (utility::point_between(v, bounds.first, bounds.second)){
    leaves[k] = v;
  }else{
    leaves.erase(k);
    g -> root -> insert(k,v);
  }
}

list<grid::iterator_type> grid::node::search(grid::value_type p, float r){
  list<grid::iterator_type> res;

  if (is_leaf){
    // collect leaves
    float r2 = r * r;
    for (auto x : leaves){
      point delta = x.second - p;
      float d2 = delta.x * delta.x + delta.y * delta.y;
      if (d2 <= r2){
	res.push_back(x);
      }
    }
  }else{
    list<grid::iterator_type> q;
    for (int i = 0; i < 4; i++){
      q = children[i] -> search(p, r);
      res.insert(res.end(), q.begin(), q.end());
    }
  }

  return res;
}
