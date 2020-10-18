#pragma once

#include <array>
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
    std::set<K> at;
  };
  const static int depth = 4;
  static constexpr float grid_size = 1000;

  struct grid_data {
    bool blocked;
    std::array<V, depth> ids;
    int n;

    grid_data() {
      n = 0;
      blocked = false;
    }
  };

  // std::map<K, std::set<V>> index; /*!< Table over which elements are found at a position */
  std::vector<grid_data> index;
  // std::set<K> block_index;
  // std::map<V, std::set<K>> reverse_index;
  std::map<V, info> reverse_index;

  bool enable_block;

  tree();

  bool test_xy(int x, int y) const;
  const grid_data &get(int x, int y) const;
  grid_data &getref(int x, int y);
  bool add(int x, int y, V id);
  void set_block(int x, int y);

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
