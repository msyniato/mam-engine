#pragma once

#include "common/common.hpp"
#include "render/api/render_pass.hpp"

namespace mam {

  class GraphicsDevice;

  /**
   * @brief Ordered collection of render passes executed each frame.
   *
   * Passes are added with addPass<T>(), initialised once with init(),
   * and executed in insertion order each frame via execute().
   * Individual passes can be disabled without removing them from the graph.
   */
  class RenderGraph {
  public:
    RenderGraph()  = default;
    ~RenderGraph() = default;

    RenderGraph(const RenderGraph&)            = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;

    /**
     * @brief Add a typed render pass to the graph.
     * @tparam T    Concrete RenderPass subclass.
     * @tparam Args Constructor argument types.
     * @param name  Unique pass name used for look-up.
     * @param args  Arguments forwarded to the pass constructor.
     * @return Raw pointer to the newly created pass.
     */
    template<typename T, typename... Args>
    T* addPass(const std::string& name, Args&&... args) {
      auto pass  = std::make_unique<T>(std::forward<Args>(args)...);
      pass->name = name;
      T* ptr     = pass.get();
      passes_.push_back(std::move(pass));
      return ptr;
    }

    /**
     * @brief Initialise all passes at the given viewport size.
     * @param w Viewport width in pixels.
     * @param h Viewport height in pixels.
     */
    void init(u32 w, u32 h) {
      for (auto& p : passes_) p->init(w, h);
    }

    /**
     * @brief Execute all enabled passes in insertion order.
     * @param ctx Frame context shared across all passes.
     */
    void execute(RenderPassContext& ctx) {
      for (auto& p : passes_)
        if (p->enabled) p->execute(ctx);
    }

    /**
     * @brief Notify all passes of a viewport resize.
     * @param w New viewport width in pixels.
     * @param h New viewport height in pixels.
     */
    void resize(u32 w, u32 h) {
      for (auto& p : passes_) p->resize(w, h);
    }

    /**
     * @brief Find a pass by name.
     * @param name Pass name.
     * @return Pointer to the pass, or nullptr if not found.
     */
    RenderPass* getPass(const std::string& name) {
      for (auto& p : passes_)
        if (p->name == name) return p.get();
      return nullptr;
    }

    /**
     * @brief Find a pass by name with a static downcast.
     * @tparam T Expected concrete pass type.
     * @param name Pass name.
     * @return Typed pointer to the pass, or nullptr if not found.
     */
    template<typename T>
    T* getPassAs(const std::string& name) {
      return static_cast<T*>(getPass(name));
    }

    /// Returns all passes in insertion order.
    const std::vector<std::unique_ptr<RenderPass>>& passes() const {
      return passes_;
    }

  private:
    std::vector<std::unique_ptr<RenderPass>> passes_; ///< Ordered list of render passes.
  };

} // namespace mam
