#ifndef BEAM_RAYTRACER_INCLUDED
#define BEAM_RAYTRACER_INCLUDED

#include <sphere.hpp>

#include <vulkan_buffer.hpp>

#include <glm/vec3.hpp>

#include <vulkan/vulkan_core.h>

#include <memory>
#include <span>

namespace vkrndr
{
    struct vulkan_device;
    struct vulkan_pipeline;
    class vulkan_renderer;
} // namespace vkrndr

namespace beam
{
    class scene;
    class perspective_camera;
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
        void update(perspective_camera const& camera);

        void draw(VkCommandBuffer command_buffer);

        void on_resize();

        void draw_imgui();

    public:
        raytracer& operator=(raytracer const&) = delete;

        raytracer& operator=(raytracer&&) noexcept = delete;

    private:
        void fill_world(std::span<sphere const> spheres);
        void fill_materials(std::span<material const> materials);
        void fill_world_and_materials();

    private:
        vkrndr::vulkan_device* device_;
        vkrndr::vulkan_renderer* renderer_;
        scene* scene_;

        VkDescriptorSetLayout descriptor_layout_;
        VkDescriptorSet descriptor_set_;

        std::unique_ptr<vkrndr::vulkan_pipeline> compute_pipeline_;

        int samples_per_pixel_{1};
        int max_depth_{5};
        uint32_t total_samples_{0};

        glm::vec3 camera_position_;
        glm::vec3 camera_front_;
        glm::vec3 camera_up_;

        float fovy_{20.0f};

        float defocus_angle_{0.6f};
        float focus_distance_{10.0f};

        vkrndr::vulkan_buffer world_buffer_;
        uint32_t sphere_count_;
        vkrndr::vulkan_buffer material_buffer_;
        uint32_t material_count_;
    };
} // namespace beam

#endif
