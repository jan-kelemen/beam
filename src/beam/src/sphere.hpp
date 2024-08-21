#ifndef BEAM_SPHERE_INCLUDED
#define BEAM_SPHERE_INCLUDED

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace beam
{
    struct [[nodiscard]] alignas(32) sphere final
    {
        glm::vec3 center;
        float radius;
        uint32_t material;
    };

    struct [[nodiscard]] alignas(32) material final
    {
        glm::vec3 color;
        float value;
        uint32_t type;
    };

} // namespace beam
#endif
