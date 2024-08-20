#ifndef BEAM_APPLICATION_INCLUDED
#define BEAM_APPLICATION_INCLUDED

#include <free_camera_controller.hpp>
#include <perspective_camera.hpp>

#include <niku_application.hpp>
#include <niku_mouse.hpp>

#include <SDL2/SDL_events.h>

#include <memory>

namespace vkrndr
{
    class scene;
} // namespace vkrndr

namespace beam
{
    class scene;
    class raytracer;
} // namespace beam

namespace beam
{
    class [[nodiscard]] application final : public niku::application
    {
    public:
        explicit application(bool debug);

        application(application const&) = delete;

        application(application&&) noexcept = delete;

    public:
        ~application() override;

    public:
        // cppcheck-suppress duplInheritedMember
        application& operator=(application const&) = delete;

        // cppcheck-suppress duplInheritedMember
        application& operator=(application&&) noexcept = delete;

    private: // niku::application callback interface
        bool handle_event([[maybe_unused]] SDL_Event const& event) override;

        void update(float delta_time) override;

        [[nodiscard]] vkrndr::scene* render_scene() override;

    private:
        perspective_camera camera_;
        niku::mouse mouse_;

        free_camera_controller camera_controller_;

        std::unique_ptr<scene> scene_;
        std::unique_ptr<raytracer> raytracer_;
    };
} // namespace beam
#endif
