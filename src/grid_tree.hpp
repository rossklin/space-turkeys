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
  static constexpr float grid_size = 10;

  std::map<K, std::set<V>> index; /*!< Table over which elements are found at a position */
  std::set<K> block_index;
  std::map<V, std::set<K>> reverse_index;
  std::map<V, info> metadata;

  bool enable_block;

  tree();

  void start_blocking();

  void stop_blocking();

  void clear();

  static K make_key(point p);

  static point map_key(K k);

  /*! insert an id-position pair
	@param id id
	@param p position
      */
  void insert(V id, point p, float rad);

  /*! update the position for given id
	@param id id
	@param p new position
      */
  void move(V id, point p);

  std::set<V> block_search(point p, float rad);

  /*! find elements in a radius of a point
	@param p point to search around
	@param r search radius
	@return list of (id, position) pairs x s.t. l2norm(x.second - p) < r
      */
  std::list<std::pair<V, point>> search(point p, float rad, int knn = 0) const;

  /*! remove an element
	@param id id of element to remove
      */
  void remove(V id);
};  // namespace grid
};  // namespace grid
};  // namespace st3
