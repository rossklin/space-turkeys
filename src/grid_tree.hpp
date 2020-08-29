#pragma once

#include <list>
#include <map>
#include <set>

#include "types.hpp"
#include "utility.hpp"

namespace st3 {
/*! functions for the 2d search tree */
namespace grid {

/*! a tree handles grid nodes */
template <typename V>
struct tree {
  typedef std::pair<int, int> K;
  struct info {
    float r;
    point p;
  };
  static constexpr float r = 10;

  std::map<K, std::set<V>> index; /*!< Table over which elements are found at a position */
  std::map<V, std::set<K>> reverse_index;
  std::map<V, info> metadata;

  void clear() {
    index.clear();
    reverse_index.clear();
    metadata.clear();
  }

  K make_key(point p) const {
    return K(p.x / r, p.y / r);
  }

  /*! insert an id-position pair
	@param id id
	@param p position
      */
  void insert(V id, point p, float rad) {
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
  void
  move(V id, point p) {
    if (metadata.count(id)) {
      float rad = metadata.at(id).r;
      remove(id);
      insert(id, p, rad);
    }
  }

  /*! find elements in a radius of a point
	@param p point to search around
	@param r search radius
	@return list of (id, position) pairs x s.t. l2norm(x.second - p) < r
      */
  std::list<std::pair<V, point>> search(point p, float rad) const {
    std::set<V> ids;
    for (float x = p.x - rad; x <= p.x + rad; x += r) {
      for (float y = p.y - rad; y <= p.y + rad; y += r) {
        K k = make_key(point(x, y));
        if (index.count(k)) {
          for (auto id : index.at(k)) {
            info buf = metadata.at(id);
            if (utility::l2norm(p - buf.p) < rad + buf.r) {
              ids.insert(id);
            }
          }
        }
      }
    }

    std::list<std::pair<V, point>> res;
    for (auto id : ids) res.push_back({id, metadata.at(id).p});

    return res;
  }

  /*! remove an element
	@param id id of element to remove
      */
  void remove(V id) {
    if (metadata.count(id)) {
      for (auto k : reverse_index.at(id)) {
        index.at(k).erase(id);
        if (index.at(k).empty()) index.erase(k);
      }
      metadata.erase(id);
      reverse_index.erase(id);
    }
  }
};  // namespace grid
};  // namespace grid
};  // namespace st3
