#ifndef BEAM_SPHERE_INCLUDED
#define BEAM_SPHERE_INCLUDED

#include <glm/vec3.hpp>

namespace beam
{
    struct [[nodiscard]] sphere final
    {
        glm::vec3 center;
        float radius;
    };
} // namespace beam
#endif
