#include <application.hpp>
#include <scene.hpp>

#include <cppext_numeric.hpp>

#include <niku_application.hpp>

#include <vulkan_renderer.hpp>

#include <SDL2/SDL_video.h>

#include <imgui.h>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <optional>

beam::application::application(bool const debug)
    : niku::application{niku::startup_params{
          .init_subsystems = {.video = true, .audio = false, .debug = debug},
          .title = "beam",
          .window_flags = static_cast<SDL_WindowFlags>(
              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI),
          .centered = true,
          .width = 512,
          .height = 512,
          .render = {.preferred_swapchain_format = VK_FORMAT_B8G8R8_UNORM}}}
    , scene_{std::make_unique<scene>(this->vulkan_device(),
          this->vulkan_renderer(),
          this->vulkan_renderer()->extent())}
{
    this->vulkan_renderer()->imgui_layer(true);
}

beam::application::~application() { }

bool beam::application::handle_event(SDL_Event const& event)
{
    if (event.type == SDL_WINDOWEVENT)
    {
        auto const& window{event.window};
        if (window.event == SDL_WINDOWEVENT_RESIZED ||
            window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            scene_->resize({cppext::narrow<uint32_t>(window.data1),
                cppext::narrow<uint32_t>(window.data2)});
        }

        return true;
    }

    return false;
}

void beam::application::update([[maybe_unused]] float delta_time) { }

vkrndr::scene* beam::application::render_scene() { return scene_.get(); }
