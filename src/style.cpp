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

  // Debug
  Component::class_style["Panel"]["border-thickness"] = "1";
  Component::class_style["Panel"]["border-color"] = "ff448855";

  Component::tag_style = {
      // Main panel
      {
          ".main-panel",
          {
              {"width", "90%"},
              {"height", "90%"},
              {"background-color", "123456bb"},
              {"color", "ffccaaff"},
          },
      },

      // Right panel
      {
          ".right-panel",
          {
              {"position", "absolute"},
              {"right", "0"},
              {"left", "auto"},
              {"top", "0"},
              {"width", "25%"},
              {"height", "100%"},
              {"background-color", "99aabbaa"},
              {"border-color", "33557755"},
              {"border-thickness", "5"},
              {"align-vertical", "top"},
          },
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
              {"background-color", "00000000"},
              {"border-thickness", "0"},
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
              {"border-thickness", "1"},
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
              {"background-color", "00000000"},
              {"border-thickness", "0"},

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
              {"background-color", "00000000"},
              {"border-thickness", "0"},
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
          {
              {"background-color", "00000000"},
              {"border-thickness", "0"},
              {"padding-left", "0"},
              {"padding-right", "0"},
              {"padding-top", "0"},
              {"padding-bottom", "0"},
              {"margin-left", "5"},
              {"margin-right", "5"},
              {"margin-top", "5"},
              {"margin-bottom", "5"},
          },
      },

      {
          ".hbar",
          {
              {"width", "100%"},
              {"height", "2px"},
              {"border-thickness", "0"},
              {"background-color", "aabbcc88"},
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
              {"background-color", "aabbcc88"},
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
              {"border-thickness", "0"},
          },
      },

      // Modifiers

      {
          ".default-panel",
          {
              {"background-color", "12345688"},
              {"color", "ffccaaff"},
          },
      },

      {
          ".list-item",
          {
              {"font-size", "12"},
          },
      },

      {
          ".h1",
          {
              {"background-color", "00000000"},
              {"border-thickness", "0"},
              {"font-size", "25"},
          },
      },

      {
          ".abs-top-left",
          {
              {"position", "absolute"},
              {"top", "10"},
              {"left", "10"},
          },
      },

      {
          ".transparent",
          {
              {"background-color", "00000000"},
              {"border-thickness", "0"},
          },
      },
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