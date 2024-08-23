#ifndef BEAM_SPHERE_INCLUDED
#define BEAM_SPHERE_INCLUDED

#include <cppext_pragma_warning.hpp>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

namespace beam
{
    DISABLE_WARNING_PUSH

    DISABLE_WARNING_STRUCTURE_WAS_PADDED_DUE_TO_ALIGNMENT_SPECIFIER
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

    DISABLE_WARNING_POP

} // namespace beam
#endif
