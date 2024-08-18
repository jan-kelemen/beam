#ifndef BEAM_SCENE_INCLUDED
#define BEAM_SCENE_INCLUDED

#include <raytracer.hpp>

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
    class [[nodiscard]] scene final : public vkrndr::scene
    {
    public:
        scene(vkrndr::vulkan_device* const device,
            vkrndr::vulkan_renderer* const renderer,
            VkExtent2D const extent);

        scene(scene const&) = delete;

        scene(scene&&) noexcept = delete;

    public:
        ~scene() override;

    public:
        [[nodiscard]] vkrndr::vulkan_image& color_image();

    public: // vkrndr::scene overrides
        void resize(VkExtent2D const extent) override;

        void draw(vkrndr::vulkan_image const& target_image,
            VkCommandBuffer command_buffer,
            VkExtent2D extent) override;

        void draw_imgui() override;

    public:
        scene& operator=(scene const&) = delete;

        scene& operator=(scene&&) = delete;

    private:
        vkrndr::vulkan_image create_color_image(VkExtent2D const extent);

    private:
        vkrndr::vulkan_device* device_;
        vkrndr::vulkan_renderer* renderer_;

        vkrndr::vulkan_image color_image_;

        raytracer raytracer_;
    };
} // namespace beam

#endif
