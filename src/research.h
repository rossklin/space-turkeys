#ifndef _STK_RESEARCH
#define _STK_RESEARCH

#include <vector>

namespace st3{
  /*! struct representing the research level of a player */
  struct research{
    /*! the available fields of research */
    enum fields{
      r_population = 0,
      r_industry,
      r_ship,
      r_num
    };
    
    /*! levels of the fields of research */
    std::vector<float> field;

    /*! default constructor */
    research();
  };
};
#endif
