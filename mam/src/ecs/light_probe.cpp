#include "ecs/light_probe.hpp"

#include <glm/glm.hpp>

namespace mam {

  glm::vec3 SHCoefficients::evaluate(const glm::vec3& n) const {
    const float x = n.x, y = n.y, z = n.z;

    const float c0 = 0.282095f * 3.141593f;   // l=0
    const float c1 = 0.488603f * 2.094395f;   // l=1
    const float c2 = 1.092548f * 0.785398f;   // l=2 (cross terms)
    const float c3 = 0.315392f * 0.785398f;   // l=2 (Y6)
    const float c4 = 0.546274f * 0.785398f;   // l=2 (Y8)

    glm::vec3 irradiance(0.f);

    irradiance += coeffs[0] * c0;

    irradiance += coeffs[1] * (c1 * y);
    irradiance += coeffs[2] * (c1 * z);
    irradiance += coeffs[3] * (c1 * x);

    irradiance += coeffs[4] * (c2 * x * y);
    irradiance += coeffs[5] * (c2 * y * z);
    irradiance += coeffs[6] * (c3 * (3.f * z * z - 1.f));
    irradiance += coeffs[7] * (c2 * x * z);
    irradiance += coeffs[8] * (c4 * (x * x - y * y));

    return glm::max(irradiance, glm::vec3(0.f));
  }

} // namespace mam
