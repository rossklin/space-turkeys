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

  // Main panel
  Component::tag_style["main-panel"] = {
      {"width", "90%"},
      {"height", "90%"},
      {"background-color", "123456bb"},
      {"color", "ffccaaff"},
  };

  // Right panel
  Component::tag_style["right-panel"] = {
      {"position", "absolute"},
      {"right", "0"},
      {"left", "auto"},
      {"top", "0"},
      {"width", "25%"},
      {"height", "100%"},
      {"background-color", "ccddee66"},
      {"border-color", "33557755"},
      {"border-thickness", "5"},
      {"align-vertical", "top"},
  };

  Component::tag_style["right-panel-button"] = {
      {"width", "90%"},
      {"height", "30px"},
  };

  Component::tag_style["right-panel-box"] = {
      {"width", "100%"},
      {"height", "200px"},
      {"align-horizontal", "left"},
      {"align-vertical", "top"},
      {"padding-top", "5"},
  };

  Component::tag_style["event-item"] = {
      {"margin-left", "0"},
      {"margin-right", "0"},
      {"margin-top", "5"},
      {"margin-bottom", "5"},
      {"width", "100%"},
  };

  // Bottom panel
  Component::tag_style["bottom-panel"] = {
      {"position", "absolute"},
      {"top", "auto"},
      {"bottom", "0"},
      {"width", "100%"},
  };

  Component::tag_style["main-commit-button"] = {
      {"font-size", "40"},
  };

  // Choice GUI
  Component::tag_style["choice-gui-info"] = {
      {"background-color", "00000000"},
      {"border-thickness", "0"},
      {"align-horizontal", "left"},
      {"width", "67%"},
  };

  Component::tag_style["choice-queue"] = {
      {"width", "100%"},
      {"height", "100%"},
      {"align-vertical", "top"},
      {"align-horizontal", "left"},
  };

  Component::tag_style["choice-wrapper"] = {
      {"width", "100%"},
      {"height", "85%"},
      {"background-color", "00000000"},
      {"border-thickness", "0"},
  };

  Component::tag_style["choice-left"] = {
      {"width", "70%"},
      {"height", "100%"},
      {"border-thickness", "1"},
  };

  Component::tag_style["choice-right"] = {
      {"width", "25%"},
      {"height", "100%"},
  };

  // Solar GUI
  Component::tag_style["solar-main-panel"] = {
      {"height", "67%"},
  };

  Component::tag_style["solar-block"] = {
      {"width", "45%"},
      {"height", "90%"},
  };

  Component::tag_style["solar-component"] = {
      {"align-horizontal", "left"},
      {"align-vertical", "top"},
      {"width", "50%"},
      {"height", "100%"},
      {"overflow", "scrolled"},
  };

  Component::tag_style["solar-queue"] = {
      {"width", "100%"},
      {"height", "100%"},
      {"background-color", "00000000"},
      {"border-thickness", "0"},
  };

  Component::tag_style["solar-gui-button"] = {
      {"width", "90%"},
      {"height", "40px"},
  };

  // General styles

  Component::tag_style["button-option"] = {
      {"width", "20%"},
      {"height", "30px"},
  };

  Component::tag_style["default-panel"] = {
      {"background-color", "12345688"},
      {"color", "ffccaaff"},
  };

  Component::tag_style["abs-top-left"] = {
      {"position", "absolute"},
      {"top", "10"},
      {"left", "10"},
  };

  Component::tag_style["transparent"] = {
      {"background-color", "00000000"},
      {"border-thickness", "0"},
  };

  Component::tag_style["label"] = {
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
  };

  Component::tag_style["list-item"] = {
      {"font-size", "12"},
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
      {"font-size", "18"},
      {"background-color", "123456ff"},
      {"border-thickness", "2"},
      {"border-color", "ddeeff88"},
  };

  Component::tag_style["card:hover"] = {
      {"background-color", "345678ff"},
  };

  Component::tag_style["card:selected"] = {
      {"color", "ffffffff"},
      {"background-color", "654321aa"},
      {"border-thickness", "4"},
      {"border-color", "ffffffaa"},
  };

  Component::tag_style["section"] = {
      {"width", "100%"},
      {"overflow", "scrolled"},
      {"margin-top", "10"},
      {"margin-bottom", "10"},
      {"border-thickness", "0"},
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

PanelPtr st3::spaced_ul(ComponentPtr c) {
  return tag(
      {"transparent"},
      with_style(
          {{"align-horizontal", "left"}, {"align-vertical", "top"}, {"width", "100%"}},
          Panel::create({c})));
}