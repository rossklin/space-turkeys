#include "style.hpp"

#include <functional>
#include <string>

#include "rsg/src/button.hpp"
#include "rsg/src/utility.hpp"

using namespace std;
using namespace RSG;
using namespace st3;

void st3::generate_styles() {
  Component::class_style["Panel"]["border-thickness"] = "1";

  Component::tag_style["main-panel"] = {
      {"width", "90%"},
      {"height", "90%"},
      {"background-color", "12345688"},
      {"color", "ffccaaff"},
  };

  Component::tag_style["transparent"] = {
      {"background-color", "00000000"},
      {"border-thickness", "0"},
  };

  Component::tag_style["label"] = {
      {"background-color", "00000000"},
      {"border-thickness", "0"},
  };

  Component::tag_style["h1"] = {
      {"background-color", "00000000"},
      {"border-thickness", "0"},
      {"font-size", "25"},
  };

  Component::tag_style["hbar"] = {
      {"width", "100%"},
      {"height", "2px"},
      {"border-thickness", "0"},
      {"background-color", "aabbcc88"},
      {"margin-top", "10"},
      {"margin-bottom", "10"},
      {"margin-left", "0"},
      {"margin-right", "0"},
  };

  Component::tag_style["card"] = {
      {"font-size", "25"},
      {"background-color", "123456ff"},
      {"hover:background-color", "123456aa"},
      {"selected:color", "ffffffff"},
      {"selected:background-color", "654321aa"},
      {"selected:border-thickness", "2"},
      {"selected:border-color", "ffffff88"},
  };
}

ButtonPtr st3::make_label(string v) {
  return tag({"label"}, Button::create(v));
}

ButtonPtr st3::make_hbar() {
  return tag({"hbar"}, Button::create(""));
}

ButtonPtr st3::make_card(string v, int n, string h) {
  ButtonPtr b = tag({"card"}, Button::create(v));
  int w = (100 - 3 * (n + 1)) / n;
  return with_style(
      {
          {"width", to_string(w) + "%"},
          {"height", h},
      },
      b);
}