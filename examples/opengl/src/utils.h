#pragma once

#include <GL/glew.h>

#include <string>
#include <string_view>
#include <chrono>

GLuint load_shader(std::string_view vert, std::string_view frag);

std::string read_file(std::string_view file);

struct Timer
{
    using clock = std::chrono::high_resolution_clock;
    using time_point = clock::time_point;
    using micro_seconds = std::chrono::microseconds;

    time_point start;

    inline Timer() : start(get()) {}

    inline float elapsed() const
    {
        auto diff = get() - start;
        auto duration_us = std::chrono::duration_cast<micro_seconds>(diff);
        return static_cast<float>(duration_us.count()) / 1000.0f;
    }

    static inline time_point get() { return clock::now(); }

    inline void reset() { start = get(); }
};