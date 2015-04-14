#ifndef _STK_GRIDTREE
#define _STK_GRIDTREE

#include "types.h"

namespace st3{
  namespace grid_tree{
    typedef std::pair<idtype, point> value_type;
    struct node{
      node *parent;
      node *lower_child;
      node *upper_child;
      bool align; // false = x, true = y
      float split; 
      int count; // number of children
      value_type value; // node has either children or value

      node(value_type v);
      ~node();
      void insert_value(value_type v) = 0;
      std::list<value_type> search(point p, float r) = 0;
    };
  };
};

#endif
