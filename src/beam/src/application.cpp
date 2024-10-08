#include <application.hpp>
#include <free_camera_controller.hpp>
#include <perspective_camera.hpp>
#include <raytracer.hpp>
#include <renderer.hpp>

#include <cppext_numeric.hpp>

#include <niku_application.hpp>
#include <niku_mouse.hpp>

#include <vulkan_renderer.hpp>

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_scancode.h>
#include <SDL2/SDL_video.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>

beam::application::application(bool const debug)
    : niku::application{niku::startup_params{
          .init_subsystems = {.video = true, .audio = false, .debug = debug},
          .title = "beam",
          .window_flags = static_cast<SDL_WindowFlags>(
              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI),
          .centered = true,
          .width = 512,
          .height = 512,
          .render = {.preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR}}}
    , mouse_{debug}
    , camera_controller_{&camera_, &mouse_}
    , renderer_{std::make_unique<renderer>(this->vulkan_device(),
          this->vulkan_renderer(),
          this->vulkan_renderer()->extent())}
    , raytracer_{std::make_unique<raytracer>(this->vulkan_device(),
          this->vulkan_renderer(),
          renderer_.get())}
{
    this->vulkan_renderer()->imgui_layer(true);

    camera_.set_position({13.0f, 2.0f, 3.0f});
    camera_.set_yaw_pitch({-167.0f, -3.0f});
    camera_.resize({512, 512});
    camera_.update();

    renderer_->set_raytracer(raytracer_.get());
    raytracer_->update(camera_);
}

beam::application::~application() = default;

bool beam::application::handle_event(SDL_Event const& event)
{
    camera_controller_.handle_event(event);

    if (event.type == SDL_WINDOWEVENT)
    {
        auto const& window{event.window};
        if (window.event == SDL_WINDOWEVENT_RESIZED ||
            window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            renderer_->resize({cppext::narrow<uint32_t>(window.data1),
                cppext::narrow<uint32_t>(window.data2)});
        }

        return true;
    }

    if (event.type == SDL_KEYDOWN)
    {
        auto const& keyboard{event.key};
        if (keyboard.keysym.scancode == SDL_SCANCODE_F3)
        {
            mouse_.set_capture(!mouse_.captured());
        }
        else if (keyboard.keysym.scancode == SDL_SCANCODE_F4)
        {
            this->vulkan_renderer()->imgui_layer(
                !this->vulkan_renderer()->imgui_layer());
        }

        return true;
    }

    return false;
}

void beam::application::update(float delta_time)
{
    if (camera_controller_.update(delta_time))
    {
        raytracer_->update(camera_);
    }
}

vkrndr::scene* beam::application::render_scene() { return renderer_.get(); }
