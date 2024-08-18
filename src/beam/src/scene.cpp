#include <scene.hpp>

#include <cppext_numeric.hpp>

#include <vkrndr_render_pass.hpp>
#include <vulkan_commands.hpp>
#include <vulkan_device.hpp>
#include <vulkan_image.hpp>
#include <vulkan_renderer.hpp>

#include <imgui.h>

#include <vulkan/vulkan_core.h>

beam::scene::scene(vkrndr::vulkan_device* const device,
    vkrndr::vulkan_renderer* const renderer,
    VkExtent2D const extent)
    : device_{device}
    , renderer_{renderer}
    , color_image_{create_color_image(extent)}
    , raytracer_{device, renderer, this}
{
}

beam::scene::~scene() { destroy(device_, &color_image_); }

vkrndr::vulkan_image& beam::scene::color_image() { return color_image_; }

void beam::scene::resize(VkExtent2D const extent)
{
    vkDeviceWaitIdle(device_->logical); // TODO-JK

    destroy(device_, &color_image_);
    color_image_ = create_color_image(extent);
    raytracer_.on_resize();
}

void beam::scene::draw(vkrndr::vulkan_image const& target_image,
    VkCommandBuffer command_buffer,
    VkExtent2D extent)
{
    VkViewport const viewport{.x = 0.0f,
        .y = 0.0f,
        .width = cppext::as_fp(extent.width),
        .height = cppext::as_fp(extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f};
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D const scissor{{0, 0}, extent};
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkrndr::transition_image(color_image_.image,
        command_buffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_PIPELINE_STAGE_2_BLIT_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        1);

    raytracer_.draw(command_buffer);

    vkrndr::transition_image(color_image_.image,
        command_buffer,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_2_BLIT_BIT,
        VK_ACCESS_2_TRANSFER_READ_BIT,
        1);

    vkrndr::transition_image(target_image.image,
        command_buffer,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_ACCESS_2_NONE,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        1);

    VkOffset3D size{
        .x = cppext::narrow<int32_t>(
            std::min(color_image_.extent.width, target_image.extent.width)),
        .y = cppext::narrow<int32_t>(
            std::min(color_image_.extent.height, target_image.extent.height)),
        .z = 1};
    VkImageBlit region{};
    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount = 1;
    region.srcOffsets[1] = size;
    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount = 1;
    region.dstOffsets[1] = size;

    vkCmdBlitImage(command_buffer,
        color_image_.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        target_image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region,
        VK_FILTER_NEAREST);

    vkrndr::transition_image(target_image.image,
        command_buffer,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_BLIT_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_2_NONE,
        1);
}

void beam::scene::draw_imgui() { ImGui::ShowMetricsWindow(); }

vkrndr::vulkan_image beam::scene::create_color_image(VkExtent2D const extent)
{
    return vkrndr::create_image_and_view(device_,
        extent,
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_TILING_LINEAR,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
}
