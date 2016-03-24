#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VKCPP_ENHANCED_MODE
#include <vulkan/vk_cpp.h>

#include <iostream>
#include <fstream>


vk::Format format;

vk::Instance create_instance()
{
	vk::ApplicationInfo appInfo("Test app", 1, "Engine", 1, VK_MAKE_VERSION(1, 0, 0));
	
	vk::InstanceCreateInfo instInfo;
	
	instInfo.pApplicationInfo(&appInfo);
	instInfo.flags(vk::InstanceCreateFlags());
	instInfo.enabledLayerCount(0);
	instInfo.ppEnabledLayerNames(nullptr);
	instInfo.enabledExtensionCount(1);
	const char* extensionNames[] = { "VK_KHR_xcb_surface" };
	instInfo.ppEnabledExtensionNames(extensionNames);
	
	
	return vk::createInstance(instInfo, vk::AllocationCallbacks::null());
}

vk::SurfaceKHR create_surface(const vk::Instance& inst, GLFWwindow* window)
{
	
	VkSurfaceKHR c_surface;
	auto result = glfwCreateWindowSurface(inst, window, nullptr, &c_surface);
	
	if(result != VK_SUCCESS)
	{
		std::cerr << "Error creating surface with error: " << vk::to_string(vk::Result(result));
		exit(-1);
	}
	
	return {c_surface};
	
}

vk::PhysicalDevice get_physical_device(const vk::Instance& inst)
{
	std::vector<vk::PhysicalDevice> phyDevices;
	inst.enumeratePhysicalDevices(phyDevices);
	
	// just select the first one
	if(phyDevices.empty()) throw std::runtime_error("No suitable device found");
	
	return phyDevices[0];
	
}

vk::Device create_device(const vk::Instance& inst, const vk::PhysicalDevice& physical_device)
{
	

	auto physDevProps = physical_device.getProperties();
	auto physDevQueueProps = physical_device.getQueueFamilyProperties();
	
	vk::DeviceQueueCreateInfo queueInfo;
	queueInfo.queueFamilyIndex(0);
	queueInfo.queueCount(physDevQueueProps[0].queueCount());
	
	float queue_priorities[] = {0.0f};
	queueInfo.pQueuePriorities(queue_priorities);
	
	
	vk::DeviceCreateInfo devInfo;
	devInfo.queueCreateInfoCount(1);
	devInfo.pQueueCreateInfos(&queueInfo);
	devInfo.enabledLayerCount(0);
	devInfo.ppEnabledLayerNames(nullptr);
	const char* deviceExtensionNames[1024];
	devInfo.enabledExtensionCount(0);
	devInfo.ppEnabledExtensionNames(deviceExtensionNames);
	devInfo.pEnabledFeatures(nullptr);
	
	return physical_device.createDevice(devInfo, vk::AllocationCallbacks::null());
	
}

vk::SwapchainKHR create_swapchain(const vk::Device& device, const vk::SurfaceKHR& surface, const vk::PhysicalDevice& physDev)
{
	vk::SurfaceCapabilitiesKHR capabilities;
	physDev.getSurfaceCapabilitiesKHR(surface, &capabilities);
	
	std::vector<vk::SurfaceFormatKHR> formats;
	physDev.getSurfaceFormatsKHR(surface, formats);
	
	format = formats[0].format();
	
	vk::SwapchainCreateInfoKHR swapchainInfo(vk::SwapchainCreateFlagsKHR(), surface, 2, format,
											 formats[0].colorSpace(), capabilities.currentExtent(), 1, 
											 vk::ImageUsageFlags(vk::ImageUsageFlagBits::eColorAttachment), vk::SharingMode::eExclusive, 0, nullptr, 
											 vk::SurfaceTransformFlagBitsKHR::eIdentity, vk::CompositeAlphaFlagBitsKHR::eOpaque, 
											 vk::PresentModeKHR::eFifoKHR, true, vk::SwapchainKHR());

	vk::SwapchainKHR ret;
	device.createSwapchainKHR(&swapchainInfo, nullptr, &ret);
	
	return ret;
}

vk::Semaphore create_semaphore(const vk::Device& device)
{
	vk::SemaphoreCreateInfo semaphoreInfo(vk::SemaphoreCreateFlags{});
	
	return device.createSemaphore(semaphoreInfo, vk::AllocationCallbacks::null());
}

vk::Fence create_fence(const vk::Device& device)
{
	vk::FenceCreateInfo fenceInfo(vk::FenceCreateFlags{});
	
	return device.createFence(fenceInfo, vk::AllocationCallbacks::null());
}

vk::ImageView create_image_view(const vk::Device& device, const vk::Image& image)
{
	vk::ImageViewCreateInfo imageViewInfo(vk::ImageViewCreateFlags{}, image, vk::ImageViewType::e2D, format, 
										  vk::ComponentMapping(), vk::ImageSubresourceRange(vk::ImageAspectFlags(), 0, 1, 0, 1));
	
	return device.createImageView(imageViewInfo, vk::AllocationCallbacks::null());
}

int main()
{
	glfwSetErrorCallback([](int error, const char* desc)
	{
		std::cerr << "GLFW error " << error << ": " << desc << std::endl; 
	});
	
	glfwInit();
	
	if(glfwVulkanSupported() == GLFW_FALSE)
	{
		std::cout << "ERROR";
	}
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	
	GLFWwindow* window = glfwCreateWindow(1024, 720, "Here", nullptr, nullptr);

	auto inst = create_instance(); assert(inst);
	auto surface = create_surface(inst, window); assert(surface);
	auto physical_device = get_physical_device(inst); assert(physical_device);
	auto device = create_device(inst, physical_device); assert(device);
	auto swapchain = create_swapchain(device, surface, physical_device); assert(swapchain);
	
	
	std::vector<vk::Image> images;
	device.getSwapchainImagesKHR(swapchain, images);
	
	auto semaphore = create_semaphore(device);
	
	uint32_t currentSwap;
	device.acquireNextImageKHR(swapchain, UINT64_MAX, semaphore, {}, currentSwap);
	
	auto image_view = create_image_view(device, images[0]);
	
	auto queue = device.getQueue(0, 0);
	
	// make render pass
	vk::AttachmentDescription attachDesc(vk::AttachmentDescriptionFlags(), format, vk::SampleCountFlagBits::e1, 
										 vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore, vk::AttachmentLoadOp::eDontCare,
									  vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference attachRef(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::SubpassDescription subpassDesc(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &attachRef, nullptr, nullptr, 0, nullptr);
	vk::RenderPassCreateInfo renderPassInfo(vk::RenderPassCreateFlags(), 1, &attachDesc, 1, &subpassDesc, 0, nullptr);
	
	auto render_pass = device.createRenderPass(renderPassInfo, vk::AllocationCallbacks::null());
	
	// make framebuffer
	vk::FramebufferCreateInfo framebufferInfo(vk::FramebufferCreateFlags(), render_pass, 1, &image_view, 1024, 720, 1);
	auto framebuffer = device.createFramebuffer(framebufferInfo, vk::AllocationCallbacks::null());
	
	// make descriptor set layout
	vk::DescriptorSetLayoutBinding bindings[] = {
		{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlags(vk::ShaderStageFlagBits::eVertex), nullptr} // vertex position
	};
	vk::DescriptorSetLayoutCreateInfo descSetLayoutCreateInfo({}, 1, bindings);
	auto descriptor_set_layout = device.createDescriptorSetLayout(descSetLayoutCreateInfo, vk::AllocationCallbacks::null());
	
	// make pipeline layout
	vk::PipelineLayoutCreateInfo pipelineLayoutInfo({}, 1, &descriptor_set_layout, 0, nullptr);
	auto pipeline_layout = device.createPipelineLayout(pipelineLayoutInfo, vk::AllocationCallbacks::null());
	
	// upload shaders
	std::ifstream fragFile("frag.spv", std::ios::binary | std::ios::ate);
	std::streamsize fragSize = fragFile.tellg();
	fragFile.seekg(0, std::ios::beg);
	std::vector<char> fragSpirVData(fragSize);
	fragFile.read(fragSpirVData.data(), fragSize);
	
	
	std::ifstream vertFile("vert.spv", std::ios::binary | std::ios::ate);
	std::streamsize vertSize = vertFile.tellg();
	vertFile.seekg(0, std::ios::beg);
	std::vector<char> vertSpirVData(vertSize);
	vertFile.read(vertSpirVData.data(), vertSize);
	
	vk::ShaderModuleCreateInfo vertModuleInfo({}, vertSize, reinterpret_cast<uint32_t*>(vertSpirVData.data()));
	vk::ShaderModuleCreateInfo fragModuleInfo({}, fragSize, reinterpret_cast<uint32_t*>(fragSpirVData.data()));
	
	auto vert_module = device.createShaderModule(vertModuleInfo, vk::AllocationCallbacks::null());
	auto frag_module = device.createShaderModule(fragModuleInfo, vk::AllocationCallbacks::null());
	
	// viewport
	auto viewport = vk::Viewport(0, 0, 1024, 720, 0, 100);
	
	// make graphics pipeline: HOLY SHIT THAT'S A LOT OF METADATA
	vk::PipelineShaderStageCreateInfo pipeShaderStageInfo[] = {
		{{}, vk::ShaderStageFlagBits::eVertex, vert_module, "Vert Shader", nullptr},
		{{}, vk::ShaderStageFlagBits::eFragment, frag_module, "Frag Shader", nullptr}
	};
	vk::VertexInputAttributeDescription vertInputAttrDesc(0, 0, vk::Format::eR32G32B32Sfloat, 0);
	vk::PipelineVertexInputStateCreateInfo pipeVertexInputStateInfo({}, 0, nullptr, 1, &vertInputAttrDesc);
	vk::PipelineInputAssemblyStateCreateInfo pipeInputAsmStateInfo({}, vk::PrimitiveTopology::eTriangleList, false); // TODO: not sure
	vk::PipelineTessellationStateCreateInfo pipeTessStateInfo({}, 0);
	vk::Rect2D scissor({}, vk::Extent2D(1024, 720));
	vk::PipelineViewportStateCreateInfo pipeViewportStateInfo({}, 1, &viewport, 1, &scissor);
	vk::PipelineRasterizationStateCreateInfo pipeRasterizationStateInfo({}, false, false, vk::PolygonMode::eFill, vk::CullModeFlags(vk::CullModeFlagBits::eNone), vk::FrontFace::eClockwise, true, 1.f, 100, 1.f, 1.f);
	vk::SampleMask sampleMask = 1; // TODO: ???? this is just an int
	vk::PipelineMultisampleStateCreateInfo pipeMultisampleStateInfo({}, vk::SampleCountFlagBits::e16, false, 1.f/2.f, &sampleMask, false, false); // TODO: not sure
	vk::PipelineDepthStencilStateCreateInfo pipeDepthStencilStateInfo({}, true, true, vk::CompareOp::eLessOrEqual, true, false, vk::StencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eEqual, 1, 1, 1), vk::StencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eEqual, 1, 1, 1), 0.f, 100.f); // TODO: not sure
	vk::PipelineColorBlendStateCreateInfo pipeColorBlendStateInfo({}, false, vk::LogicOp::eAnd, 0, nullptr, {{0.f, 0.f, 1.f, 1.f}}); // TODO: not sure
	vk::PipelineDynamicStateCreateInfo pipeDynStateInfo({}, 0, nullptr); // TODO: not sure
	vk::GraphicsPipelineCreateInfo graphicsPipeInfo({}, 2, pipeShaderStageInfo, &pipeVertexInputStateInfo, &pipeInputAsmStateInfo, &pipeTessStateInfo, &pipeViewportStateInfo, &pipeRasterizationStateInfo, &pipeMultisampleStateInfo, &pipeDepthStencilStateInfo, &pipeColorBlendStateInfo, &pipeDynStateInfo, pipeline_layout, render_pass, 0, {}, 0);
	auto graphics_pipeline = device.createGraphicsPipelines({}, {graphicsPipeInfo}, vk::AllocationCallbacks::null())[0];
	
	// make the descriptor pool
	vk::DescriptorPoolSize descPoolSize(vk::DescriptorType::eStorageBuffer, 1);
	vk::DescriptorPoolCreateInfo descPoolInfo({}, 1, 1, &descPoolSize);
	auto descriptor_pool = device.createDescriptorPool(descPoolInfo, vk::AllocationCallbacks::null());
	
	// make the descriptor set
	vk::DescriptorSetAllocateInfo descSetAllocInfo(descriptor_pool, 1, &descriptor_set_layout);
	auto descriptor_set = device.allocateDescriptorSets(descSetAllocInfo)[0];
	
	
	
	// make a command pool
	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlags(), 0);
	auto commandPool = device.createCommandPool(poolInfo, vk::AllocationCallbacks::null());
	
	// make the command buffer
	vk::CommandBufferAllocateInfo commandBufferInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1); 
	auto commandBuffer = device.allocateCommandBuffers(commandBufferInfo);
	
	
	device.destroy(nullptr);
	glfwDestroyWindow(window);
	inst.destroy(nullptr);
	
	
}
