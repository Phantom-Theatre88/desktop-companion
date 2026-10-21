#pragma once
#include "Arduino.h"
#include <vector>
#include <cassert>
constexpr uint32_t TFT_WHITE = 0xffff, TFT_RED = 0xf800, TFT_BLACK = 0, TFT_CYAN = 0x07ff;
constexpr int middle_center = 4;
struct DrawCall { int x, y, w, h, radius; uint32_t color; };
class M5GFX {
 public:
  int width() const { return 320; }
  int height() const { return 240; }
};
class M5Canvas {
 public:
  static std::vector<DrawCall> rects;
  static int pushes;
  explicit M5Canvas(M5GFX*) {}
  void setColorDepth(int) {}
  void createSprite(int, int) {}
  void fillSprite(uint32_t) { rects.clear(); }
  void fillRoundRect(int x, int y, int w, int h, int r, uint32_t c) {
    assert(w > 0 && h > 0 && r >= 0 && r <= w/2 && r <= h/2);
    assert(x >= 0 && y >= 0 && x+w <= 320 && y+h <= 240);
    rects.push_back({x,y,w,h,r,c});
  }
  void fillRect(int x, int y, int w, int h, uint32_t c) {
    assert(w > 0 && h > 0);
    rects.push_back({x,y,w,h,0,c});
  }
  void fillTriangle(int,int,int,int,int,int,uint32_t) {}
  // Platform-only shims for auxiliary mouth/prop/effect rendering.
  void fillEllipse(int,int,int,int,uint32_t) {}
  void drawCircle(int,int,int,uint32_t) {}
  void fillCircle(int,int,int,uint32_t) {}
  void drawLine(int,int,int,int,uint32_t) {}
  void setTextDatum(int) {}
  void setTextSize(int) {}
  void setTextColor(uint32_t,uint32_t) {}
  void drawString(const char*,int,int) {}
  void pushSprite(int,int) { ++pushes; }
};
