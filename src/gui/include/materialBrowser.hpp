#pragma once

#include "material.hpp"
#include "resourceManager.hpp"
#include <vector>

namespace Render {

class MaterialBrowser {
  public:
    explicit MaterialBrowser(const ResourceManager& resourceManager);

    void update();

  private:
    const ResourceManager* resources;

    std::vector<bool> materialSettingsView;

    static constexpr std::array<const char*, 8> depthCompareNames = {
        "Never", "Less", "Equal", "LessOrEqual", "Greater", "NotEqual", "GreaterOrEqual", "Always"};

    static constexpr std::array<const char*, 4> blendModeNames = {"Opaque", "Alpha", "Premultiplied", "Additive"};

    static constexpr std::array<const char*, 3> vertexInputModeNames = {"None", "Mesh", "Instanced"};

    static constexpr std::array<const char*, 4> cullModeNames = {"None", "Front", "Back", "FrontAndBack"};
};

} // namespace Render
