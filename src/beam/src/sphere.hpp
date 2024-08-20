#ifndef BEAM_SPHERE_INCLUDED
#define BEAM_SPHERE_INCLUDED

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace beam
{
    struct [[nodiscard]] sphere final
    {
        glm::vec3 center;
        float radius;
        glm::uvec4 material;
    };

    struct [[nodiscard]] material final
    {
        glm::vec3 color;
        float value;
    };

} // namespace beam
#endif
