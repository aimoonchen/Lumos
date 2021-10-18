#include "Precompiled.h"
#include "GridRenderer.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/Framebuffer.h"
#include "Graphics/RHI/Texture.h"
#include "Graphics/RHI/UniformBuffer.h"
#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/CommandBuffer.h"
#include "Graphics/RHI/SwapChain.h"
#include "Graphics/RHI/RenderPass.h"
#include "Graphics/RHI/Pipeline.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/GBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/MeshFactory.h"
#include "Scene/Scene.h"
#include "Core/Application.h"
#include "Graphics/Camera/Camera.h"
#include "Maths/Transform.h"
#include "Graphics/Renderers/RenderGraph.h"

#include "CompiledSPV/Headers/Gridvertspv.hpp"
#include "CompiledSPV/Headers/Gridfragspv.hpp"

#include <imgui/imgui.h>

namespace Lumos
{
    namespace Graphics
    {
        GridRenderer::GridRenderer(uint32_t width, uint32_t height)
        {
            m_Pipeline = nullptr;

            IRenderer::SetScreenBufferSize(width, height);
            GridRenderer::Init();

            m_GridRes = 1.4f;
            m_GridSize = 500.0f;
        }

        GridRenderer::~GridRenderer()
        {
            delete m_Quad;
        }

        void GridRenderer::RenderScene()
        {
            LUMOS_PROFILE_FUNCTION();
            m_CurrentBufferID = 0;
            if(!m_RenderTexture)
                m_CurrentBufferID = Renderer::GetMainSwapChain()->GetCurrentBufferIndex();

            CreateGraphicsPipeline();

            m_Pipeline->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

            m_CurrentDescriptorSets[0] = m_DescriptorSet[0].get();

            m_Quad->GetVertexBuffer()->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), m_Pipeline.get());
            m_Quad->GetIndexBuffer()->Bind(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

            Renderer::BindDescriptorSets(m_Pipeline.get(), Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), 0, m_CurrentDescriptorSets.data(), 1);
            Renderer::DrawIndexed(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer(), DrawType::TRIANGLE, m_Quad->GetIndexBuffer()->GetCount());

            m_Quad->GetVertexBuffer()->Unbind();
            m_Quad->GetIndexBuffer()->Unbind();

            End();

            m_Pipeline->End(Renderer::GetMainSwapChain()->GetCurrentCommandBuffer());

            //if(!m_RenderTexture)
            //Renderer::Present((m_CommandBuffers[Renderer::GetMainSwapChain()->GetCurrentBufferIndex()].get()));
        }

        enum VSSystemUniformIndices : int32_t
        {
            VSSystemUniformIndex_InverseProjectionViewMatrix = 0,
            VSSystemUniformIndex_Size
        };

        void GridRenderer::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            //m_Shader = Application::Get().GetShaderLibrary()->GetResource("//CoreShaders/Grid.shader");
            m_Shader = Graphics::Shader::CreateFromEmbeddedArray(spirv_Gridvertspv.data(), spirv_Gridvertspv_size, spirv_Gridfragspv.data(), spirv_Gridfragspv_size);

            m_Quad = Graphics::CreatePlane(5000.0f, 5000.f, Maths::Vector3(0.0f, 1.0f, 0.0f));

            Graphics::DescriptorDesc descriptorDesc {};
            descriptorDesc.layoutIndex = 0;
            descriptorDesc.shader = m_Shader.get();
            m_DescriptorSet.resize(1);
            m_DescriptorSet[0] = SharedPtr<Graphics::DescriptorSet>(Graphics::DescriptorSet::Create(descriptorDesc));
            UpdateUniformBuffer();

            m_CurrentDescriptorSets.resize(1);
        }

        void GridRenderer::Begin()
        {
            LUMOS_PROFILE_FUNCTION();
        }

        void GridRenderer::BeginScene(Scene* scene, Camera* overrideCamera, Maths::Transform* overrideCameraTransform)
        {
            LUMOS_PROFILE_FUNCTION();
            auto& registry = scene->GetRegistry();

            if(overrideCamera)
            {
                m_Camera = overrideCamera;
                m_CameraTransform = overrideCameraTransform;
            }
            else
            {
                auto cameraView = registry.view<Camera>();
                if(!cameraView.empty())
                {
                    m_Camera = &cameraView.get<Camera>(cameraView.front());
                    m_CameraTransform = registry.try_get<Maths::Transform>(cameraView.front());
                }
            }

            if(!m_Camera || !m_CameraTransform)
                return;

            auto proj = m_Camera->GetProjectionMatrix();

            UniformBufferObjectFrag test;
            test.res = m_GridRes;
            test.scale = m_GridSize;
            test.cameraPos = m_CameraTransform->GetWorldPosition();
            test.maxDistance = m_MaxDistance;

            auto invViewProj = proj * m_CameraTransform->GetWorldMatrix().Inverse();
            m_DescriptorSet[0]->SetUniform("UniformBufferObject", "u_MVP", &invViewProj);
            m_DescriptorSet[0]->SetUniformBufferData("UniformBuffer", &test);
            m_DescriptorSet[0]->Update();
        }

        void GridRenderer::End()
        {
            LUMOS_PROFILE_FUNCTION();
        }

        void GridRenderer::OnImGui()
        {
            LUMOS_PROFILE_FUNCTION();
            ImGui::TextUnformatted("Grid Renderer");

            if(ImGui::TreeNode("Parameters"))
            {
                ImGui::DragFloat("Resolution", &m_GridRes, 1.0f, 0.0f, 10.0f);
                ImGui::DragFloat("Scale", &m_GridSize, 1.0f, 1.0f, 10000.0f);
                ImGui::DragFloat("Max Distance", &m_MaxDistance, 1.0f, 1.0f, 10000.0f);

                ImGui::TreePop();
            }
        }

        void GridRenderer::OnResize(uint32_t width, uint32_t height)
        {
            LUMOS_PROFILE_FUNCTION();
            SetScreenBufferSize(width, height);

            UpdateUniformBuffer();
        }

        void GridRenderer::CreateGraphicsPipeline()
        {
            LUMOS_PROFILE_FUNCTION();

            Graphics::PipelineDesc pipelineDesc;
            pipelineDesc.shader = m_Shader;

            pipelineDesc.polygonMode = Graphics::PolygonMode::FILL;
            pipelineDesc.cullMode = Graphics::CullMode::NONE;
            pipelineDesc.transparencyEnabled = true;
            pipelineDesc.blendMode = BlendMode::SrcAlphaOneMinusSrcAlpha;

            {
                pipelineDesc.depthTarget = reinterpret_cast<Texture*>(Application::Get().GetRenderGraph()->GetDepthTexture());
            }

            if(m_RenderTexture)
                pipelineDesc.colourTargets[0] = m_RenderTexture;
            else
                pipelineDesc.swapchainTarget = true;

            m_Pipeline = Graphics::Pipeline::Get(pipelineDesc);
        }

        void GridRenderer::UpdateUniformBuffer()
        {
            LUMOS_PROFILE_FUNCTION();
        }

        void GridRenderer::SetRenderTarget(Texture* texture, bool rebuildFramebuffer)
        {
            LUMOS_PROFILE_FUNCTION();
            m_RenderTexture = texture;

            if(!rebuildFramebuffer)
                return;
        }

        void GridRenderer::CreateFramebuffers()
        {
            LUMOS_PROFILE_FUNCTION();
        }
    }
}
