// Copyright (C) Krzysztof Jakubowski <nadult@fastmail.fm>
// This file is part of FreeFT. See license.txt for details.

#include "gfx_device.h"

#include <fwk/gfx/canvas_2d.h>
#include <fwk/gfx/shader_compiler.h>
#include <fwk/vulkan/vulkan_command_queue.h>
#include <fwk/vulkan/vulkan_device.h>
#include <fwk/vulkan/vulkan_image.h>
#include <fwk/vulkan/vulkan_instance.h>
#include <fwk/vulkan/vulkan_swap_chain.h>
#include <fwk/vulkan/vulkan_window.h>

Ex<GfxDevice> GfxDevice::create(ZStr name, const Config &config) {
	auto title = format("FreeFT::%; built " __DATE__ " " __TIME__, name);

	VInstanceSetup setup;
	auto window_flags = VWindowFlag::resizable | VWindowFlag::allow_hidpi |
						VWindowFlag::sleep_when_minimized |
						mask(!config.window_pos, VWindowFlag::centered) |
						mask(config.fullscreen_on, VWindowFlag::fullscreen_desktop);

	VSwapChainSetup swap_chain_setup;
	// TODO: UI is configured for Unorm, shouldn't we use SRGB by default?
	swap_chain_setup.preferred_formats = {VK_FORMAT_B8G8R8A8_UNORM};
	swap_chain_setup.preferred_present_mode = VPresentMode::immediate;
	swap_chain_setup.usage =
		VImageUsage::color_att | VImageUsage::storage | VImageUsage::transfer_dst;
	swap_chain_setup.initial_layout = VImageLayout::general;

	bool debug_mode = false;
	if(debug_mode) {
		setup.debug_levels = all<VDebugLevel>;
		setup.debug_types = all<VDebugType>;
	}

	// TODO: create instance on a thread, in the meantime load resources?
	auto instance = EX_PASS(VulkanInstance::create(setup));
	IRect window_rect(config.resolution);
	if(config.window_pos)
		window_rect += *config.window_pos;

	// TODO: better window setup
	auto window = EX_PASS(VulkanWindow::create(instance, title, window_rect, window_flags));

	// TODO: prefer integrated device
	VDeviceSetup dev_setup;
	auto pref_device = instance->preferredDevice(window->surfaceHandle(), &dev_setup.queues);
	if(!pref_device)
		return ERROR("Couldn't find a suitable Vulkan device");
	auto device = EX_PASS(instance->createDevice(*pref_device, dev_setup));
	auto phys_info = instance->info(device->physId());
	print("Selected Vulkan physical device: %\nDriver version: %\n",
		  phys_info.properties.deviceName, phys_info.properties.driverVersion);
	device->addSwapChain(EX_PASS(VulkanSwapChain::create(device, window, swap_chain_setup)));

	Dynamic<ShaderCompiler> compiler;
	compiler.emplace();
	return GfxDevice{std::move(compiler), instance, device, window};
}

Ex<> GfxDevice::drawFrame(Canvas2D &canvas) {
	auto &device = *device_ref;
	auto &cmds = device.cmdQueue();
	auto swap_chain = device.swapChain();
	EXPECT(device.beginFrame());

	if(swap_chain->status() == VSwapChainStatus::image_acquired) {
		// Drawing only if swap chain is available
		auto render_pass =
			device.getRenderPass({{swap_chain->format(), 1, VColorSyncStd::clear_present}});
		auto dc = EX_PASS(canvas.genDrawCall(*compiler, device, render_pass));
		auto fb = device.getFramebuffer({swap_chain->acquiredImage()});

		cmds.beginRenderPass(fb, render_pass, none, {FColor(0.0, 0.0, 0.0)});
		cmds.setViewport(IRect(swap_chain->size()));
		cmds.setScissor(none);

		dc.render(device);
		cmds.endRenderPass();
	}
	return device.finishFrame();
}
