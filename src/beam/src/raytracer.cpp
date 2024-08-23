#include <raytracer.hpp>

#include <perspective_camera.hpp>
#include <scene.hpp>
#include <sphere.hpp>

#include <cppext_numeric.hpp>
#include <cppext_pragma_warning.hpp>

#include <vulkan_buffer.hpp>
#include <vulkan_descriptors.hpp>
#include <vulkan_device.hpp>
#include <vulkan_image.hpp>
#include <vulkan_memory.hpp>
#include <vulkan_pipeline.hpp>
#include <vulkan_renderer.hpp>
#include <vulkan_utility.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <imgui.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <vector>

// IWYU pragma: no_include <filesystem>

namespace
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    std::default_random_engine rng{std::random_device{}()};
    std::uniform_int_distribution<uint32_t> frame_dist{};

    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    struct [[nodiscard]] push_constants
    {
        glm::vec3 camera_position;
        uint32_t world_count;
        glm::vec3 camera_front;
        uint32_t material_count;
        glm::vec3 camera_up;
        uint32_t samples_per_pixel;
        uint32_t max_depth;
        float defocus_angle;
        float focus_distance;
        float fovy;
        uint32_t total_samples;
        uint32_t frame_seed;
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

        VkDescriptorSetLayoutBinding material_buffer_binding{};
        material_buffer_binding.binding = 2;
        material_buffer_binding.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        material_buffer_binding.descriptorCount = 1;
        material_buffer_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        std::array const bindings{target_image_binding,
            world_buffer_binding,
            material_buffer_binding};

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
        VkDescriptorBufferInfo const world_buffer_info,
        VkDescriptorBufferInfo const material_buffer_info)
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

        VkWriteDescriptorSet material_buffer_write{};
        material_buffer_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        material_buffer_write.dstSet = descriptor_set;
        material_buffer_write.dstBinding = 2;
        material_buffer_write.dstArrayElement = 0;
        material_buffer_write.descriptorType =
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        material_buffer_write.descriptorCount = 1;
        material_buffer_write.pBufferInfo = &material_buffer_info;

        std::array const descriptor_writes{target_image_write,
            world_buffer_write,
            material_buffer_write};

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
    fill_world_and_materials();

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

    DISABLE_WARNING_PUSH
    DISABLE_WARNING_MISSING_FIELD_INITIALIZERS
    bind_descriptor_set(device_,
        descriptor_set_,
        VkDescriptorImageInfo{.imageView = scene_->color_image().view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
        VkDescriptorBufferInfo{.buffer = world_buffer_.buffer,
            .offset = 0,
            .range = world_buffer_.size},
        VkDescriptorBufferInfo{.buffer = material_buffer_.buffer,
            .offset = 0,
            .range = material_buffer_.size});
    DISABLE_WARNING_POP
}

beam::raytracer::~raytracer()
{
    destroy(device_, &material_buffer_);
    destroy(device_, &world_buffer_);

    destroy(device_, compute_pipeline_.get());

    vkDestroyDescriptorSetLayout(device_->logical, descriptor_layout_, nullptr);
}

void beam::raytracer::update(perspective_camera const& camera)
{
    camera_position_ = camera.position();
    camera_front_ = camera.front_direction();
    camera_up_ = camera.up_direction();
    total_samples_ = 0;
}

void beam::raytracer::draw(VkCommandBuffer command_buffer)
{
    auto const& target_extent{scene_->color_image().extent};

    push_constants const pc{.camera_position = camera_position_,
        .world_count = sphere_count_,
        .camera_front = camera_position_ + camera_front_,
        .material_count = material_count_,
        .camera_up = camera_up_,
        .samples_per_pixel = cppext::narrow<uint32_t>(samples_per_pixel_),
        .max_depth = cppext::narrow<uint32_t>(max_depth_),
        .defocus_angle = defocus_angle_,
        .focus_distance = focus_distance_,
        .fovy = fovy_,
        .total_samples = total_samples_,
        .frame_seed = frame_dist(rng)};

    vkCmdPushConstants(command_buffer,
        *compute_pipeline_->layout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(push_constants),
        &pc);

    vkrndr::bind_pipeline(command_buffer,
        *compute_pipeline_,
        0,
        std::span{&descriptor_set_, 1});

    vkCmdDispatch(command_buffer,
        static_cast<uint32_t>(
            std::ceil(cppext::as_fp(target_extent.width) / 16.0f)),
        static_cast<uint32_t>(
            std::ceil(cppext::as_fp(target_extent.height) / 16.0f)),
        1);

    total_samples_ += cppext::narrow<uint32_t>(samples_per_pixel_);
}

void beam::raytracer::on_resize()
{
    DISABLE_WARNING_PUSH
    DISABLE_WARNING_MISSING_FIELD_INITIALIZERS
    bind_descriptor_set(device_,
        descriptor_set_,
        VkDescriptorImageInfo{.imageView = scene_->color_image().view,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL},
        VkDescriptorBufferInfo{.buffer = world_buffer_.buffer,
            .offset = 0,
            .range = world_buffer_.size},
        VkDescriptorBufferInfo{.buffer = material_buffer_.buffer,
            .offset = 0,
            .range = material_buffer_.size});
    DISABLE_WARNING_POP
}

void beam::raytracer::draw_imgui()
{
    bool reset{};

    ImGui::Begin("Raytracer");
    ImGui::SliderInt("Samples per pixel", &samples_per_pixel_, 1, 5);
    reset |= ImGui::SliderInt("Max depth", &max_depth_, 1, 10);
    reset |= ImGui::SliderFloat("Focus distance", &focus_distance_, 0, 100);
    reset |= ImGui::SliderFloat("Defocus angle", &defocus_angle_, -1, 10);
    reset |= ImGui::SliderFloat("FOV Y", &fovy_, 0, 120);
    ImGui::End();

    if (reset)
    {
        total_samples_ = 0;
    }
}

void beam::raytracer::fill_world(std::span<sphere const> spheres)
{
    vkrndr::vulkan_buffer staging_buffer{vkrndr::create_buffer(*device_,
        spheres.size() * sizeof(sphere),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    {
        auto world_map{vkrndr::map_memory(*device_, staging_buffer)};
        sphere* const sph{world_map.as<sphere>()};
        std::ranges::copy(spheres, sph);
        unmap_memory(*device_, &world_map);
    }

    world_buffer_ = create_buffer(*device_,
        staging_buffer.size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    sphere_count_ = cppext::narrow<uint32_t>(spheres.size());

    renderer_->transfer_buffer(staging_buffer, world_buffer_);

    destroy(device_, &staging_buffer);
}

void beam::raytracer::fill_materials(std::span<material const> materials)
{
    vkrndr::vulkan_buffer staging_buffer{vkrndr::create_buffer(*device_,
        materials.size() * sizeof(material),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)};

    {
        auto world_map{vkrndr::map_memory(*device_, staging_buffer)};
        material* const mat{world_map.as<material>()};
        std::ranges::copy(materials, mat);
        unmap_memory(*device_, &world_map);
    }

    material_buffer_ = create_buffer(*device_,
        staging_buffer.size,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    material_count_ = cppext::narrow<uint32_t>(materials.size());

    renderer_->transfer_buffer(staging_buffer, material_buffer_);

    destroy(device_, &staging_buffer);
}

void beam::raytracer::fill_world_and_materials()
{
    static constexpr uint32_t lambertian{0};
    static constexpr uint32_t metal{1};
    static constexpr uint32_t dielectric{2};

    std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    std::uniform_real_distribution<float> lower_dist{0.0f, 0.5f};
    std::uniform_real_distribution<float> upper_dist{0.5f, 1.0f};

    std::vector<sphere> spheres;
    std::vector<material> materials;
    materials.emplace_back(glm::vec3{0.5f, 0.5f, 0.5f}, 0.0f, lambertian);
    spheres.emplace_back(glm::vec3{0.0f, -1000.0f, 0.0f},
        1000.0f,
        cppext::narrow<uint32_t>(materials.size() - 1));

    auto gen_color = [&]()
    { return glm::vec3{dist(rng), dist(rng), dist(rng)}; };

    for (int a = -11; a < 11; a++)
    {
        for (int b = -11; b < 11; b++)
        {
            auto const choose_mat{dist(rng)};
            glm::vec3 const center{cppext::as_fp(a) + 0.9f * dist(rng),
                0.2f,
                cppext::as_fp(b) + 0.9f * dist(rng)};

            if (glm::length(center - glm::vec3{4.0f, 0.2f, 0.0f}) > 0.9f)
            {
                if (choose_mat < 0.8f)
                {
                    // diffuse
                    glm::vec3 const albedo{gen_color() * gen_color()};
                    materials.emplace_back(albedo, 0.0f, lambertian);
                }
                else if (choose_mat < 0.95f)
                {
                    // metal
                    glm::vec3 const albedo{upper_dist(rng),
                        upper_dist(rng),
                        upper_dist(rng)};
                    float const fuzz{lower_dist(rng)};
                    materials.emplace_back(albedo, fuzz, metal);
                }
                else
                {
                    materials.emplace_back(glm::vec3{}, 1.5f, dielectric);
                }

                spheres.emplace_back(center,
                    0.2f,
                    cppext::narrow<uint32_t>(materials.size() - 1));
            }
        }
    }

    materials.emplace_back(glm::vec3{}, 1.5f, dielectric);
    spheres.emplace_back(glm::vec3{0.0f, 1.0f, 0.0f},
        1.0f,
        cppext::narrow<uint32_t>(materials.size() - 1));

    materials.emplace_back(glm::vec3{0.4f, 0.2f, 0.1f}, 0.0f, lambertian);
    spheres.emplace_back(glm::vec3{-4.0f, 1.0f, 0.0f},
        1.0f,
        cppext::narrow<uint32_t>(materials.size() - 1));

    materials.emplace_back(glm::vec3{0.7f, 0.6f, 0.5f}, 0.0f, metal);
    spheres.emplace_back(glm::vec3{4.0f, 1.0f, 0.0f},
        1.0f,
        cppext::narrow<uint32_t>(materials.size() - 1));

    fill_materials(materials);
    fill_world(spheres);
}
