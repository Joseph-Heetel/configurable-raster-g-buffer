#include "conf-gbuffer.hpp"
#include <scene/foray_geo.hpp>
#include <scene/globalcomponents/foray_cameramanager.hpp>
#include <scene/globalcomponents/foray_drawmanager.hpp>
#include <scene/globalcomponents/foray_materialmanager.hpp>
#include <scene/globalcomponents/foray_texturemanager.hpp>
#include <util/foray_pipelinebuilder.hpp>
#include <util/foray_shaderstagecreateinfos.hpp>

namespace cgbuffer {
    std::string CGBuffer::ToString(FragmentInputFlagBits input)
    {
        switch(input)
        {
            case FragmentInputFlagBits::WORLDPOS:
                return "WORLDPOS";
            case FragmentInputFlagBits::WORLDPOSOLD:
                return "WORLDPOSOLD";
            case FragmentInputFlagBits::DEVICEPOS:
                return "DEVICEPOS";
            case FragmentInputFlagBits::DEVICEPOSOLD:
                return "DEVICEPOSOLD";
            case FragmentInputFlagBits::NORMAL:
                return "NORMAL";
            case FragmentInputFlagBits::TANGENT:
                return "TANGENT";
            case FragmentInputFlagBits::UV:
                return "UV";
            case FragmentInputFlagBits::MESHID:
                return "MESHID";
            default:
                FORAY_THROWFMT("Unhandled FragmentInputFlagBits value 0x{:x}", (uint32_t)input);
        }
    }
    std::string CGBuffer::ToString(BuiltInFeaturesFlagBits feature)
    {
        switch(feature)
        {
            case BuiltInFeaturesFlagBits::MATERIALPROBE:
                return "MATERIALPROBE";
            case BuiltInFeaturesFlagBits::MATERIALPROBEALPHA:
                return "MATERIALPROBEALPHA";
            case BuiltInFeaturesFlagBits::ALPHATEST:
                return "ALPHATEST";
            case BuiltInFeaturesFlagBits::NORMALMAPPING:
                return "NORMALMAPPING";
            default:
                FORAY_THROWFMT("Unhandled BuiltInFeaturesFlagBits value 0x{:x}", (uint32_t)feature);
        }
    }
    std::string CGBuffer::ToString(FragmentOutputType type)
    {
        switch(type)
        {
            case FragmentOutputType::FLOAT:
                return "float";
            case FragmentOutputType::INT:
                return "int";
            case FragmentOutputType::UINT:
                return "uint";
            case FragmentOutputType::VEC2:
                return "vec2";
            case FragmentOutputType::VEC3:
                return "vec3";
            case FragmentOutputType::VEC4:
                return "vec4";
            case FragmentOutputType::IVEC4:
                return "ivec4";
            case FragmentOutputType::UVEC2:
                return "uvec2";
            case FragmentOutputType::UVEC3:
                return "uvec3";
            case FragmentOutputType::UVEC4:
                return "uvec4";
            default:
                FORAY_THROWFMT("Unhandled FragmentOutputType value 0x{:x}", (uint32_t)type);
        }
    }

    CGBuffer::OutputRecipe& CGBuffer::OutputRecipe::AddFragmentInput(FragmentInputFlagBits input)
    {
        FragmentInputFlags |= (uint32_t)input;
        return *this;
    }
    CGBuffer::OutputRecipe& CGBuffer::OutputRecipe::EnableBuiltInFeature(BuiltInFeaturesFlagBits feature)
    {
        BuiltInFeaturesFlags |= (uint32_t)feature;
        switch(feature)
        {
            case BuiltInFeaturesFlagBits::MATERIALPROBE:
                AddFragmentInput(FragmentInputFlagBits::UV);
                break;
            case BuiltInFeaturesFlagBits::MATERIALPROBEALPHA:
                AddFragmentInput(FragmentInputFlagBits::UV);
                break;
            case BuiltInFeaturesFlagBits::ALPHATEST:
                AddFragmentInput(FragmentInputFlagBits::UV);
                break;
            case BuiltInFeaturesFlagBits::NORMALMAPPING:
                AddFragmentInput(FragmentInputFlagBits::NORMAL);
                AddFragmentInput(FragmentInputFlagBits::TANGENT);
                break;
        }
        return *this;
    }

    void CGBuffer::AddOutput(std::string_view name, const OutputRecipe& recipe)
    {
        foray::Assert(mOutputMap.size() < MAX_OUTPUT_COUNT, fmt::format("Can not exceed maximum output count of {}", MAX_OUTPUT_COUNT));
        std::string keycopy(name);
        mOutputMap[keycopy] = std::make_unique<Output>(name, recipe);
    }
    const CGBuffer::OutputRecipe& CGBuffer::GetOutputRecipe(std::string_view name) const
    {
        std::string               keycopy(name);
        OutputMap::const_iterator iter = mOutputMap.find(keycopy);
        if(iter != mOutputMap.cend())
        {
            return iter->second->Recipe;
        }
        FORAY_THROWFMT("CGBuffer does not contain output \"{}\"!", name);
    }

    VkAttachmentDescription CGBuffer::Output::GetAttachmentDescr() const
    {
        return VkAttachmentDescription{.flags          = 0,
                                       .format         = Image.GetFormat(),
                                       .samples        = Image.GetSampleCount(),
                                       .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                       .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                       .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                       .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                       .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                       .finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    }

    void CGBuffer::Build(foray::core::Context* context, foray::scene::Scene* scene)
    {
        Destroy();
        mContext = context;
        mScene   = scene;

        CreateOutputs(mContext->GetSwapchainSize());
        CreateRenderPass();
        CreateFrameBuffer();
    }

    void CGBuffer::CreateOutputs(const VkExtent2D& size)
    {
        mOutputList.clear();
        for(auto& pair : mOutputMap)
        {
            foray::core::ManagedImage& image  = pair.second->Image;
            OutputRecipe&              recipe = pair.second->Recipe;
            std::string_view           name   = pair.second->Name;

            image.Destroy();

            foray::core::ManagedImage::CreateInfo ci(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_SAMPLED_BIT
                                                         | VkImageUsageFlagBits::VK_IMAGE_USAGE_STORAGE_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                                     recipe.ImageFormat, size, name);
            image.Create(mContext, ci);
            mOutputList.push_back(pair.second.get());
        }
        mDepthImage.Destroy();
        VkImageUsageFlags depthUsage =
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        foray::core::ManagedImage::CreateInfo ci(depthUsage, VK_FORMAT_D32_SFLOAT, size, "CGBuffer.Depth");
        ci.ImageViewCI.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;
        mDepthImage.Create(mContext, ci);
    }

    void CGBuffer::CreateRenderPass()
    {
        std::vector<VkAttachmentReference>   colorAttachmentRefs;
        std::vector<VkAttachmentDescription> attachmentDescr;

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            colorAttachmentRefs.push_back({outLocation, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
            attachmentDescr.push_back(mOutputList[outLocation]->GetAttachmentDescr());
        }

        uint32_t              depthLocation = mOutputList.size();
        VkAttachmentReference depthAttachmentRef{depthLocation, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        attachmentDescr.push_back(VkAttachmentDescription{.flags          = 0,
                                                          .format         = mDepthImage.GetFormat(),
                                                          .samples        = mDepthImage.GetSampleCount(),
                                                          .loadOp         = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                          .storeOp        = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                                          .stencilLoadOp  = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                                          .stencilStoreOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE,
                                                          .initialLayout  = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,
                                                          .finalLayout    = VkImageLayout::VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});

        // Subpass description
        VkSubpassDescription subpass    = {};
        subpass.pipelineBindPoint       = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentRefs.size();
        subpass.pColorAttachments       = colorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency subPassDependencies[2] = {};
        subPassDependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
        subPassDependencies[0].dstSubpass          = 0;
        subPassDependencies[0].srcStageMask        = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependencies[0].srcAccessMask       = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subPassDependencies[0].dependencyFlags     = VK_DEPENDENCY_BY_REGION_BIT;

        subPassDependencies[1].srcSubpass      = 0;
        subPassDependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        subPassDependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        subPassDependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        subPassDependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        subPassDependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        subPassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.pAttachments           = attachmentDescr.data();
        renderPassInfo.attachmentCount        = (uint32_t)attachmentDescr.size();
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpass;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = subPassDependencies;
        foray::AssertVkResult(vkCreateRenderPass(mContext->Device(), &renderPassInfo, nullptr, &mRenderpass));
    }
    void CGBuffer::CreateFrameBuffer()
    {
        std::vector<VkImageView> attachmentViews;

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            attachmentViews.push_back(mOutputList[outLocation]->Image.GetImageView());
        }
        attachmentViews.push_back(mDepthImage.GetImageView());

        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext                   = NULL;
        fbufCreateInfo.renderPass              = mRenderpass;
        fbufCreateInfo.pAttachments            = attachmentViews.data();
        fbufCreateInfo.attachmentCount         = (uint32_t)attachmentViews.size();
        fbufCreateInfo.width                   = mContext->GetSwapchainSize().width;
        fbufCreateInfo.height                  = mContext->GetSwapchainSize().height;
        fbufCreateInfo.layers                  = 1;
        foray::AssertVkResult(vkCreateFramebuffer(mContext->Device(), &fbufCreateInfo, nullptr, &mFrameBuffer));
    }

    void CGBuffer::SetupDescriptors()
    {
        auto materialBuffer = mScene->GetComponent<foray::scene::gcomp::MaterialManager>();
        auto textureStore   = mScene->GetComponent<foray::scene::gcomp::TextureManager>();
        auto cameraManager  = mScene->GetComponent<foray::scene::gcomp::CameraManager>();
        auto drawDirector   = mScene->GetComponent<foray::scene::gcomp::DrawDirector>();
        mDescriptorSet.SetDescriptorAt(0, materialBuffer->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
        mDescriptorSet.SetDescriptorAt(1, textureStore->GetDescriptorInfos(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        mDescriptorSet.SetDescriptorAt(2, cameraManager->GetVkDescriptorInfo(), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        mDescriptorSet.SetDescriptorAt(3, drawDirector->GetCurrentTransformsDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        mDescriptorSet.SetDescriptorAt(4, drawDirector->GetPreviousTransformsDescriptorInfo(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    }

    void CGBuffer::CreateDescriptorSets()
    {
        mDescriptorSet.Create(mContext, "CGBuffer.DescriptorSet");
    }

    void CGBuffer::CreatePipelineLayout()
    {
        mPipelineLayout.AddDescriptorSetLayout(mDescriptorSet.GetDescriptorSetLayout());
        mPipelineLayout.AddPushConstantRange<foray::scene::DrawPushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT
                                                                             | VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT);
        mPipelineLayout.Build(mContext);
    }

    void CGBuffer::CreatePipeline()
    {
        foray::core::ShaderCompilerConfig shaderConfig;
        uint32_t                          interfaceFlags = 0;
        uint32_t                          featuresFlags  = 0;

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            interfaceFlags |= mOutputList[outLocation]->Recipe.FragmentInputFlags;
            featuresFlags |= mOutputList[outLocation]->Recipe.BuiltInFeaturesFlags;
        }

        // Add interface and feature flags

        for(uint32_t flag = 1; flag < (uint32_t)FragmentInputFlagBits::MAXENUM; flag = flag << 1)
        {
            if((interfaceFlags & flag) > 0)
            {
                shaderConfig.Definitions.push_back(ToString((FragmentInputFlagBits)flag));
            }
        }

        for(uint32_t flag = 1; flag < (uint32_t)BuiltInFeaturesFlagBits::MAXENUM; flag = flag << 1)
        {
            if((featuresFlags & flag) > 0)
            {
                shaderConfig.Definitions.push_back(ToString((BuiltInFeaturesFlagBits)flag));
            }
        }

        for(uint32_t outLocation = 0; outLocation < mOutputList.size(); outLocation++)
        {
            OutputRecipe& recipe = mOutputList[outLocation]->Recipe;
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}", outLocation));
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}_TYPE", outLocation, recipe.Type));
            shaderConfig.Definitions.push_back(fmt::format("OUT_{}_RESULT", outLocation, recipe.Result));
            if(recipe.Calculation.size() > 0)
            {
                shaderConfig.Definitions.push_back(fmt::format("OUT_{}_CALC", outLocation, recipe.Calculation));
            }
        }

        mShaderKeys.push_back(mContext->ShaderMan->CompileShader("src/shaders/cgbuf.vert", mVertexShaderModule, shaderConfig));
        mShaderKeys.push_back(mContext->ShaderMan->CompileShader("src/shaders/cgbuf.frag", mFragmentShaderModule, shaderConfig));
        foray::util::ShaderStageCreateInfos shaderStageCreateInfos;
        shaderStageCreateInfos.Add(VK_SHADER_STAGE_VERTEX_BIT, mVertexShaderModule).Add(VK_SHADER_STAGE_FRAGMENT_BIT, mFragmentShaderModule);

        // vertex layout
        foray::scene::VertexInputStateBuilder vertexInputStateBuilder;
        vertexInputStateBuilder.AddVertexComponentBinding(foray::scene::EVertexComponent::Position);
        vertexInputStateBuilder.AddVertexComponentBinding(foray::scene::EVertexComponent::Normal);
        vertexInputStateBuilder.AddVertexComponentBinding(foray::scene::EVertexComponent::Tangent);
        vertexInputStateBuilder.AddVertexComponentBinding(foray::scene::EVertexComponent::Uv);
        vertexInputStateBuilder.Build();

        // clang-format off
        mPipeline = foray::util::PipelineBuilder()
            .SetContext(mContext)
            // Blend attachment states required for all color attachments
            // This is important, as color write mask will otherwise be 0x0 and you
            // won't see anything rendered to the attachment
            .SetColorAttachmentBlendCount(mOutputList.size())
            .SetPipelineLayout(mPipelineLayout.GetPipelineLayout())
            .SetVertexInputStateBuilder(&vertexInputStateBuilder)
            .SetShaderStageCreateInfos(shaderStageCreateInfos.Get())
            .SetPipelineCache(mContext->PipelineCache)
            .SetRenderPass(mRenderpass)
            .Build();
        // clang-format on
    }

    void CGBuffer::Resize(const VkExtent2D& extent)
    {
        if(!!mFrameBuffer)
        {
            vkDestroyFramebuffer(mContext->Device(), mFrameBuffer, nullptr);
            mFrameBuffer = nullptr;
        }

        for(auto& pair : mOutputMap)
        {
            foray::core::ManagedImage& image = pair.second->Image;
            if(image.Exists())
            {
                image.Resize(extent);
            }
        }
        mDepthImage.Resize(extent);

        CreateFrameBuffer();
    }

}  // namespace cgbuffer