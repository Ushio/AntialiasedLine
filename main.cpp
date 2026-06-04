#include "pr.hpp"
#include <iostream>
#include <memory>

void put_pixel(int ix, int iy, uint8_t brightness)
{
    using namespace pr;
    float x = ix - 0.5f;
    float y = iy - 0.5f;
    glm::uvec4 color = { brightness, brightness, brightness, 255 };
    TriBegin(0);
    TriVertex({ x, y, 0 }, { 0, 0 }, color);
    TriVertex({ x + 1.0f, y, 0 }, { 0, 0 }, color);
    TriVertex({ x + 1.0f, y + 1.0f, 0 }, { 0, 0 }, color);
    TriVertex({ x + 1.0f, y + 1.0f, 0 }, { 0, 0 }, color);
    TriVertex({ x, y + 1.0f, 0 }, { 0, 0 }, color);
    TriVertex({ x, y, 0 }, { 0, 0 }, color);
    TriEnd();
}

template <class T>
inline constexpr T ss_max(T x, T y)
{
    return (x < y) ? y : x;
}

template <class T>
inline constexpr T ss_min(T x, T y)
{
    return (y < x) ? y : x;
}

// Xiaolin Wu's line algorithm
template <class PutPixel>
void draw_antialised_line(float p0_x, float p0_y, float p1_x, float p1_y, PutPixel put_pixel_f)
{
    int x0 = roundf(p0_x);
    int x1 = roundf(p1_x);

    float dydx = (p1_y - p0_y) / (p1_x - p0_x);

    float step_to_align = x0 - p0_x;
    float beg_y = p0_y + dydx * step_to_align;
    float head_coverage = ss_max(x0 + 0.5f - p0_x, 0.0f);
    float tail_coverage = ss_max(p1_x - (x1 - 0.5f), 0.0f);

    int steps = x1 - x0;
    for (int i = 0; i <= steps; i++)
    {
        int ix = x0 + i;
        float x_here = x0 + i;
        float y_here = beg_y + dydx * i;
        pr::DrawPoint({ x_here, y_here, 0 }, { 255, 0, 0 }, 10);

        int center0 = floor(y_here);
        int center1 = ceil(y_here);
        float distance0 = fabs(center0 - y_here);

        float e0 = ss_max(1.0f - distance0, 0.0f);
        float e1 = distance0;

        if (i == 0) // head
        {
            e0 *= head_coverage;
            e1 *= head_coverage;
        }
        if (i == steps) // tail
        {
            e0 *= tail_coverage;
            e1 *= tail_coverage;
        }
        put_pixel_f(ix, center0, e0);
        put_pixel_f(ix, center1, e1);
    }
}

int main() {
    using namespace pr;

    Config config;
    config.ScreenWidth = 1920;
    config.ScreenHeight = 1080;
    config.SwapInterval = 1;
    Initialize(config);

    Camera3D camera;
    camera.origin = { 0, 0, 10 };
    camera.lookat = { 0, 0, 0 };

    double e = GetElapsedTime();

    while (pr::NextFrame() == false) {
        if (IsImGuiUsingMouse() == false) {
            UpdateCameraBlenderLike(&camera);
        }

        ClearBackground(0, 0, 0, 1);

        BeginCamera(camera);

        PushGraphicState();
        SetDepthTest(true);

        DrawXYZAxis(1.0f);

        //SetObjectTransform(glm::translate(glm::identity<glm::mat4>(), { -0.5f, -0.5f, 0.0f }));
        DrawGrid(GridAxis::XY, 1.0f, 20, { 128, 128, 128 });
        
        static glm::vec3 p0 = { 0.0f, 0.0f, 0.0f };
        static glm::vec3 p1 = { 5.0f, 1.0f, 0.0f };

        draw_antialised_line(p0.x, p0.y, p1.x, p1.y, [](int x, int y, float energy) {
            // super rough approx of x^(1 - 2.2)
            float c = sqrtf(energy);
            int c8 = c * 255.0f + 0.5f; // nearest neighbor
            put_pixel(x, y, ss_min(c8, 255));
        });

        ManipulatePosition(camera, &p0, 2);
        ManipulatePosition(camera, &p1, 2);

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
