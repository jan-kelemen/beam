#ifndef BEAM_FREE_CAMERA_CONTROLLER
#define BEAM_FREE_CAMERA_CONTROLLER

#include <glm/vec3.hpp>

#include <SDL2/SDL_events.h>

namespace niku
{
    class mouse;
} // namespace niku

namespace beam
{
    class perspective_camera;
} // namespace beam

namespace beam
{
    class [[nodiscard]] free_camera_controller final
    {
    public:
        free_camera_controller(perspective_camera* camera, niku::mouse* mouse);

        free_camera_controller(free_camera_controller const&) = default;

        free_camera_controller(free_camera_controller&&) noexcept = default;

    public:
        ~free_camera_controller() = default;

    public:
        void handle_event(SDL_Event const& event);

        void update(float delta_time);

    public:
        free_camera_controller& operator=(
            free_camera_controller const&) = default;

        free_camera_controller& operator=(
            free_camera_controller&&) noexcept = default;

    private:
        perspective_camera* camera_;
        niku::mouse* mouse_;
        bool update_needed_{false};

        glm::vec3 velocity_{0.0f, 0.0f, 0.0f};
    };
} // namespace beam

#endif
