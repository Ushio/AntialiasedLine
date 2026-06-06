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
void draw_antialised_line(float p0_x, float p0_y, float p1_x, float p1_y, PutPixel put_pixel_f, bool transposed = false)
{
    float dydx = (p1_y - p0_y) / (p1_x - p0_x);

    if (transposed == false && 1.0f < fabs(dydx))
    {
        draw_antialised_line(p0_y, p0_x, p1_y, p1_x, put_pixel_f, true);
        return;
    }

    if (p1_x < p0_x)
    {
        std::swap(p0_x, p1_x);
        std::swap(p1_y, p0_y);
    }

    int x0 = roundf(p0_x);
    int x1 = roundf(p1_x);

    float step_to_align = x0 - p0_x;
    float beg_y = p0_y + dydx * step_to_align;

    int steps = x1 - x0;
    if (steps == 0)
    {
        int y = roundf(p0_y);
        if (transposed)
        {
            put_pixel_f(x0, y, 0.5f);
        }
        else
        {
            put_pixel_f(y, x0, 0.5f);
        }
        return;
    }
    for (int i = 0; i <= steps; i++)
    {
        int ix = x0 + i;
        float x_here = x0 + i;
        float y_here = beg_y + dydx * i;

        // debug only 
        //if (transposed)
        //{
        //    pr::DrawPoint({ y_here,x_here,  0 }, { 255, 0, 0 }, 10);
        //}
        //else
        //{
        //    pr::DrawPoint({ x_here, y_here, 0 }, { 255, 0, 0 }, 10);
        //}

        int center0 = floor(y_here);
        int center1 = center0 + 1;
        float distance0 = fabs(center0 - y_here);

        float e0 = ss_max(1.0f - distance0, 0.0f);
        float e1 = distance0;

        if (transposed)
        {
            put_pixel_f(center0, ix, e0);
            put_pixel_f(center1, ix, e1);
        }
        else
        {
            put_pixel_f(ix, center0, e0);
            put_pixel_f(ix, center1, e1);
        }
    }
}

int main() {
    using namespace pr;

    SetDataDir(ExecutableDir());

    Config config;
    config.ScreenWidth = 1920;
    config.ScreenHeight = 1080;
    config.SwapInterval = 1;
    Initialize(config);

    Camera3D camera;
    camera.origin = { 0, 0, 10 };
    camera.lookat = { 0, 0, 0 };
    camera.zFar = 10000;
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
        static glm::vec3 p2 = { 4.0f, 7.0f, 0.0f };

        draw_antialised_line(p0.x, p0.y, p1.x, p1.y, [](int x, int y, float energy) {
            // super rough approx of x^(1 - 2.2)
            float c = sqrtf(energy);
            int c8 = c * 255.0f + 0.5f; // nearest neighbor
            put_pixel(x, y, ss_min(c8, 255));
        });

        //draw_antialised_line(p1.x, p1.y, p2.x, p2.y, [](int x, int y, float energy) {
        //    // super rough approx of x^(1 - 2.2)
        //    float c = sqrtf(energy);
        //    int c8 = c * 255.0f + 0.5f; // nearest neighbor
        //    put_pixel(x, y, ss_min(c8, 255));
        //});

        // corner case
        //draw_antialised_line(1, 10, 1, 10, 0.5f, [](int x, int y, float energy) {
        //    // super rough approx of x^(1 - 2.2)
        //    float c = sqrtf(energy);
        //    int c8 = c * 255.0f + 0.5f; // nearest neighbor
        //    put_pixel(x, y, ss_min(c8, 255));
        //});

        int N = 64;
        pr::CircleGenerator c(glm::pi<float>() * 2.0f / N);
        for (int i = 0; i < N; i++)
        {
            float from_x = c.cos() * 200.0f;
            float from_y = c.sin() * 200.0f;

            c.step();

            float to_x = c.cos() * 200.0f;
            float to_y = c.sin() * 200.0f;

            draw_antialised_line(from_x, from_y, to_x, to_y, [](int x, int y, float energy) {
                // super rough approx of x^(1 - 2.2)
                float c = sqrtf(energy);
                int c8 = c * 255.0f + 0.5f; // nearest neighbor
                put_pixel(x, y, ss_min(c8, 255));
            });
        }

        ManipulatePosition(camera, &p0, 2);
        ManipulatePosition(camera, &p1, 2);
        ManipulatePosition(camera, &p2, 2);

        PopGraphicState();
        EndCamera();

        BeginImGui();

        ImGui::SetNextWindowSize({ 500, 800 }, ImGuiCond_Once);
        ImGui::Begin("Panel");
        ImGui::Text("fps = %f", GetFrameRate());

        if (ImGui::Button("save"))
        {
            Image2DRGBA8 image;
            int size = 512;
            image.allocate(size, size);
            for (int y = 0; y < image.height(); y++)
            for (int x = 0; x < image.width(); x++)
            {
                image(x, y) = { 0, 0, 0, 255 };
            }

            int N = 64;
            pr::CircleGenerator c(glm::pi<float>() * 2.0f / N);
            for (int i = 0; i < N; i++)
            {
                float from_x = 255.0f + c.cos() * 200.0f;
                float from_y = 255.0f + c.sin() * 200.0f;

                c.step();

                float to_x = 255.0f + c.cos() * 200.0f;
                float to_y = 255.0f + c.sin() * 200.0f;

                draw_antialised_line(from_x, from_y, to_x, to_y, [&image](int x, int y, float energy) {
                    // super rough approx of x^(1 - 2.2)
                    float c = sqrtf(energy);
                    int c8 = c * 255.0f + 0.5f; // nearest neighbor
                    c8 = ss_min(c8, 255);

                    int r = image(x, y).r;

                    //// screen composition. very heuristic.. but looks not bad.
                    //int composite = 255 - (255 - c8) * (255 - r) / 255;
                    //image(x, y) = { composite, composite, composite, 255};

                    // don't care
                    image(x, y) = { c8, c8, c8, 255 };
                });
            }
            image.saveAsPng("circle.png");
        }

        ImGui::End();

        EndImGui();
    }

    pr::CleanUp();
}
