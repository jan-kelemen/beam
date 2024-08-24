#ifndef BEAM_RENDERER_INCLUDED
#define BEAM_RENDERER_INCLUDED

#include <vkrndr_scene.hpp>
#include <vulkan_image.hpp>

#include <vulkan/vulkan_core.h>

namespace vkrndr
{
    class vulkan_renderer;
    struct vulkan_device;
} // namespace vkrndr

namespace beam
{
    class raytracer;
} // namespace beam

namespace beam
{
    class [[nodiscard]] renderer final : public vkrndr::scene
    {
    public:
        renderer(vkrndr::vulkan_device* device,
            vkrndr::vulkan_renderer* renderer,
            VkExtent2D extent);

        renderer(renderer const&) = delete;

        renderer(renderer&&) noexcept = delete;

    public:
        ~renderer() override;

    public:
        [[nodiscard]] vkrndr::vulkan_image& color_image();

        void set_raytracer(raytracer* raytracer);

    public: // vkrndr::scene overrides
        void resize(VkExtent2D extent) override;

        void draw(vkrndr::vulkan_image const& target_image,
            VkCommandBuffer command_buffer,
            VkExtent2D extent) override;

        void draw_imgui() override;

    public:
        scene& operator=(scene const&) = delete;

        scene& operator=(scene&&) = delete;

    private:
        vkrndr::vulkan_image create_color_image(VkExtent2D extent) const;

    private:
        vkrndr::vulkan_device* device_;
        vkrndr::vulkan_renderer* renderer_;

        raytracer* raytracer_{};

        vkrndr::vulkan_image color_image_;
    };
} // namespace beam

#endif
