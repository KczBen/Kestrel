#pragma once

#include "resourceHandles.hpp"
#include <vulkan/vulkan.hpp>

namespace Vulkan {
class Image;
} // namespace Vulkan

namespace Render {

using RenderTarget = Vulkan::Image;
using RenderTargetHandle = ResourceHandle<RenderTarget>;

} // namespace Render
