#include "Precompiled.h"
#include "Application.h"

#include "Scene/Scene.h"
#include "Engine.h"
#include "Utilities/Timer.h"

#include "Graphics/RHI/Renderer.h"
#include "Graphics/RHI/GraphicsContext.h"
#include "Graphics/Renderers/RenderGraph.h"
#include "Graphics/Camera/Camera.h"
#include "Graphics/Material.h"
#include "Graphics/Renderers/DebugRenderer.h"
#include "Graphics/Renderers/GridRenderer.h"

#include "Maths/Transform.h"

#include "Scene/EntityFactory.h"
#include "Utilities/LoadImage.h"
#include "Core/OS/Input.h"
#include "Core/OS/Window.h"
#include "Core/OS/OS.h"
#include "Core/Profiler.h"
#include "Core/VFS.h"
#include "Core/JobSystem.h"
#include "Core/StringUtilities.h"
#include "Core/OS/FileSystem.h"
#include "Scripting/Lua/LuaManager.h"
#include "ImGui/ImGuiManager.h"
#include "Events/ApplicationEvent.h"
#include "Audio/AudioManager.h"
#include "Audio/Sound.h"
#include "Physics/B2PhysicsEngine/B2PhysicsEngine.h"
#include "Physics/LumosPhysicsEngine/LumosPhysicsEngine.h"

#include "Embedded/SplashIcon.inl"

#if __has_include(<filesystem>)
#include <filesystem>
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
#endif

#include <cereal/archives/json.hpp>
#include <imgui/imgui.h>

namespace Lumos
{
    Application* Application::s_Instance = nullptr;

    Application::Application()
        : m_Frames(0)
        , m_Updates(0)
        , m_SceneViewWidth(800)
        , m_SceneViewHeight(600)
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_ASSERT(!s_Instance, "Application already exists!");
        s_Instance = this;
    }

    Application::~Application()
    {
        LUMOS_PROFILE_FUNCTION();
        ImGui::DestroyContext();
    }

    void Application::OpenProject(const std::string& filePath)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ProjectName = StringUtilities::GetFileName(filePath);
        m_ProjectName = StringUtilities::RemoveFilePathExtension(m_ProjectName);

#ifndef LUMOS_PLATFORM_IOS
        auto projectRoot = StringUtilities::GetFileLocation(filePath);
        m_ProjectRoot = projectRoot;

        m_ProjectRoot = StringUtilities::RemoveSpaces(m_ProjectRoot);
        m_ProjectName = StringUtilities::RemoveSpaces(m_ProjectName);
#endif

        m_SceneManager = CreateUniquePtr<SceneManager>();

        Deserialise();

        m_SceneManager->LoadCurrentList();
        m_SceneManager->ApplySceneSwitch();
    }

    void Application::OpenNewProject(const std::string& path)
    {
        LUMOS_PROFILE_FUNCTION();
        m_ProjectRoot = path + "NewProject/";
        m_ProjectName = "NewProject";
        m_ProjectRoot = StringUtilities::RemoveSpaces(m_ProjectRoot);

        std::filesystem::create_directory(m_ProjectRoot);

        m_SceneManager = CreateUniquePtr<SceneManager>();

        MountVFSPaths();
        //Set Default values
        RenderAPI = 1;
        Width = 1200;
        Height = 800;
        Borderless = false;
        VSync = false;
        Title = "App";
        ShowConsole = false;
        Fullscreen = false;
        m_EngineAssetPath = "/Users/jmorton/dev/Lumos/Lumos/Assets/";
        VFS::Get().Mount("CoreShaders", m_EngineAssetPath + std::string("Shaders"));

        if(!FileSystem::FolderExists(m_ProjectRoot + "Assets"))
            std::filesystem::create_directory(m_ProjectRoot + "Assets");

        if(!FileSystem::FolderExists(m_ProjectRoot + "Assets/Scripts"))
            std::filesystem::create_directory(m_ProjectRoot + "Assets/Scripts");

        if(!FileSystem::FolderExists(m_ProjectRoot + "Assets/Scenes"))
            std::filesystem::create_directory(m_ProjectRoot + "Assets/Scenes");

        if(!FileSystem::FolderExists(m_ProjectRoot + "Assets/Textures"))
            std::filesystem::create_directory(m_ProjectRoot + "Assets/Texturs");

        if(!FileSystem::FolderExists(m_ProjectRoot + "Assets/Meshes"))
            std::filesystem::create_directory(m_ProjectRoot + "Assets/Meshes");

        if(!FileSystem::FolderExists(m_ProjectRoot + "Assets/Sounds"))
            std::filesystem::create_directory(m_ProjectRoot + "Assets/Sounds");

        MountVFSPaths();

        m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
        m_SceneManager->SwitchScene(0);

        //Set Default values
        Title = "App";
        Fullscreen = false;

        //m_SceneManager->ApplySceneSwitch();

        Serialise();
    }

    void Application::MountVFSPaths()
    {
        VFS::Get().Mount("Meshes", m_ProjectRoot + std::string("Assets/Meshes"));
        VFS::Get().Mount("Textures", m_ProjectRoot + std::string("Assets/Textures"));
        VFS::Get().Mount("Sounds", m_ProjectRoot + std::string("Assets/Sounds"));
        VFS::Get().Mount("Scripts", m_ProjectRoot + std::string("Assets/Scripts"));
        VFS::Get().Mount("Scenes", m_ProjectRoot + std::string("Assets/Scenes"));
        VFS::Get().Mount("Assets", m_ProjectRoot + std::string("Assets"));
    }

    Scene* Application::GetCurrentScene() const
    {
        LUMOS_PROFILE_FUNCTION();
        return m_SceneManager->GetCurrentScene();
    }

    void Application::Init()
    {
        LUMOS_PROFILE_FUNCTION();

        m_ProjectRoot = StringUtilities::RemoveSpaces(m_ProjectRoot);
        m_ProjectName = StringUtilities::RemoveSpaces(m_ProjectName);

        m_SceneManager = CreateUniquePtr<SceneManager>();
        Deserialise();

        Engine::Get();

        m_Timer = CreateUniquePtr<Timer>();

        Graphics::GraphicsContext::SetRenderAPI(static_cast<Graphics::RenderAPI>(RenderAPI));

        WindowDesc windowDesc;
        windowDesc.Width = Width;
        windowDesc.Height = Height;
        windowDesc.RenderAPI = RenderAPI;
        windowDesc.Fullscreen = Fullscreen;
        windowDesc.Borderless = Borderless;
        windowDesc.ShowConsole = ShowConsole;
        windowDesc.Title = Title;
        windowDesc.VSync = VSync;

        // Initialise the Window
        m_Window = UniquePtr<Window>(Window::Create(windowDesc));
        if(!m_Window->HasInitialised())
            OnQuit();

        m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

        m_EditorState = EditorState::Play;

        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        Graphics::Renderer::Init();

        //Draw Splash Screeh
        {
            auto splashTexture = Graphics::Texture2D::CreateFromSource(SplashIconWidth, SplashIconHeight, (void*)SplashIcon);
            Graphics::Renderer::GetRenderer()->Begin();
            Graphics::Renderer::GetRenderer()->DrawSplashScreen(splashTexture);
            Graphics::Renderer::GetRenderer()->Present();
            //To Display the window
            m_Window->ProcessInput();
            m_Window->OnUpdate();

            delete splashTexture;
        }

        uint32_t screenWidth = m_Window->GetWidth();
        uint32_t screenHeight = m_Window->GetHeight();
        m_SystemManager = CreateUniquePtr<SystemManager>();

        System::JobSystem::Context context;

        System::JobSystem::Execute(context, [](JobDispatchArgs args)
            { Lumos::Input::Get(); });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
            {
                auto audioManager = AudioManager::Create();
                if(audioManager)
                {
                    audioManager->OnInit();
                    audioManager->SetPaused(true);
                    m_SystemManager->RegisterSystem<AudioManager>(audioManager);
                }
            });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
            {
                m_SystemManager->RegisterSystem<LumosPhysicsEngine>();
                m_SystemManager->RegisterSystem<B2PhysicsEngine>();
            });

        System::JobSystem::Execute(context, [this](JobDispatchArgs args)
            { m_SceneManager->LoadCurrentList(); });

        m_ImGuiManager = CreateUniquePtr<ImGuiManager>(false);
        m_ImGuiManager->OnInit();
        LUMOS_LOG_INFO("Initialised ImGui Manager");

        m_ShaderLibrary = CreateSharedPtr<ShaderLibrary>();
		m_ModelLibrary = CreateSharedPtr<ModelLibrary>();

        m_RenderGraph = CreateUniquePtr<Graphics::RenderGraph>(screenWidth, screenHeight);

        m_CurrentState = AppState::Running;

        Graphics::Material::InitDefaultTexture();

        //Need to disable shadows for A12 and earlier - doesn't support rendering to depth array
        //        m_RenderGraph->AddRenderer(new Graphics::ForwardRenderer(screenWidth, screenHeight));
        //m_RenderGraph->AddRenderer(new Graphics::SkyboxRenderer(screenWidth, screenHeight));
        //m_RenderGraph->AddRenderer(new Graphics::Renderer2D(screenWidth, screenHeight, false, false, true));
        m_RenderGraph->EnableDebugRenderer(true);

        System::JobSystem::Wait(context);
    }

    void Application::OnQuit()
    {
        LUMOS_PROFILE_FUNCTION();
        Serialise();
        Graphics::Material::ReleaseDefaultTexture();
        Engine::Release();
        Input::Release();

        m_ShaderLibrary.reset();
		m_ModelLibrary.reset();
        m_SceneManager.reset();
        m_RenderGraph.reset();
        m_SystemManager.reset();
        m_ImGuiManager.reset();

        Graphics::Pipeline::ClearCache();
        Graphics::RenderPass::ClearCache();
        Graphics::Framebuffer::ClearCache();
        Graphics::Renderer::Release();

        m_Window.reset();
    }

    Maths::Vector2 Application::GetWindowSize() const
    {
        if(!m_Window)
            return Maths::Vector2(0.0f, 0.0f);
        return Maths::Vector2(static_cast<float>(m_Window->GetWidth()), static_cast<float>(m_Window->GetHeight()));
    }

    float Application::GetWindowDPI() const
    {
        if(!m_Window)
            return 1.0f;

        return m_Window->GetDPIScale();
    }

    bool Application::OnFrame()
    {
        LUMOS_PROFILE_FUNCTION();
        LUMOS_PROFILE_FRAMEMARKER();

        if(m_SceneManager->GetSwitchingScene())
        {
            LUMOS_PROFILE_SCOPE("Application::SceneSwitch");
            Graphics::Renderer::GetGraphicsContext()->WaitIdle();
            m_SceneManager->ApplySceneSwitch();
            return m_CurrentState != AppState::Closing;
        }

        float now = m_Timer->GetElapsedS();
        auto& stats = Engine::Get().Statistics();
        auto& ts = Engine::GetTimeStep();

        {
            LUMOS_PROFILE_SCOPE("Application::TimeStepUpdates");
            ts.Update(now);

            ImGuiIO& io = ImGui::GetIO();
            io.DeltaTime = ts.GetSeconds();

            stats.FrameTime = ts.GetMillis();
        }

        Input::Get().ResetPressed();
        m_Window->ProcessInput();

        if(Input::Get().GetKeyPressed(Lumos::InputCode::Key::Escape))
        {
            m_CurrentState = AppState::Closing;
        }

        //Exit frame early if escape or close button clicked
        //Prevents a crash with vulkan/moltenvk
        if(m_CurrentState == AppState::Closing)
            return false;

        {
            LUMOS_PROFILE_SCOPE("Application::ImGui::NewFrame");
            ImGui::NewFrame();
        }

        {
            LUMOS_PROFILE_SCOPE("Application::Update");
            OnUpdate(ts);
            m_Updates++;
        }

        if(!m_Minimized)
        {
            LUMOS_PROFILE_SCOPE("Application::Render");

            Graphics::Renderer::GetRenderer()->Begin();

            OnRender();
            m_ImGuiManager->OnRender(m_SceneManager->GetCurrentScene());

            Graphics::Renderer::GetRenderer()->Present();

            Graphics::Pipeline::DeleteUnusedCache();
            Graphics::Framebuffer::DeleteUnusedCache();
            Graphics::RenderPass::DeleteUnusedCache();

            m_ShaderLibrary->Update(ts.GetElapsedSeconds());
			m_ModelLibrary->Update(ts.GetElapsedSeconds());
			
            m_Frames++;
        }

        {
            LUMOS_PROFILE_SCOPE("Application::UpdateGraphicsStats");
            stats.UsedGPUMemory = Graphics::Renderer::GetGraphicsContext()->GetGPUMemoryUsed();
            stats.TotalGPUMemory = Graphics::Renderer::GetGraphicsContext()->GetTotalGPUMemory();
        }

        {
            LUMOS_PROFILE_SCOPE("Application::WindowUpdate");
            m_Window->UpdateCursorImGui();
            m_Window->OnUpdate();
        }

        if(now - m_SecondTimer > 1.0f)
        {
            LUMOS_PROFILE_SCOPE("Application::FrameRateCalc");
            m_SecondTimer += 1.0f;

            stats.FramesPerSecond = m_Frames;
            stats.UpdatesPerSecond = m_Updates;

            m_Frames = 0;
            m_Updates = 0;
        }

        return m_CurrentState != AppState::Closing;
    }

    void Application::OnRender()
    {
        LUMOS_PROFILE_FUNCTION();
       // if(m_RenderGraph->GetCount() > 0)
        {
            m_RenderGraph->BeginScene(m_SceneManager->GetCurrentScene());

            m_RenderGraph->OnRender();
            OnDebugDraw();
        }
    }

    void Application::OnDebugDraw()
    {
        m_SystemManager->OnDebugDraw();
    }

    void Application::OnUpdate(const TimeStep& dt)
    {
        LUMOS_PROFILE_FUNCTION();
        if(Application::Get().GetEditorState() != EditorState::Paused
            && Application::Get().GetEditorState() != EditorState::Preview)
        {
            m_SystemManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
            LuaManager::Get().OnUpdate(m_SceneManager->GetCurrentScene());
            m_SceneManager->GetCurrentScene()->OnUpdate(dt);
        }

        if(!m_Minimized)
        {
            m_RenderGraph->OnUpdate(dt, m_SceneManager->GetCurrentScene());
        }
        m_ImGuiManager->OnUpdate(dt, m_SceneManager->GetCurrentScene());
    }

    void Application::OnEvent(Event& e)
    {
        LUMOS_PROFILE_FUNCTION();
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(Application::OnWindowResize));

        if(m_ImGuiManager)
            m_ImGuiManager->OnEvent(e);
        if(e.Handled())
            return;

        if(m_RenderGraph)
            m_RenderGraph->OnEvent(e);

        if(e.Handled())
            return;

        if(m_SceneManager->GetCurrentScene())
            m_SceneManager->GetCurrentScene()->OnEvent(e);

        Input::Get().OnEvent(e);
    }

    void Application::Run()
    {
        while(OnFrame())
        {
        }

        OnQuit();
    }

    void Application::OnNewScene(Scene* scene)
    {
        LUMOS_PROFILE_FUNCTION();
    }

    SharedPtr<ShaderLibrary>& Application::GetShaderLibrary() { return m_ShaderLibrary; }
	SharedPtr<ModelLibrary>& Application::GetModelLibrary() { return m_ModelLibrary; }
	
    void Application::OnExitScene()
    {
    }

    bool Application::OnWindowClose(WindowCloseEvent& e)
    {
        m_CurrentState = AppState::Closing;
        return true;
    }

    bool Application::OnWindowResize(WindowResizeEvent& e)
    {
        LUMOS_PROFILE_FUNCTION();
        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        int width = e.GetWidth(), height = e.GetHeight();

        if(width == 0 || height == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;

        Graphics::Renderer::GetRenderer()->OnResize(width, height);

        if(m_RenderGraph)
            m_RenderGraph->OnResize(width, height);

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        return false;
    }

    void Application::OnImGui()
    {
        LUMOS_PROFILE_FUNCTION();
        m_SceneManager->GetCurrentScene()->OnImGui();
    }

    void Application::OnSceneViewSizeUpdated(uint32_t width, uint32_t height)
    {
        LUMOS_PROFILE_FUNCTION();
        Graphics::Renderer::GetGraphicsContext()->WaitIdle();

        WindowResizeEvent e(width, height);
        if(width == 0 || height == 0)
        {
            m_Minimized = true;
            return;
        }
        m_Minimized = false;
        m_RenderGraph->OnResize(width, height);
        m_RenderGraph->OnEvent(e);

        Graphics::Renderer::GetGraphicsContext()->WaitIdle();
    }

    void Application::EmbedTexture(const std::string& texFilePath, const std::string& outPath, const std::string& arrayName)
    {
        uint32_t width, height, bits;
        bool isHDR;
        auto texture = LoadImageFromFile(texFilePath.c_str(), &width, &height, &bits, &isHDR);

        size_t psize = width * height * 4;
        std::ofstream file;
        file.open(outPath);
        file << "//Generated by Lumos using " << texFilePath << std::endl;
        file << "static const uint32_t " << arrayName << "Width = " << width << ";" << std::endl;
        file << "static const uint32_t " << arrayName << "Height = " << height << ";" << std::endl;
        file << "static const uint8_t " << arrayName << "[] = {" << (int)texture[0];
        for(size_t i = 1; i < psize; ++i)
            file << "," << (int)texture[i];
        file << "};";

        file.close();
    }

    void Application::Serialise()
    {
        LUMOS_PROFILE_FUNCTION();
        {
            std::stringstream storage;
            {
                // output finishes flushing its contents when it goes out of scope
                cereal::JSONOutputArchive output { storage };
                output(*this);
            }
            auto fullPath = m_ProjectRoot + m_ProjectName + std::string(".lmproj");
            LUMOS_LOG_INFO("Serialising Application {0}", fullPath);
            FileSystem::WriteTextFile(fullPath, storage.str());
        }
    }

    void Application::Deserialise()
    {
        LUMOS_PROFILE_FUNCTION();
        {
            auto filePath = m_ProjectRoot + m_ProjectName + std::string(".lmproj");

            MountVFSPaths();

            if(!FileSystem::FileExists(filePath))
            {
                LUMOS_LOG_INFO("No saved Project file found {0}", filePath);
                {
#ifdef LUMOS_PLATFORM_MACOS
                    OpenNewProject(StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../../");
#else
                    OpenNewProject(StringUtilities::GetFileLocation(OS::Instance()->GetExecutablePath()));
#endif
                }
                return;
            }

            std::string data = FileSystem::ReadTextFile(filePath);
            std::istringstream istr;
            istr.str(data);
            try
            {
                cereal::JSONInputArchive input(istr);
                input(*this);
            }
            catch(...)
            {
                //Set Default values
                RenderAPI = 1;
                Width = 1200;
                Height = 800;
                Borderless = false;
                VSync = false;
                Title = "App";
                ShowConsole = false;
                Fullscreen = false;
                m_EngineAssetPath = "/Users/jmorton/dev/Lumos/Lumos/Assets/";
                VFS::Get().Mount("CoreShaders", m_EngineAssetPath + std::string("Shaders"));

                m_SceneManager->EnqueueScene(new Scene("Empty Scene"));
                m_SceneManager->SwitchScene(0);

                LUMOS_LOG_ERROR("Failed to load project");
            }
        }
    }
}
