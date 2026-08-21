#pragma once


#include "render/api/graphics_device.hpp"
#include "render/api/buffer.hpp"
#include "render/api/vertex_array.hpp"
#include "render/vulkan/vk_pipeline.hpp"
#include <vulkan/vulkan.hpp>
#include <optional>

namespace mam {
  class VKWindow;
  class GraphicContext;
  class VKPipeline;

  class VKCubemap : public Texture {
  public:
    explicit VKCubemap(const std::array<std::string, 6>& faces) = delete;
    ~VKCubemap() override = default;

    void bind(u32 slot = 0) const override = 0;
    void unBind(u32 slot = 0) const override = 0;

    // No aplican a un cubemap estatico de skybox:
    void setData(const void*, u32, u32, u32) override {}
    void setData(const void*, u32) override {}
    void resetTexture(const std::string&, bool = true) override {}
    void setMinFilter(Filter) override {}
    void setMagFilter(Filter) override {}
    void setWrapS(Wrap) override {}
    void setWrapT(Wrap) override {}
    void setWrapR(Wrap) override {}
    void setWrap(Wrap) override {}
    void generateMipmaps() override {}
  };


}