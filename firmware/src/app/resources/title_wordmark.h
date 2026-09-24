#pragma once

#include <Arduino.h>

// Whale-Dock 艺术体标题字标 1bpp 位图（生成物，勿手改；见 title_wordmark.cpp 头注释）
namespace title_wordmark {
constexpr int W = 234;
constexpr int H = 48;
constexpr int STRIDE = 30;
constexpr int BYTES = 1440;
extern const uint8_t BITS[BYTES];
}  // namespace title_wordmark
