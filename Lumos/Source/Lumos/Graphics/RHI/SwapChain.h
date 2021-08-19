#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class Texture;
        class Framebuffer;
        class RenderPass;
        class CommandBuffer;

        class SwapChain
        {
        public:
            virtual ~SwapChain() = default;
            static SwapChain* Create(uint32_t width, uint32_t height);

            virtual bool Init(bool vsync) = 0;
            virtual Texture* GetCurrentImage() = 0;
            virtual Texture* GetImage(uint32_t index) = 0;
            virtual uint32_t GetCurrentBufferIndex() const = 0;
            virtual size_t GetSwapChainBufferCount() const = 0;
            virtual CommandBuffer* GetCurrentCommandBuffer() { return nullptr; }

        protected:
            static SwapChain* (*CreateFunc)(uint32_t, uint32_t);
        };
    }
}
