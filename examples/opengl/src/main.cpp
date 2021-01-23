#include <cmath>
#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <br.h>
#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include "utils.h"

using namespace glm;

void error_callback(int code, const char *message)
{
    std::cout << "GLFW Error: " << code << " :: " << message << std::endl;
}

struct Vertex
{
    Vertex() = default;
    Vertex(vec2 _v, vec2 _uv, vec4 _color) : v(_v), uv(_uv), color(_color)
    {
    }

    vec2 v;
    vec2 uv;
    vec4 color;
};

void upload_vertices(const BrushVertex *vertices, uint32_t count, GLuint buffer)
{
    if (count == 0)
        return;
    std::vector<Vertex> vertex_buffer(count * 6);
    for (uint i = 0; i < count; i++)
    {
        uint j = i * 6;
        const auto v0 =
            Vertex(vec2(vertices[i].min_x, vertices[i].min_y), vec2(vertices[i].uv_min_x, vertices[i].uv_min_y),
                   vec4(vertices[i].color_r, vertices[i].color_g, vertices[i].color_b, vertices[i].color_a));
        const auto v1 =
            Vertex(vec2(vertices[i].max_x, vertices[i].min_y), vec2(vertices[i].uv_max_x, vertices[i].uv_min_y),
                   vec4(vertices[i].color_r, vertices[i].color_g, vertices[i].color_b, vertices[i].color_a));
        const auto v2 =
            Vertex(vec2(vertices[i].max_x, vertices[i].max_y), vec2(vertices[i].uv_max_x, vertices[i].uv_max_y),
                   vec4(vertices[i].color_r, vertices[i].color_g, vertices[i].color_b, vertices[i].color_a));
        const auto v3 =
            Vertex(vec2(vertices[i].min_x, vertices[i].max_y), vec2(vertices[i].uv_min_x, vertices[i].uv_max_y),
                   vec4(vertices[i].color_r, vertices[i].color_g, vertices[i].color_b, vertices[i].color_a));

        vertex_buffer[j] = v0;
        vertex_buffer[j + 1] = v1;
        vertex_buffer[j + 2] = v2;
        vertex_buffer[j + 3] = v3;
        vertex_buffer[j + 4] = v0;
        vertex_buffer[j + 5] = v2;
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer.size() * sizeof(Vertex), vertex_buffer.data(), GL_DYNAMIC_DRAW);
}

GLint get_uniform_location(GLuint shader_id, std::string_view name)
{
    return glGetUniformLocation(shader_id, name.data());
}

int main()
{
    if (!glfwInit())
    {
        std::cout << "Could not initialize GLFW" << std::endl;
        return -1;
    }

    glfwSetErrorCallback(error_callback);

    const unsigned int width = 1280;
    const unsigned int height = 720;
    auto good_times = br_create_brush("fonts/good-times-rg.ttf");
    auto halo3 = br_add_font("fonts/Halo3.ttf");
    auto hemi_head = br_add_font("fonts/hemi-head-bd-it.ttf");
    auto pricedown = br_add_font("fonts/pricedown-bl.ttf");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(width, height, "opengl brush example", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Could not initialize GLFW window" << std::endl;
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    const auto error = glewInit();
    if (error != GLEW_NO_ERROR)
    {
        std::cout << "Could not initialize GLEW: " << glewGetErrorString(error) << std::endl;
    }

    glViewport(0, 0, width, height);

    auto shader_id = load_shader("shaders/vertex.vert", "shaders/fragment.frag");

    br_update();
    uint32_t count = 0;
    const BrushVertex *vertices = br_get_vertices(&count);
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    upload_vertices(vertices, count, buffer);

    GLuint texture = 0;
    uint32_t needs_update = 0;
    const uint8_t *tex_data = br_get_texture_data(&needs_update);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    uint32_t tex_width, tex_height;
    br_get_texture_dimensions(&tex_width, &tex_height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R, tex_width, tex_height, 0, GL_R, GL_UNSIGNED_BYTE, tex_data);

    GLuint vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)sizeof(vec2));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)sizeof(vec4));

    const mat4 matrix = scale(mat4(1.0f), vec3(1.0f, -1.0f, 1.0f)) *
                        ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -1.0f, 1.0f);

    Timer t;

    while (!glfwWindowShouldClose(window))
    {
        const float fps = 1000.0f / t.elapsed();
        t.reset();
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        std::string fps_str = std::to_string(fps);
        std::string text = "FPS: " + fps_str.substr(0, fps_str.size() - 3);

        for (int x = 10; x < width - 200; x += 200)
        {
            for (int y = 10; y < height - 100; y += 100)
            {
                auto section = TextSection();
                section.text = text.c_str();
                section.screen_position_x = static_cast<float>(x);
                section.screen_position_y = static_cast<float>(y);
                section.bounds_x = INFINITY;
                section.bounds_y = INFINITY;
                section.scale_x = 24.0f;
                section.scale_y = 24.0f;
                section.color_r = 1.0f;
                section.color_g = 1.0f;
                section.color_b = 1.0f;
                section.color_a = 1.0f;
                section.font_id = good_times;
                br_queue_text(section);

                section.screen_position_y += 20.0f;
                section.font_id = halo3;
                br_queue_text(section);

                section.screen_position_y += 20.0f;
                section.font_id = hemi_head;
                br_queue_text(section);

                section.screen_position_y += 20.0f;
                section.font_id = pricedown;
                br_queue_text(section);
            }
        }

        br_update();

        count = 0;
        needs_update = 0;
        upload_vertices(br_get_vertices(&count), count, buffer);

        br_get_texture_dimensions(&tex_width, &tex_height);
        br_get_texture_data(&needs_update);
        if (needs_update)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, tex_width, tex_height, 0, GL_RED, GL_UNSIGNED_BYTE, tex_data);

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        if (count > 0)
        {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(shader_id);
            glUniformMatrix4fv(get_uniform_location(shader_id, "matrix"), 1, GL_FALSE, value_ptr(matrix));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, count * 6);
        }

        glfwSwapBuffers(window);
    }

    return 0;
}
