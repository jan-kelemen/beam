#include <raytracer.hpp>

#include <scene.hpp>
#include <sphere.hpp>

#include <cppext_numeric.hpp>

#include <vulkan_descriptors.hpp>
#include <vulkan_device.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_utility.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <glm/vec3.hpp>

#include <imgui.h>

#include <vulkan/vulkan_core.h>

#include <array>

namespace
{
    struct [[nodiscard]] push_constants
    {
        glm::vec3 camera_position;
        uint32_t world_count;
        uint32_t samples_per_pixel;
    };

    [[nodiscard]] VkDescriptorSetLayout create_descriptor_set_layout(
        vkrndr::vulkan_device const* const device)
    {
        VkDescriptorSetLayoutBinding target_image_binding{};
        target_image_binding.binding = 0;
        target_image_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        target_image_binding.descriptorCount = 1;
        target_image_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutBinding world_buffer_binding{};
        world_buffer_binding.binding = 1;
        world_buffer_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        world_buffer_binding.descriptorCount = 1;
        world_buffer_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array const bindings{target_image_binding, world_buffer_binding};

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
        VkDescriptorImageInfo const target_image_info,
        VkDescriptorBufferInfo const world_buffer_info)
    {
        VkWriteDescriptorSet target_image_write{};
        target_image_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        target_image_write.dstSet = descriptor_set;
        target_image_write.dstBinding = 0;
        target_image_write.dstArrayElement = 0;
        target_image_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        target_image_write.descriptorCount = 1;
        target_image_write.pImageInfo = &target_image_info;

        VkWriteDescriptorSet world_buffer_write{};
        world_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        world_buffer_write.dstSet = descriptor_set;
        world_buffer_write.dstBinding = 1;
        world_buffer_write.dstArrayElement = 0;
        world_buffer_write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        world_buffer_write.descriptorCount = 1;
        world_buffer_write.pBufferInfo = &world_buffer_info;

        std::array const descriptor_writes{target_image_write,
            world_buffer_write};

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
    fill_world();

    vkrndr::create_descriptor_sets(device_,
        descriptor_layout_,
        renderer_->descriptor_pool(),
        std::span{&descriptor_set_, 1});

    compute_pipeline_ = std::make_unique<vkrndr::vulkan_pipeline>(
        vkrndr::vulkan_compute_pipeline_builder{device_,
            vkrndr::vulkan_pipeline_layout_builder{device_}
                .add_descriptor_set_layout(descriptor_layout_)
                .add_push_constants(VkPushConstantRange{
                    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                    .offset = 0,
                    .size = sizeof(push_constants),
                })
                .build()}
            .with_shader("raytracer.comp.spv", "main")
            .build());

    bind_descriptor_set(device_,
        descriptor_set_,
        VkDescriptorImageInfo{.imageView = scene_->color_image().view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
        VkDescriptorBufferInfo{.buffer = world_buffer_.buffer,
            .offset = 0,
            .range = world_buffer_.size});
}

beam::raytracer::~raytracer()
{
    destroy(device_, &world_buffer_);

    destroy(device_, compute_pipeline_.get());

    vkDestroyDescriptorSetLayout(device_->logical, descriptor_layout_, nullptr);
}

void beam::raytracer::draw(VkCommandBuffer command_buffer)
{
    auto& target_extent{scene_->color_image().extent};

    push_constants const pc{.camera_position = camera_position_,
        .world_count = 2,
        .samples_per_pixel = cppext::narrow<uint32_t>(samples_per_pixel_)};

    vkCmdPushConstants(command_buffer,
        *compute_pipeline_->pipeline_layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(push_constants),
        &pc);

    vkrndr::bind_pipeline(command_buffer,
        *compute_pipeline_,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        0,
        std::span{&descriptor_set_, 1});

    vkCmdDispatch(command_buffer,
        static_cast<uint32_t>(
            std::ceil(cppext::as_fp(target_extent.width) / 16.0f)),
        static_cast<uint32_t>(
            std::ceil(cppext::as_fp(target_extent.height) / 16.0f)),
        1);
}

void beam::raytracer::on_resize()
{
    bind_descriptor_set(device_,
        descriptor_set_,
        VkDescriptorImageInfo{.imageView = scene_->color_image().view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
        VkDescriptorBufferInfo{.buffer = world_buffer_.buffer,
            .offset = 0,
            .range = world_buffer_.size});
}

void beam::raytracer::draw_imgui()
{
    ImGui::Begin("Raytracer");
    ImGui::SliderFloat3("Camera position",
        glm::value_ptr(camera_position_),
        -10.f,
        10.f);
    ImGui::SliderInt("Samples per pixel", &samples_per_pixel_, 1, 10);
    ImGui::End();
}

void beam::raytracer::fill_world()
{
    vkrndr::vulkan_buffer staging_buffer{vkrndr::create_buffer(*device_,
        2 * sizeof(sphere),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    {
        auto world_map{vkrndr::map_memory(*device_, staging_buffer.allocation)};
        sphere* const spheres{world_map.as<sphere>()};

        spheres[0] = {.center = {0.0f, 0.0f, -1.0f}, .radius = 0.5f};
        spheres[1] = {.center = {0.0f, -100.5f, -1.0f}, .radius = 100.0f};

        unmap_memory(*device_, &world_map);
    }

    world_buffer_ = create_buffer(*device_,
        staging_buffer.size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    renderer_->transfer_buffer(staging_buffer, world_buffer_);

    destroy(device_, &staging_buffer);
}
