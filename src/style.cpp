#include "style.hpp"

#include <functional>
#include <string>

#include "rsg/src/button.hpp"
#include "rsg/src/utility.hpp"

using namespace std;
using namespace RSG;

ButtonPtr make_label(string v) {
  return styled<Button>(
      {
          {"background-color", "00000000"},
          {"border-thickness", "0"},
      },
      v);
}

ButtonPtr make_hbar() {
  return styled<Button>(
      {
          {"width", "100%"},
          {"height", "1px"},
          {"border-thickness", "0"},
          {"background-color", "11223388"},
          {"margin-top", "20"},
          {"margin-bottom", "20"},
          {"margin-left", "0"},
          {"margin-right", "0"},
      });
}
