#include <cmath>
#include <iostream>

#include "grid_tree.h"
#include "utility.h"

using namespace std;
using namespace st3;
using namespace grid;

tree::tree() {
  max_leaves = 10;
  root = new node(this, 0);
}

tree::~tree() {
  delete root;
}

tree::ptr tree::create() {
  return ptr(new tree());
}

void tree::clear() {
  delete root;
  root = new node(this, 0);
  index.clear();
}

void tree::insert(key_type k, value_type v) {
  if (index.count(k)) {
    index[k]->move(k, v);
  } else {
    root->insert(k, v);
  }
}

list<iterator_type> tree::search(point p, float r) {
  return root->search(p, r);
}

void tree::move(key_type k, value_type v) {
  insert(k, v);
}

void tree::remove(key_type k) {
  if (!index.count(k)) return;
  index[k]->remove(k);
  index.erase(k);
}

node::node(tree *i, node *p) {
  g = i;
  parent = p;
  is_leaf = true;
  bounds = bound_type(point(-INFINITY, -INFINITY), point(INFINITY, INFINITY));
}

node::~node() {
  if (!is_leaf) {
    for (int i = 0; i < 4; i++)
      if (children[i]) delete children[i];
  }
}

void node::make_split() {
  int n = leaves.size();
  if (n <= g->max_leaves || !is_leaf) return;

  vector<int> xvalues(n);
  vector<int> yvalues(n);
  int i = 0;
  for (auto x : leaves) {
    xvalues[i] = x.second.x;
    yvalues[i] = x.second.y;
    i++;
  }

  sort(xvalues.begin(), xvalues.end());
  sort(yvalues.begin(), yvalues.end());

  // guarantee 3 distinct values
  auto test_diverse = [](vector<int> &data) {
    int idx = data.size() / 2;
    return data.front() != data[idx] && data.back() != data[idx];
  };

  if (!(test_diverse(xvalues) || test_diverse(yvalues))) {
    server::log("Non-diverse leaves in grid::node::make_split!", "warning");
    return;
  }

  int idx = n / 2;
  split = point(xvalues[idx], yvalues[idx]);

  // make child nodes
  for (int i = 0; i < 4; i++) children[i] = new node(g, this);
  children[0]->bounds = {bounds.first, split};
  children[1]->bounds = {point(split.x, bounds.first.y), point(bounds.second.x, split.y)};
  children[2]->bounds = {point(bounds.first.x, split.y), point(split.x, bounds.second.y)};
  children[3]->bounds = {split, bounds.second};

  // insert child values
  for (auto x : leaves) {
    point v = x.second;
    int idx = (v.x > split.x) + 2 * (v.y > split.y);
    children[idx]->insert(x.first, x.second);
  }

  leaves.clear();
  is_leaf = false;
}

void node::insert(key_type k, value_type v) {
  if (is_leaf) {
    leaves[k] = v;
    g->index[k] = this;

    if (leaves.size() > g->max_leaves) {
      make_split();
    }
  } else {
    int idx = (v.x > split.x) + 2 * (v.y > split.y);
    children[idx]->insert(k, v);
  }
}

void node::move(key_type k, value_type v) {
  if (!leaves.count(k)) throw logical_error("node: missing leaf: " + k);

  if (utility::point_between(v, bounds.first, bounds.second)) {
    leaves[k] = v;
  } else {
    leaves.erase(k);
    g->root->insert(k, v);
  }
}

list<iterator_type> node::search(value_type p, float r) {
  static int indent = 0;
  list<iterator_type> res;

  if (!(utility::point_above(p + point(r, r), bounds.first) && utility::point_below(p - point(r, r), bounds.second))) {
    return res;
  }

  indent++;

  if (is_leaf) {
    // collect leaves
    float r2 = r * r;
    for (auto x : leaves) {
      point delta = x.second - p;
      float d2 = delta.x * delta.x + delta.y * delta.y;
      if (d2 <= r2) res.push_back(x);
    }
  } else {
    list<iterator_type> q;
    for (int i = 0; i < 4; i++) {
      q = children[i]->search(p, r);
      res.insert(res.end(), q.begin(), q.end());
    }
  }

  indent--;
  return res;
}

void node::remove(key_type k) {
  if (!is_leaf) throw logical_error("node: remove called on non-leaf");
  leaves.erase(k);
}
