#include "instance.hpp"
#include <gtest/gtest.h>

using namespace Vulkan;

class InstanceFixture : public ::testing::Test {
  protected:
    Instance instance{
        "TestApp", std::array<const char* const, 2>{vk::KHRSurfaceExtensionName, vk::EXTHeadlessSurfaceExtensionName}};
};

TEST_F(InstanceFixture, GetReturnsNonNull) { ASSERT_NE(*instance, nullptr); }