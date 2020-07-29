#include "style.hpp"

#include <functional>
#include <string>

#include "rsg/src/button.hpp"
#include "rsg/src/panel.hpp"
#include "rsg/src/utility.hpp"

using namespace std;
using namespace RSG;
using namespace st3;

void st3::generate_styles() {
  // Class style modifications

  // Default dark theme
  string default_bg = "123456bb";
  string default_fg = "ffccaaff";
  string default_border = "ffffffbb";

  // Default light theme
  string default_bg2 = "99aabbaa";
  string default_fg2 = "553311ff";
  string default_border2 = "33557755";

  string transparent = "00000000";
  string bar_color = "aabbcc88";

  StyleMap s_transparent = {
      {"background-color", transparent},
      {"border-thickness", "0"},
  };

  StyleMap s_default1 = {
      {"color", default_fg},
      {"background-color", default_bg},
      {"border-color", default_border},
  };

  StyleMap s_default2 = {
      {"color", default_fg2},
      {"background-color", default_bg2},
      {"border-color", default_border2},
  };

  // Prevent inheritance for panel background since stacked semitransparent panels look weird
  Component::class_style["Panel"]["background-color"] = transparent;
  Component::class_style["Panel"]["border-thickness"] = "0";

  Component::class_style["Button:hover"]["background-color"] = "345678ff";

  Component::tag_style = {
      // Main panel
      {
          ".main-panel",
          StyleMap{
              {"width", "90%"},
              {"height", "90%"},
          } + s_default1,
      },

      // Right panel
      {
          ".right-panel",
          StyleMap{
              {"position", "absolute"},
              {"right", "0"},
              {"left", "auto"},
              {"top", "0"},
              {"width", "25%"},
              {"height", "100%"},
              {"border-thickness", "5"},
              {"align-vertical", "top"},
          } + s_default2,
      },

      {
          ".right-panel Button",
          {
              {"width", "100%"},
              {"height", "30px"},
              {"border-thickness", "0"},
          },
      },

      {
          ".right-panel > Panel",
          {
              {"width", "100%"},
              {"height", "200px"},
              {"align-horizontal", "left"},
              {"align-vertical", "top"},
              {"padding-top", "5"},
          },
      },

      {
          ".event-log .list-item",
          {
              {"margin-left", "0"},
              {"margin-right", "0"},
              {"margin-top", "5"},
              {"margin-bottom", "5"},
              {"width", "100%"},
          },
      },

      // Bottom panel
      {
          ".bottom-panel",
          {
              {"position", "absolute"},
              {"top", "auto"},
              {"bottom", "0"},
              {"width", "100%"},
          },
      },

      {
          ".bottom-panel Button",
          {
              {"font-size", "40"},
          },
      },

      // Choice GUI
      {
          ".choice-wrapper .choice-info",
          {
              {"align-horizontal", "left"},
              {"width", "67%"},
          },
      },

      {
          ".choice-wrapper .choice-queue",
          {
              {"width", "100%"},
              {"height", "100%"},
              {"align-vertical", "top"},
              {"align-horizontal", "left"},

          },
      },

      {
          ".choice-wrapper .choice-left",
          {
              {"width", "70%"},
              {"height", "100%"},
          },
      },

      {
          ".choice-wrapper .choice-right",
          {
              {"width", "25%"},
              {"height", "100%"},
          },
      },

      {
          ".choice-wrapper",
          {
              {"width", "100%"},
              {"height", "85%"},

          },
      },

      // Solar GUI
      {
          ".solar-main-panel",
          {
              {"height", "67%"},
          },
      },

      {
          ".solar-main-panel .solar-block",
          {
              {"width", "45%"},
              {"height", "90%"},
          },
      },

      {
          ".solar-main-panel .solar-component",
          {
              {"align-horizontal", "left"},
              {"align-vertical", "top"},
              {"width", "50%"},
              {"height", "100%"},
              {"overflow", "scrolled"},
          },
      },

      {
          ".solar-main-panel .solar-queue",
          {
              {"width", "100%"},
              {"height", "100%"},
          },
      },

      {
          ".solar-main-panel .solar-component .card",
          {
              {"width", "90%"},
              {"height", "40px"},
          },
      },

      // Command GUI
      {
          ".command-gui Slider",
          {
              {"width", "90%"},
              {"height", "40px"},
          },
      },

      // Target GUI
      {
          ".target-gui",
          StyleMap{
              {"width", "300px"},
          } + s_default2,
      },

      {
          ".target-gui .target-wrapper .label",
          {
              {"height", "30px"},
          },
      },

      {
          ".target-gui .target-wrapper ButtonOptions",
          {
              {"width", "100%"},
              {"height", "50px"},
          },
      },

      // General styles
      {
          ".button-option",
          {
              {"width", "20%"},
              {"height", "30px"},
          },
      },

      {
          ".label",
          StyleMap{
              {"padding-left", "0"},
              {"padding-right", "0"},
              {"padding-top", "0"},
              {"padding-bottom", "0"},
              {"margin-left", "5"},
              {"margin-right", "5"},
              {"margin-top", "5"},
              {"margin-bottom", "5"},
          } + s_transparent,
      },

      {
          ".hbar",
          {
              {"width", "100%"},
              {"height", "2px"},
              {"border-thickness", "0"},
              {"background-color", bar_color},
              {"margin-top", "10"},
              {"margin-bottom", "10"},
              {"margin-left", "0"},
              {"margin-right", "0"},
          },
      },

      {
          ".vbar",
          {
              {"height", "100%"},
              {"width", "2px"},
              {"border-thickness", "0"},
              {"background-color", bar_color},
              {"margin-left", "10"},
              {"margin-right", "10"},
              {"margin-top", "0"},
              {"margin-bottom", "0"},
          },
      },

      {
          ".card:hover",
          {
              {"background-color", "345678ff"},
          },
      },

      {
          ".card:selected",
          {
              {"color", "ffffffff"},
              {"background-color", "654321aa"},
              {"border-thickness", "4"},
              {"border-color", "ffffffaa"},
          },
      },

      {
          ".card",
          {
              {"font-size", "16"},
              {"background-color", "123456ff"},
              {"border-thickness", "2"},
              {"border-color", "ddeeff88"},
          },
      },

      {
          ".section",
          {
              {"width", "100%"},
              {"overflow", "scrolled"},
              {"margin-top", "10"},
              {"margin-bottom", "10"},
          },
      },

      // Modifiers

      {".default-panel", s_default1},
      {".default-panel2", s_default2},

      {
          ".list-item",
          {
              {"font-size", "12"},
          },
      },

      {
          ".h1",
          StyleMap{
              {"font-size", "25"},
          } + s_transparent,
      },

      {".column2", {{"width", "50%"}}},
      {".column3", {{"width", "33%"}}},
      {".column4", {{"width", "25%"}}},
      {".column5", {{"width", "20%"}}},

      {".scrolled", {{"overflow", "scrolled"}}},

      {
          ".abs-top-left",
          {
              {"position", "absolute"},
              {"top", "10"},
              {"left", "10"},
          },
      },

      {".transparent", s_transparent},

      {".fill-container", {{"width", "100%"}, {"height", "100%"}, {"position", "absolute"}}},

  };
}

ButtonPtr st3::make_label(string v) {
  return tag({"label"}, Button::create(v));
}

ButtonPtr st3::make_hbar() {
  return tag({"hbar"}, Button::create(""));
}

ButtonPtr st3::make_vbar() {
  return tag({"vbar"}, Button::create(""));
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

PanelPtr st3::spaced_ul(ComponentPtr c) {
  return tag(
      {"transparent"},
      with_style(
          {{"align-horizontal", "left"}, {"align-vertical", "top"}, {"width", "100%"}},
          Panel::create({c})));
}