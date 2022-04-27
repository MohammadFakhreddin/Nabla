// Copyright (C) 2018-2020 - DevSH Graphics Programming Sp. z O.O.
// This file is part of the "Nabla Engine".
// For conditions of distribution and use, see copyright notice in nabla.h

#define _NBL_STATIC_LIB_
#include <nabla.h>

#include "../common/CommonAPI.h"
#include "nbl/ext/ImGui/ImGui.h"
#include "nbl/ui/ICursorControl.h"

using namespace nbl;
using namespace asset;
using namespace video;
using namespace core;
using namespace ui;

class UIApp : public ApplicationBase
{
	_NBL_STATIC_INLINE_CONSTEXPR uint32_t WIN_W = 1280;
	_NBL_STATIC_INLINE_CONSTEXPR uint32_t WIN_H = 720;
	_NBL_STATIC_INLINE_CONSTEXPR uint32_t SC_IMG_COUNT = 3u;
	_NBL_STATIC_INLINE_CONSTEXPR uint32_t FRAMES_IN_FLIGHT = 5u;
	static_assert(FRAMES_IN_FLIGHT > SC_IMG_COUNT);

	public:
		void setWindow(core::smart_refctd_ptr<nbl::ui::IWindow>&& wnd) override
		{
			window = std::move(wnd);
		}
		void setSystem(core::smart_refctd_ptr<nbl::system::ISystem>&& s) override
		{
			system = std::move(s);
		}
		nbl::ui::IWindow* getWindow() override
		{
			return window.get();
		}
		video::IAPIConnection* getAPIConnection() override
		{
			return gl.get();
		}
		video::ILogicalDevice* getLogicalDevice()  override
		{
			return logicalDevice.get();
		}
		video::IGPURenderpass* getRenderpass() override
		{
			return renderpass.get();
		}
		void setSurface(core::smart_refctd_ptr<video::ISurface>&& s) override
		{
			surface = std::move(s);
		}
		void setFBOs(std::vector<core::smart_refctd_ptr<video::IGPUFramebuffer>>& f) override
		{
			for (int i = 0; i < f.size(); i++)
			{
				fbos[i] = core::smart_refctd_ptr(f[i]);
			}
		}
		void setSwapchain(core::smart_refctd_ptr<video::ISwapchain>&& s) override
		{
			swapchain = std::move(s);
		}
		uint32_t getSwapchainImageCount() override
		{
			return SC_IMG_COUNT;
		}
		nbl::asset::E_FORMAT getDepthFormat() override
		{
			return nbl::asset::EF_D32_SFLOAT;
		}

		APP_CONSTRUCTOR(UIApp)
		void onAppInitialized_impl() override
		{
			initOutput.window = core::smart_refctd_ptr(window);
			
			const auto swapchainImageUsage = static_cast<asset::IImage::E_USAGE_FLAGS>(asset::IImage::EUF_COLOR_ATTACHMENT_BIT);
			const video::ISurface::SFormat surfaceFormat(asset::EF_R8G8B8A8_SRGB, asset::ECP_SRGB, asset::EOTF_sRGB);

	        CommonAPI::InitWithDefaultExt(
				initOutput, 
				video::EAT_VULKAN, 
				"UI", 
				WIN_W, 
				WIN_H, 
				SC_IMG_COUNT, 
				swapchainImageUsage, 
				surfaceFormat, 
				nbl::asset::EF_D32_SFLOAT
			);

		    window = std::move(initOutput.window);

			gl = std::move(initOutput.apiConnection);
			surface = std::move(initOutput.surface);
			gpuPhysicalDevice = std::move(initOutput.physicalDevice);
			logicalDevice = std::move(initOutput.logicalDevice);
			queues = std::move(initOutput.queues);
			swapchain = std::move(initOutput.swapchain);
			renderpass = std::move(initOutput.renderpass);
			fbos = std::move(initOutput.fbo);
			commandPools = std::move(initOutput.commandPools);
			assetManager = std::move(initOutput.assetManager);
			logger = std::move(initOutput.logger);
			inputSystem = std::move(initOutput.inputSystem);
			system = std::move(initOutput.system);
			windowCallback = std::move(initOutput.windowCb);
			cpu2gpuParams = std::move(initOutput.cpu2gpuParams);
			utilities = std::move(initOutput.utilities);

			nbl::video::IGPUObjectFromAssetConverter cpu2gpu;

			ui = core::make_smart_refctd_ptr<nbl::ext::imgui::UI>(
				logicalDevice, 
				FRAMES_IN_FLIGHT,
				renderpass, 
				nullptr, 
				cpu2gpuParams,
				window
			);

			ui->Register([this]()->void{
				ui->BeginWindow("Test window");
				ui->SetNextItemWidth(100);
				ui->Text("Hi");
				ui->SetNextItemWidth(100);
				ui->Button("Button", []()->void {
					printf("Button pressed!\n");
				});
				ui->EndWindow();
			});

			logicalDevice->createCommandBuffers(commandPools[CommonAPI::InitOutput::EQT_GRAPHICS].get(),video::IGPUCommandBuffer::EL_PRIMARY,FRAMES_IN_FLIGHT,commandBuffers);

			//
			for (uint32_t i = 0u; i < FRAMES_IN_FLIGHT; i++)
			{
				imageAcquire[i] = logicalDevice->createSemaphore();
				renderFinished[i] = logicalDevice->createSemaphore();
			}
		}

		void onAppTerminated_impl() override
		{
			logicalDevice->waitIdle();
		}

		void workLoopBody() override
		{
			++resourceIx;
			if (resourceIx >= FRAMES_IN_FLIGHT)
				resourceIx = 0;

			auto& commandBuffer = commandBuffers[resourceIx];
			auto& fence = frameComplete[resourceIx];

			if (fence)
				logicalDevice->blockForFences(1u, &fence.get());
			else
				fence = logicalDevice->createFence(static_cast<video::IGPUFence::E_CREATE_FLAGS>(0));

			commandBuffer->reset(nbl::video::IGPUCommandBuffer::ERF_RELEASE_RESOURCES_BIT);
			commandBuffer->begin(0);

			auto acquireResult = swapchain->acquireNextImage(imageAcquire[resourceIx].get(), /*fence.get()*/ nullptr, &acquiredNextFBO);
			assert(acquireResult == ISwapchain::EAIR_SUCCESS);

			asset::SViewport viewport;
			viewport.minDepth = 1.f;
			viewport.maxDepth = 0.f;
			viewport.x = 0u;
			viewport.y = 0u;
			viewport.width = WIN_W;
			viewport.height = WIN_H;
			commandBuffer->setViewport(0u, 1u, &viewport);

			nbl::video::IGPUCommandBuffer::SRenderpassBeginInfo beginInfo;
			{
				VkRect2D area;
				area.offset = { 0,0 };
				area.extent = { WIN_W, WIN_H };
				asset::SClearValue clear[2] = {};
				clear[0].color.float32[0] = 0.f;
				clear[0].color.float32[1] = 0.f;
				clear[0].color.float32[2] = 0.f;
				clear[0].color.float32[3] = 1.f;
				clear[1].depthStencil.depth = 0.f;

				beginInfo.clearValueCount = 2u;
				beginInfo.framebuffer = fbos[acquiredNextFBO];
				beginInfo.renderpass = renderpass;
				beginInfo.renderArea = area;
				beginInfo.clearValues = clear;
			}

			commandBuffer->beginRenderPass(&beginInfo, nbl::asset::ESC_INLINE);

			// TODO: Use real deltaTime instead
			float deltaTimeInSec = 0.1f;
			
			ui->Render(*commandBuffer, resourceIx);

			commandBuffer->endRenderPass();
			commandBuffer->end();

			logicalDevice->resetFences(1, &fence.get());
        
			CommonAPI::Submit(
				logicalDevice.get(), 
				swapchain.get(), 
				commandBuffer.get(), 
				queues[decltype(initOutput)::EQT_GRAPHICS], 
				imageAcquire[resourceIx].get(), 
				renderFinished[resourceIx].get(), 
				fence.get()
			);
			CommonAPI::Present(
				logicalDevice.get(), 
				swapchain.get(), 
				queues[decltype(initOutput)::EQT_GRAPHICS], 
				renderFinished[resourceIx].get(), 
				acquiredNextFBO
			);

			CommonAPI::InputSystem::ChannelReader<IMouseEventChannel> mouse;

			inputSystem->getDefaultMouse(&mouse);

			static std::chrono::microseconds previousEventTimestamp {};

			std::vector<SMouseEvent> validEvents {};

			mouse.consumeEvents([&validEvents](const IMouseEventChannel::range_t& events) -> void {
				for (auto event : events)
				{
					if (event.timeStamp < previousEventTimestamp)
					{
						continue;
					}
					previousEventTimestamp = event.timeStamp;

					validEvents.push_back(event);
				}
			});

			auto const mousePosition = window->getCursorControl()->getPosition();

			ui->Update(
				deltaTimeInSec, 
				static_cast<float>(mousePosition.x), 
				static_cast<float>(mousePosition.y),
				validEvents.size(),
				validEvents.data()
			);
		}

		bool keepRunning() override
		{
			return windowCallback->isWindowOpen();
		}

	private:
		CommonAPI::InitOutput initOutput;
		nbl::core::smart_refctd_ptr<nbl::ui::IWindow> window;
		nbl::core::smart_refctd_ptr<nbl::video::IAPIConnection> gl;
		nbl::core::smart_refctd_ptr<nbl::video::ISurface> surface;
		nbl::video::IPhysicalDevice* gpuPhysicalDevice;
		nbl::core::smart_refctd_ptr<nbl::video::ILogicalDevice> logicalDevice;
		std::array<nbl::video::IGPUQueue*, CommonAPI::InitOutput::MaxQueuesCount> queues;
		nbl::core::smart_refctd_ptr<nbl::video::ISwapchain> swapchain;
		nbl::core::smart_refctd_ptr<nbl::video::IGPURenderpass> renderpass;
		std::array<nbl::core::smart_refctd_ptr<nbl::video::IGPUFramebuffer>, CommonAPI::InitOutput::MaxSwapChainImageCount> fbos;
		std::array<nbl::core::smart_refctd_ptr<nbl::video::IGPUCommandPool>, CommonAPI::InitOutput::MaxQueuesCount> commandPools;
		nbl::core::smart_refctd_ptr<nbl::asset::IAssetManager> assetManager;
		nbl::core::smart_refctd_ptr<nbl::system::ILogger> logger;
		nbl::core::smart_refctd_ptr<CommonAPI::InputSystem> inputSystem;
		nbl::core::smart_refctd_ptr<nbl::system::ISystem> system;
		nbl::core::smart_refctd_ptr<CommonAPI::CommonAPIEventCallback> windowCallback;
		nbl::video::IGPUObjectFromAssetConverter::SParams cpu2gpuParams;
		nbl::core::smart_refctd_ptr<nbl::video::IUtilities> utilities;

		core::smart_refctd_ptr<video::IGPUCommandBuffer> commandBuffers[FRAMES_IN_FLIGHT];
		core::smart_refctd_ptr<video::IGPUFence> frameComplete[FRAMES_IN_FLIGHT] = { nullptr };
		core::smart_refctd_ptr<video::IGPUSemaphore> imageAcquire[FRAMES_IN_FLIGHT] = { nullptr };
		core::smart_refctd_ptr<video::IGPUSemaphore> renderFinished[FRAMES_IN_FLIGHT] = { nullptr };

		nbl::core::smart_refctd_ptr<nbl::ext::imgui::UI> ui {};

		_NBL_STATIC_INLINE_CONSTEXPR uint64_t MAX_TIMEOUT = 99999999999999ull;
		uint32_t acquiredNextFBO = {};
		int32_t resourceIx = -1;
};

NBL_COMMON_API_MAIN(UIApp)