#include "style.hpp"

#include <functional>
#include <string>

#include "rsg/src/button.hpp"
#include "rsg/src/utility.hpp"

using namespace std;
using namespace RSG;
using namespace st3;

ButtonPtr st3::make_label(string v) {
  return styled<Button>(
      {
          {"background-color", "00000000"},
          {"border-thickness", "0"},
      },
      v);
}

ButtonPtr st3::make_hbar() {
  return styled<Button>(
      {
          {"width", "100%"},
          {"height", "1px"},
          {"border-thickness", "0"},
          {"background-color", "11223388"},
          {"margin-top", "10"},
          {"margin-bottom", "10"},
          {"margin-left", "0"},
          {"margin-right", "0"},
      });
}

ButtonPtr st3::make_card(string v, int n, string h) {
  int w = (100 - 3 * (n + 1)) / n;
  return styled<Button>(
      {
          {"width", to_string(w) + "%"},
          {"height", h},
          {"font-size", "40"},
          {"background-color", "123456ff"},
          {"hover:background-color", "123456aa"},
          {"selected:color", "ffffffff"},
          {"selected:background-color", "654321aa"},
          {"selected:border-thickness", "2"},
          {"selected:border-color", "ffffff88"},
      },
      v);
}