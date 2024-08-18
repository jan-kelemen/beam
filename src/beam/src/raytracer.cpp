#include <raytracer.hpp>

#include <scene.hpp>

#include <cppext_numeric.hpp>

#include <vulkan_descriptors.hpp>
#include <vulkan_device.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_utility.hpp>

#include <vulkan/vulkan_core.h>

#include <array>

namespace
{
    [[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(
        vkrndr::vulkan_device const* const device)
    {
        VkDescriptorSetLayoutBinding target_image_binding{};
        target_image_binding.binding = 0;
        target_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        target_image_binding.descriptorCount = 1;
        target_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array const bindings{target_image_binding};

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = vkrndr::count_cast(bindings.size());
        layout_info.pBindings = bindings.data();

        VkDescriptorSetLayout rv; // NOLINT
        vkrndr::check_result(vkCreateDescriptorSetLayout(device->logical,
            &layout_info,
            nullptr,
            &rv));

        return rv;
    }

    void bind_descriptor_set(vkrndr::vulkan_device const* const device,
        VkDescriptorSet const& descriptor_set,
        VkDescriptorImageInfo const target_image_info)
    {
        VkWriteDescriptorSet target_image_write{};
        target_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        target_image_write.dstSet = descriptor_set;
        target_image_write.dstBinding = 0;
        target_image_write.dstArrayElement = 0;
        target_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        target_image_write.descriptorCount = 1;
        target_image_write.pImageInfo = &target_image_info;

        std::array const descriptor_writes{target_image_write};

        vkUpdateDescriptorSets(device->logical,
            vkrndr::count_cast(descriptor_writes.size()),
            descriptor_writes.data(),
            0,
            nullptr);
    }
} // namespace

beam::raytracer::raytracer(vkrndr::vulkan_device* device,
    vkrndr::vulkan_renderer* renderer,
    scene* scene)
    : device_{device}
    , renderer_{renderer}
    , scene_{scene}
    , descriptor_layout_{create_descriptor_set_layout(device_)}
{
    vkrndr::create_descriptor_sets(device_,
        descriptor_layout_,
        renderer_->descriptor_pool(),
        std::span{&descriptor_set_, 1});

    compute_pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
        vkrndr::vulkan_compute_pipeline_builder{device_,
            vkrndr::vulkan_pipeline_layout_builder{device_}
                .add_descriptor_set_layout(descriptor_layout_)
                .build()}
            .with_shader("raytracer.comp.spv", "main")
            .build());

    bind_descriptor_set(device_,
        descriptor_set_,
        VkDescriptorImageInfo{.imageView = scene_->color_image().view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
}

beam::raytracer::~raytracer()
{
    destroy(device_, compute_pipeline_.get());

    vkDestroyDescriptorSetLayout(device_->logical, descriptor_layout_, nullptr);
}

void beam::raytracer::draw(VkCommandBuffer command_buffer)
{
    vkrndr::bind_pipeline(command_buffer,
        *compute_pipeline_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        0,
        std::span{&descriptor_set_, 1});

    vkCmdDispatch(command_buffer,
        static_cast<uint32_t>(std::ceil(
            cppext::as_fp(scene_->color_image().extent.width) / 16.0f)),
        static_cast<uint32_t>(std::ceil(
            cppext::as_fp(scene_->color_image().extent.height) / 16.0f)),
        1);
}

void beam::raytracer::on_resize()
{
    bind_descriptor_set(device_,
        descriptor_set_,
        VkDescriptorImageInfo{.imageView = scene_->color_image().view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
}
