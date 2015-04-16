#ifndef _STK_GRIDTREE
#define _STK_GRIDTREE

#include <SFML/Graphics.hpp>
#include "types.h"

namespace st3{
  struct grid{
    typedef idtype key_type;
    typedef point value_type;
    typedef std::pair<point, point> bound_type;
    typedef hm_t<key_type, node*> index_type;

    struct node{
      grid *g;
      node *parent;
      bool is_leaf;
      bound_type bounds;

      // branch node
      point split;
      node *children[4]; // UL, UR, BL, BR

      // leaf node
      hm_t<key_type, value_type> leaves;

      node(grid *g, node *p);
      node(const node &n) = delete;
      ~node();
      void make_split();
      bound_type insert(key_type k, value_type v); // return leaf node
      std::list<value_type> search(point p, float r);
    };

    index_type index;
    int max_leaves;
    node *root;

    grid();
    ~grid();
    grid(const grid &g) = delete;
    void insert(key_type id, value_type p);
    void move(key_type id, value_type p);
    std::list<value_type> search(value_type p, float r);
  };
};

#endif
