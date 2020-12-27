#include "grid_tree.hpp"

#include <list>
#include <map>
#include <set>

#include "types.hpp"
#include "utility.hpp"

using namespace std;
using namespace st3;
using namespace grid;

template class grid::tree<idtype>;

template <typename V>
grid::tree<V>::tree() {
  enable_block = false;
  index.resize(grid_size * grid_size);
}

template <typename V>
bool grid::tree<V>::test_xy(int x, int y) const {
  return x >= 0 && x < grid_size && y >= 0 && y < grid_size;
}

template <typename V>
typename grid::tree<V>::grid_data &grid::tree<V>::getref(int x, int y) {
  if (test_xy(x, y)) {
    return index[y * grid_size + x];
  } else {
    throw logical_error("Called get on invalid index");
  }
}

template <typename V>
const typename grid::tree<V>::grid_data &grid::tree<V>::get(int x, int y) const {
  if (test_xy(x, y)) {
    return index[y * grid_size + x];
  } else {
    throw logical_error("Called get on invalid index");
  }
}

template <typename V>
bool grid::tree<V>::add(int x, int y, V id) {
  if (!test_xy(x, y)) return false;
  grid_data &buf = getref(x, y);
  if (buf.n >= depth) return false;
  buf.ids[buf.n++] = id;
  return true;
}

template <typename V>
void grid::tree<V>::set_block(int x, int y) {
  if (test_xy(x, y)) getref(x, y).blocked = true;
}

template <typename V>
void grid::tree<V>::start_blocking() {
  // block_index.clear();
  for (auto &x : index) x.blocked = false;
  enable_block = true;
}

template <typename V>
void grid::tree<V>::stop_blocking() {
  for (auto &x : index) x.blocked = false;
  // block_index.clear();
  enable_block = false;
}

template <typename V>
void grid::tree<V>::clear() {
  index.clear();
  index.resize(grid_size * grid_size);

  reverse_index.clear();
  // block_index.clear();
  // metadata.clear();
  enable_block = false;
}

template <typename V>
typename grid::tree<V>::K grid::tree<V>::make_key(point p) {
  return K(p.x / grid_size, p.y / grid_size);
}

template <typename V>
point grid::tree<V>::map_key(K k) {
  return point(grid_size * (k.first + 0.5), grid_size * (k.second + 0.5));
}

/*! insert an id-position pair
	@param id id
	@param p position
      */
template <typename V>
void grid::tree<V>::insert(V id, point p, float rad) {
  // metadata.emplace(id, info{rad, p});
  reverse_index[id] = {rad, p, {}};
  for (float x = p.x - rad; x <= p.x + rad; x++) {
    float dx = x - p.x;
    float dy = sqrt(rad * rad - dx * dx);
    for (float y = p.y - dy; y <= p.y + dy; y++) {
      if (test_xy(x, y) && add(x, y, id)) reverse_index[id].at.insert({x, y});
    }
  }
}

/*! update the position for given id
	@param id id
	@param p new position
      */
template <typename V>
void grid::tree<V>::move(V id, point p) {
  if (reverse_index.count(id)) {
    float rad = reverse_index.at(id).r;
    remove(id);
    insert(id, p, rad);
  }
}

template <typename V>
set<V> grid::tree<V>::block_search(point p, float rad) {
  set<V> ids;

  // // Check for floating point errors
  // if (!(p.x + grid_size > p.x && p.y + grid_size > p.y)) {
  //   server::log("Rounding error in grid::tree::block_search!", "warning");
  //   return {};
  // }

  for (float x = p.x - rad; x <= p.x + rad; x++) {
    float dx = x - p.x;
    float dy = sqrt(rad * rad - dx * dx);
    for (float y = p.y - dy; y <= p.y + dy; y++) {
      if (test_xy(x, y) && !get(x, y).blocked) {
        grid_data &buf = getref(x, y);
        ids.insert(buf.ids.begin(), buf.ids.begin() + buf.n);
        buf.blocked = true;
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
  map<V, point> ids;

  auto process = [this, rad, &ids](K k) {
    int x = k.first;
    int y = k.second;
    if (x * x + y * y < rad * rad && test_xy(x, y)) {
      const grid_data &buf = get(x, y);
      for (int i = 0; i < buf.n; i++) {
        ids[buf.ids[i]] = {x, y};  // OPTIMIZE 18
      }
    }
  };

  int max_delta = round(rad);
  for (int delta = 0; delta <= max_delta; delta++) {
    for (int idx = -delta; idx <= delta; idx++) {
      process({p.x + idx, p.y - delta});
      process({p.x + idx, p.y + delta});
      process({p.x - delta, p.y + idx});
      process({p.x + delta, p.y + idx});
      if (knn > 0 && ids.size() >= knn) break;
    }
    if (knn > 0 && ids.size() >= knn) break;
  }

  return list<pair<V, point>>(ids.begin(), ids.end());
}

/*! remove an element
	@param id id of element to remove
      */
template <typename V>
void grid::tree<V>::remove(V id) {
  if (reverse_index.count(id)) {
    for (auto k : reverse_index.at(id).at) {
      grid_data &buf = getref(k.first, k.second);
      for (int i = 0; i < buf.n; i++) {
        if (buf.ids[i] == id) {
          buf.ids[i] = buf.ids[buf.n - 1];
          buf.n--;
          break;
        }
      }
    }
    reverse_index.erase(id);
  }
}
