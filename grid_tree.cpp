#include <iostream>
#include <cmath>

#include "utility.h"
#include "grid_tree.h"

using namespace std;
using namespace st3;
using namespace grid;

tree::tree(){
  max_leaves = 10;
  root = new node(this, 0);
}

tree::~tree(){
  delete root;
}

void tree::insert(key_type k, value_type v){
  root -> insert(k, v);
}

list<iterator_type> tree::search(point p, float r){
  return root -> search(p, r);
}

void tree::move(key_type k, value_type v){
  index[k] -> move(k, v);
}

void tree::remove(key_type k){
  index[k] -> remove(k);
  index.erase(k);
}

node::node(tree *i, node *p){
  g = i;
  parent = p;
  is_leaf = true;
  bounds = bound_type(point(-INFINITY, -INFINITY), point(INFINITY, INFINITY));
}

node::~node(){
  if (!is_leaf){
    for (int i = 0; i < 4; i++) if (children[i]) delete children[i];
  }
}

void node::make_split(){
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

void node::insert(key_type k, value_type v){
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

void node::move(key_type k, value_type v){
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

list<iterator_type> node::search(value_type p, float r){
  list<iterator_type> res;

  if (!(utility::point_above(p + point(r,r), bounds.first) && utility::point_below(p - point(r,r), bounds.second))){
    return res;
  }

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
    list<iterator_type> q;
    for (int i = 0; i < 4; i++){
      q = children[i] -> search(p, r);
      res.insert(res.end(), q.begin(), q.end());
    }
  }

  return res;
}

void node::remove(key_type k){
  if (!is_leaf){
    cout << "node: remove called on non-leaf" << endl;
    exit(-1);
  }

  leaves.erase(k);
}
