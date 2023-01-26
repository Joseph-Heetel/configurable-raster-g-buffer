#pragma once
#include <foray_api.hpp>

namespace cgbuffer {
    class CGBuffer : public foray::stages::RasterizedRenderStage
    {
      public:
        inline static constexpr uint32_t MAX_OUTPUT_COUNT = 16;

        enum class FragmentInputFlagBits : uint32_t
        {
            WORLDPOS     = 0x001,
            WORLDPOSOLD  = 0x002,
            DEVICEPOS    = 0x004,
            DEVICEPOSOLD = 0x008,
            NORMAL       = 0x010,
            TANGENT      = 0x020,
            UV           = 0x040,
            MESHID       = 0x080,
            MAXENUM      = 0x100,
        };

        static std::string ToString(FragmentInputFlagBits input);

        enum class BuiltInFeaturesFlagBits : uint32_t
        {
            MATERIALPROBE      = 0x01,
            MATERIALPROBEALPHA = 0x02,
            ALPHATEST          = 0x04,
            NORMALMAPPING      = 0x08,
            MAXENUM            = 0x10,
        };

        static std::string ToString(BuiltInFeaturesFlagBits feature);

        enum class FragmentOutputType
        {
            FLOAT,
            INT,
            UINT,
            VEC2,
            VEC3,
            VEC4,
            IVEC2,
            IVEC3,
            IVEC4,
            UVEC2,
            UVEC3,
            UVEC4,
        };

        static std::string ToString(FragmentOutputType type);

        struct OutputRecipe
        {
            uint32_t           FragmentInputFlags   = 0;
            uint32_t           BuiltInFeaturesFlags = 0;
            FragmentOutputType Type                 = FragmentOutputType::FLOAT;
            VkFormat           ImageFormat          = VkFormat::VK_FORMAT_UNDEFINED;
            VkClearColorValue  ClearValue           = {};
            std::string        Result               = "0";
            std::string        Calculation          = "";

            OutputRecipe& AddFragmentInput(FragmentInputFlagBits input);
            OutputRecipe& EnableBuiltInFeature(BuiltInFeaturesFlagBits feature);
        };

        CGBuffer& EnableBuiltInFeature(BuiltInFeaturesFlagBits feature);

        CGBuffer&           AddOutput(std::string_view name, const OutputRecipe& recipe);
        const OutputRecipe& GetOutputRecipe(std::string_view name) const;

        virtual void Build(foray::core::Context* context, foray::scene::Scene* scene);

        virtual void RecordFrame(VkCommandBuffer cmdBuffer, foray::base::FrameRenderInfo& renderInfo) override;

        virtual void Resize(const VkExtent2D& extent) override;

        virtual void Destroy() override;

      protected:
        struct Output
        {
            std::string               Name;
            foray::core::ManagedImage Image;
            OutputRecipe              Recipe;

            inline Output(std::string_view name, const OutputRecipe& recipe) : Name(name), Recipe(recipe) {}
            VkAttachmentDescription GetAttachmentDescr() const;
        };
        using OutputMap  = std::unordered_map<std::string, std::unique_ptr<Output>>;
        using OutputList = std::vector<Output*>;

        OutputMap                 mOutputMap;
        OutputList                mOutputList;
        foray::core::ManagedImage mDepthImage;
        foray::scene::Scene*      mScene = nullptr;

        uint32_t mBuiltInFeaturesFlagsGlobal = 0;
        uint32_t mInterfaceFlagsGlobal = 0;

        foray::core::ShaderModule mVertexShaderModule;
        foray::core::ShaderModule mFragmentShaderModule;

        void         CreateOutputs(const VkExtent2D& size);
        void         CreateRenderPass();
        void         CreateFrameBuffer();
        virtual void SetupDescriptors() override;
        virtual void CreateDescriptorSets() override;
        virtual void CreatePipelineLayout() override;
        void         CreatePipeline();
    };
}  // namespace cgbuffer