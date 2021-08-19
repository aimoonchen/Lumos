#pragma once
#include "Graphics/RHI/GraphicsContext.h"

#include "VK.h"
#include "Core/Reference.h"

#include "VKDevice.h"

#include <deque>

#ifdef USE_VMA_ALLOCATOR
#include <vulkan/vk_mem_alloc.h>
#endif

#ifdef LUMOS_DEBUG
const bool EnableValidationLayers = true;
#else
const bool EnableValidationLayers = false;
#endif

namespace Lumos
{
    namespace Graphics
    {
        class VKCommandPool;
        class VKSwapChain;

        class VKContext : public GraphicsContext
        {
        public:
            VKContext(const WindowDesc& properties, Window* window);
            ~VKContext();

            void Init() override;
            void Present() override;
            void Unload();

            static VKContext* Get() { return static_cast<VKContext*>(s_Context); }

            static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugReportFlagsEXT flags,
                VkDebugReportObjectTypeEXT objType,
                uint64_t sourceObj,
                size_t location,
                int32_t msgCode,
                const char* pLayerPrefix,
                const char* pMsg,
                void* userData);

            VkInstance GetVKInstance() const { return m_VkInstance; }
            void* GetWindowContext() const { return m_Window->GetHandle(); }

            size_t GetMinUniformBufferOffsetAlignment() const override;

            bool FlipImGUITexture() const override { return false; }
            void WaitIdle() const override;
            void OnResize(uint32_t width, uint32_t height);
            void OnImGui() override;

            float GetGPUMemoryUsed() override;
            float GetTotalGPUMemory() override;

            const std::vector<const char*>& GetLayerNames() const { return m_InstanceLayerNames; }
            const std::vector<const char*>& GetExtensionNames() const { return m_InstanceExtensionNames; }

            const SharedPtr<Lumos::Graphics::VKSwapChain>& GetSwapChain() const { return m_Swapchain; }

            static void MakeDefault();

            struct DeletionQueue
            {
                std::deque<std::function<void()>> m_Deletors;

                template <typename F>
                void PushFunction(F&& function)
                {
                    LUMOS_ASSERT(sizeof(F) < 200, "Lambda too large");
                    m_Deletors.push_back(function);
                }

                void Flush()
                {
                    for(auto it = m_Deletors.rbegin(); it != m_Deletors.rend(); it++)
                    {
                        (*it)();
                    }

                    m_Deletors.clear();
                }
            };

        protected:
            static GraphicsContext* CreateFuncVulkan(const WindowDesc&, Window*);

            void CreateInstance();
            void SetupDebugCallback();
            bool CheckValidationLayerSupport(const std::vector<const char*>& validationLayers);
            bool CheckExtensionSupport(std::vector<const char*>& extensions);

#ifdef USE_VMA_ALLOCATOR
            void DebugDrawVmaMemory(VmaStatInfo& info, bool indent = true);
#endif

            static const std::vector<const char*> GetRequiredExtensions();
            const std::vector<const char*> GetRequiredLayers() const;

        private:
            VkInstance m_VkInstance;
            VkDebugReportCallbackEXT m_DebugCallback {};

            std::vector<VkLayerProperties> m_InstanceLayers;
            std::vector<VkExtensionProperties> m_InstanceExtensions;

            std::vector<const char*> m_InstanceLayerNames;
            std::vector<const char*> m_InstanceExtensionNames;

            SharedPtr<Lumos::Graphics::VKSwapChain> m_Swapchain;

            uint32_t m_Width, m_Height;
            bool m_VSync;

            Window* m_Window;

            bool m_StandardValidationLayer = false;
            bool m_ValidationLayer = false;
            bool m_RenderDocLayer = false;
            bool m_AssistanceLayer = false;
        };
    }
}
