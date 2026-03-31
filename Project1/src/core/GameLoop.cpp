#include "GameLoop.h"
#define NOMINMAX
#include <windows.h>
#include <algorithm>

GameLoop::GameLoop() {
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*> (&_frequency));
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*> (&_last_time));
    _accumulator = 0.0;
}

double GameLoop::Tick() {
    int64_t current_time;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*> (&current_time));
    double dt = (double)(current_time - _last_time) / (double)_frequency;
    dt = std::min(dt, 0.1);
    _last_time = current_time;
    _accumulator += dt;
    return dt;
}

bool GameLoop::ShouldUpdate() {
    return _accumulator >= FIXED_DT;
}

void GameLoop::ConsumeUpdate() {
    _accumulator -= FIXED_DT;
}

double GameLoop::GetAlpha() const {
    return _accumulator / FIXED_DT;
}
