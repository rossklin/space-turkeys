#include "grid_tree.hpp"

#include <list>
#include <map>
#include <set>

#include "types.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace grid;

template class grid::tree<combid>;

template <typename V>
grid::tree<V>::tree() {
  enable_block = false;
}

template <typename V>
void grid::tree<V>::start_blocking() {
  block_index.clear();
  enable_block = true;
}

template <typename V>
void grid::tree<V>::stop_blocking() {
  block_index.clear();
  enable_block = false;
}

template <typename V>
void grid::tree<V>::clear() {
  index.clear();
  reverse_index.clear();
  block_index.clear();
  metadata.clear();
  enable_block = false;
}

template <typename V>
typename grid::tree<V>::K grid::tree<V>::make_key(point p) {
  return K(p.x / r, p.y / r);
}

template <typename V>
point grid::tree<V>::map_key(K k) {
  return point(r * (k.first + 0.5), r * (k.second + 0.5));
}

/*! insert an id-position pair
	@param id id
	@param p position
      */
template <typename V>
void grid::tree<V>::insert(V id, point p, float rad) {
  metadata.emplace(id, info{r, p});
  reverse_index[id] = {};
  for (float x = p.x - rad; x <= p.x + rad; x += r) {
    for (float y = p.y - rad; y <= p.y + rad; y += r) {
      K k = make_key({x, y});
      index[k].insert(id);
      reverse_index[id].insert(k);
    }
  }
}

/*! update the position for given id
	@param id id
	@param p new position
      */
template <typename V>
void grid::tree<V>::move(V id, point p) {
  if (metadata.count(id)) {
    float rad = metadata.at(id).r;
    remove(id);
    insert(id, p, rad);
  }
}

template <typename V>
set<V> grid::tree<V>::block_search(point p, float rad) {
  set<V> ids;

  for (float x = p.x - rad; x <= p.x + rad; x += r) {
    for (float y = p.y - rad; y <= p.y + rad; y += r) {
      K k = make_key(point(x, y));
      if (index.count(k) && !block_index.count(k)) {
        ids += index.at(k);
        block_index.insert(k);
      }
    }
  }

  return ids;
}

/*! find elements in a radius of a point
	@param p point to search around
	@param r search radius
	@return list of (id, position) pairs x s.t. l2norm(x.second - p) < r
      */
template <typename V>
list<pair<V, point>> grid::tree<V>::search(point p, float rad, int knn) const {
  set<V> ids;
  K last_k(INT_FAST32_MAX, INT_FAST32_MAX);

  for (float rbuf = 0; rbuf <= rad; rbuf += r) {
    float a_inc = 2 * M_PI;
    if (rbuf > 0) a_inc = 0.1 * r / rbuf;
    for (float a = 0; a < 2 * M_PI; a += a_inc) {
      K k = make_key(p + rbuf * utility::normv(a));
      if (k != last_k) {
        last_k = k;
        if (index.count(k)) {
          for (auto id : index.at(k)) {
            info buf = metadata.at(id);
            if (utility::l2norm(p - buf.p) < rad + buf.r) {
              ids.insert(id);
            }
          }
        }
      }
      if (knn > 0 && ids.size() >= knn) break;
    }
    if (knn > 0 && ids.size() >= knn) break;
  }

  list<pair<V, point>> res;
  for (auto id : ids) res.push_back({id, metadata.at(id).p});

  return res;
}

/*! remove an element
	@param id id of element to remove
      */
template <typename V>
void grid::tree<V>::remove(V id) {
  if (metadata.count(id)) {
    for (auto k : reverse_index.at(id)) {
      index.at(k).erase(id);
      if (index.at(k).empty()) index.erase(k);
    }
    metadata.erase(id);
    reverse_index.erase(id);
  }
}
