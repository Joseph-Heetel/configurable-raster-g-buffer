#include "conf-gbuffer.hpp"

namespace cgbuffer {
    class GBufferTestApp : public foray::base::DefaultAppBase
    {
      protected:
        virtual void ApiBeforeInit() override;
        virtual void ApiBeforeDeviceSelection(vkb::PhysicalDeviceSelector& pds) override;
        virtual void ApiBeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder) override;
        virtual void ApiInit() override;
        virtual void InitCreateVma() override;
        virtual void ApiRender(foray::base::FrameRenderInfo& renderInfo) override;
        virtual void ApiDestroy() override;

        CGBuffer mGBufferStage;
        struct
        {
            VkPhysicalDeviceBufferDeviceAddressFeatures   BufferDeviceAdressFeatures = {};
            VkPhysicalDeviceDescriptorIndexingFeaturesEXT DescriptorIndexingFeatures = {};
            VkPhysicalDeviceSynchronization2Features      Sync2FEatures              = {};
        } mDeviceFeatures = {};
        std::unique_ptr<foray::scene::Scene> mScene;
    };

    void GBufferTestApp::ApiBeforeInit()
    {
        mDevice.SetEnableDefaultDeviceFeatures(false);
        mDevice.SetSetDefaultCapabilitiesToDeviceSelector(false);
    }
    void GBufferTestApp::InitCreateVma()
    {
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_2;
        allocatorCreateInfo.physicalDevice         = mDevice;
        allocatorCreateInfo.device                 = mDevice;
        allocatorCreateInfo.instance               = mInstance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        allocatorCreateInfo.flags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

        vmaCreateAllocator(&allocatorCreateInfo, &mContext.Allocator);
    }
    void GBufferTestApp::ApiBeforeDeviceSelection(vkb::PhysicalDeviceSelector& deviceSelector)
    {
        // Require capability to present to the current windows surface
        deviceSelector.require_present();

        // Prefer dedicated devices
        deviceSelector.prefer_gpu_device_type();

        deviceSelector.set_minimum_version(1U, 3U);

        // Set raytracing extensions
        std::vector<const char*> requiredExtensions{VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_KHR_SPIRV_1_4_EXTENSION_NAME,
                                                    VK_KHR_RELAXED_BLOCK_LAYOUT_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME};
        deviceSelector.add_required_extensions(requiredExtensions);

        // Enable samplerAnisotropy
        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        deviceSelector.set_required_features(deviceFeatures);
    }
    void GBufferTestApp::ApiBeforeDeviceBuilding(vkb::DeviceBuilder& deviceBuilder)
    {
        mDeviceFeatures.BufferDeviceAdressFeatures = {.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, .bufferDeviceAddress = VK_TRUE};

        mDeviceFeatures.DescriptorIndexingFeatures = {.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
                                                      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
                                                      .runtimeDescriptorArray                    = VK_TRUE};  // enable this for unbound descriptor arrays

        mDeviceFeatures.Sync2FEatures = {.sType = VkStructureType::VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES, .synchronization2 = VK_TRUE};

        deviceBuilder.add_pNext(&mDeviceFeatures.BufferDeviceAdressFeatures);
        deviceBuilder.add_pNext(&mDeviceFeatures.DescriptorIndexingFeatures);
        deviceBuilder.add_pNext(&mDeviceFeatures.Sync2FEatures);
    }
    void GBufferTestApp::ApiInit()
    {
        mScene = std::make_unique<foray::scene::Scene>(&mContext);

        foray::gltf::ModelConverter converter(mScene.get());
        converter.LoadGltfModel(SCENE_DIR);
    }
    void GBufferTestApp::ApiRender(foray::base::FrameRenderInfo& renderInfo)
    {
        foray::core::DeviceSyncCommandBuffer& cb = renderInfo.GetPrimaryCommandBuffer();
        cb.Begin();
        renderInfo.ClearSwapchainImage(cb);
        renderInfo.PrepareSwapchainImageForPresent(cb);
        cb.End();
        cb.Submit();
    }
    void GBufferTestApp::ApiDestroy() 
    {
        mScene = nullptr;
        mGBufferStage.Destroy();
    }
}  // namespace cgbuffer

int main(int argc, char** argv)
{
    foray::osi::OverrideCurrentWorkingDirectory(CWD_OVERRIDE);
    cgbuffer::GBufferTestApp testApp;
    return testApp.Run();
}