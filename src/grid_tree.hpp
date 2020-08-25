#pragma once

#include <list>
#include <map>
#include <set>

#include "types.hpp"

namespace st3 {
/*! functions for the 2d search tree */
namespace grid {

/*! a tree handles grid nodes */
template <typename V>
struct tree {
  // static ptr create();
  typedef std::pair<int, int> K;

  static constexpr float r = 10;

  std::map<K, std::set<V>> index; /*!< table over which node elements are listed in */
  std::map<V, K> reverse_index;

  // int max_leaves;                                    /*!< maximum number of leaves per node before split */
  // node *root;                                        /*!< root node */

  // /*! default constructor */
  // tree();

  void clear() {
    index.clear();
    reverse_index.clear();
  }

  K make_key(point p) const {
    return K(p.x / r, p.y / r);
  }

  /*! insert an id-position pair
	@param id id
	@param p position
      */
  void insert(V id, point p) {
    K k = make_key(p);
    index[k].insert(id);
    reverse_index[id] = k;
  }

  /*! update the position for given id
	@param id id
	@param p new position
      */
  void move(V id, point p) {
    remove(id);
    insert(id, p);
  }

  /*! find elements in a radius of a point
	@param p point to search around
	@param r search radius
	@return list of (id, position) pairs x s.t. l2norm(x.second - p) < r
      */
  std::list<std::pair<V, point>> search(point p, float rad) const {
    std::list<std::pair<V, point>> res;
    for (float x = p.x - rad; x <= p.x + rad; x += r) {
      for (float y = p.y - rad; y <= p.y + rad; y += r) {
        K k = make_key(point(x, y));
        if (index.count(k)) {
          for (auto id : index.at(k)) res.push_back({id, {x, y}});
        }
      }
    }

    return res;
  }

  /*! remove an element
	@param id id of element to remove
      */
  void remove(V id) {
    if (reverse_index.count(id)) {
      K k = reverse_index[id];
      reverse_index.erase(id);
      index[k].erase(id);
    }
  }
};
};  // namespace grid
};  // namespace st3
