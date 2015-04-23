#ifndef _STK_RESEARCH
#define _STK_RESEARCH

#include <vector>

namespace st3{
  struct research{
    enum fields{
      r_population = 0,
      r_industry,
      r_ship,
      r_num
    };
    
    std::vector<float> field;

    research();
  };
};
#endif
