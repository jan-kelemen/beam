#ifndef BEAM_RAYTRACER_INCLUDED
#define BEAM_RAYTRACER_INCLUDED

#include <vulkan_buffer.hpp>

#include <glm/vec3.hpp>

#include <vulkan/vulkan_core.h>

#include <memory>

namespace vkrndr
{
    struct vulkan_device;
    struct vulkan_pipeline;
    class vulkan_renderer;
} // namespace vkrndr

namespace beam
{
    class scene;
} // namespace beam

namespace beam
{
    class [[nodiscard]] raytracer final
    {
    public:
        raytracer(vkrndr::vulkan_device* device,
            vkrndr::vulkan_renderer* renderer,
            scene* scene);

        raytracer(raytracer const&) = delete;

        raytracer(raytracer&&) noexcept = delete;

    public:
        ~raytracer();

    public:
        void draw(VkCommandBuffer command_buffer);

        void on_resize();

        void draw_imgui();

    public:
        raytracer& operator=(raytracer const&) = delete;

        raytracer& operator=(raytracer&&) noexcept = delete;

    private:
        void fill_world();

    private:
        vkrndr::vulkan_device* device_;
        vkrndr::vulkan_renderer* renderer_;
        scene* scene_;

        VkDescriptorSetLayout descriptor_layout_;
        VkDescriptorSet descriptor_set_;

        std::unique_ptr<vkrndr::vulkan_pipeline> compute_pipeline_;

        glm::vec3 camera_position_{0.0f, 0.0f, 0.0f};

        vkrndr::vulkan_buffer world_buffer_;
    };
} // namespace beam

#endif
