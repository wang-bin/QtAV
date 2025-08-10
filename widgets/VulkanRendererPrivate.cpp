#include "VulkanRendererPrivate.h"
#include <QFile>
#include <QCoreApplication>
#include <QVulkanInstance>
#include <QVulkanDeviceFunctions>
#include <QThread>

namespace QtAV
{
	constexpr uint32_t Matrix4x4Size = 64;

	std::vector<Vertex> vertices = {
		{{-1.0f, 1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f}, {1.0f, 1.0f}},
		{{1.0f, -1.0f}, {1.0f, 0.0f}},
		{{1.0f, -1.0f}, {1.0f, 0.0f}},
		{{-1.0f, -1.0f}, {0.0f, 0.0f}},
		{{-1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	static QByteArray readShaderFile(const QString& fileName)
	{
		QFile f(qApp->applicationDirPath() + QStringLiteral("/") + fileName);
		if (!f.exists()) {
			f.setFileName(QStringLiteral(":/") + fileName);
		}
		if (!f.open(QIODevice::ReadOnly)) {
			qWarning("Can not load shader %s: %s", f.fileName().toUtf8().constData(), f.errorString().toUtf8().constData());
			return QByteArray();
		}
		QByteArray src = f.readAll();
		f.close();
		return src;
	}

	VulkanRendererPrivate::VulkanRendererPrivate()
	{
	}

	VulkanRendererPrivate::~VulkanRendererPrivate()
	{
	}

	void VulkanRendererPrivate::setupAspectRatio()
	{
		matrix.setToIdentity();
		matrix.scale(
			static_cast<float>(out_rect.width()) / static_cast<float>(renderer_width),
			static_cast<float>(out_rect.height()) / static_cast<float>(renderer_height),
			1
		);
		if (rotation())
			matrix.rotate(rotation(), 0, 0, 1); // Z axis

		if (vulkanWindowRenderer)
			vulkanWindowRenderer->setPosTransformMatrix(matrix);
	}

	VulkanWindowRenderer::VulkanWindowRenderer(QVulkanWindow* w)
		: m_window(w),
		m_vulkanInstance(w->vulkanInstance()),
		m_vulkanFunctions(m_vulkanInstance->functions())
	{
	}

	void VulkanWindowRenderer::initResources()
	{
		m_device = m_window->device();
		m_vulkanDeviceFunctions = m_vulkanInstance->deviceFunctions(m_device);
		m_renderPass = m_window->defaultRenderPass();
		createTextureSampler();
		createVertexBuffer();
		createDescriptorSetLayout();
	}

	void VulkanWindowRenderer::initSwapChainResources()
	{
		m_swapChainImageCount = m_window->swapChainImageCount();
		m_swapChainImageSize = m_window->swapChainImageSize();

		createGraphicsPipeline();
		createUniformBuffers();
		createDescriptorPool();
	}

	void VulkanWindowRenderer::preInitResources()
	{
		m_physicalDevice = m_window->physicalDevice();
	}

	void VulkanWindowRenderer::releaseResources()
	{
		cleanupTextureSampler();
		cleanupVertexBuffer();
		cleanupDescriptorSetLayout();
		m_vulkanDeviceFunctions = nullptr;
		m_device = VK_NULL_HANDLE;
		m_renderPass = VK_NULL_HANDLE;
		m_physicalDevice = VK_NULL_HANDLE;
	}

	void VulkanWindowRenderer::releaseSwapChainResources()
	{
		m_swapChainImageCount = 0;
		m_swapChainImageSize = { 0, 0 };

		cleanupGraphicsPipeline();
		cleanupUniformBuffers();
		cleanupDescriptorPool();
		cleanupDescriptorSets();
		cleanupTextureObject();
	}

	void VulkanWindowRenderer::startNextFrame()
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);

		if (!m_currentFrame.isValid())
		{
			m_window->frameReady();
			return;
		}

		if (!checkTextureObject())
		{
			cleanupTextureObject();
			cleanupDescriptorSets();

			createTextureObject();
			createDescriptorSets();
		}

		VkCommandBuffer commandBuffer = m_window->currentCommandBuffer();
		auto currentSwapChainImageIndex = m_window->currentSwapChainImageIndex();

		updateUniformBuffer(currentSwapChainImageIndex);
		updateTextureImage(currentSwapChainImageIndex);

		// 渲染通道启动信息
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_window->currentFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { (uint32_t)m_swapChainImageSize.width(), (uint32_t)m_swapChainImageSize.height() };

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		m_vulkanDeviceFunctions->vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		m_vulkanDeviceFunctions->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		m_vulkanDeviceFunctions->vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		m_vulkanDeviceFunctions->vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[currentSwapChainImageIndex], 0, nullptr);
		m_vulkanDeviceFunctions->vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

		m_vulkanDeviceFunctions->vkCmdEndRenderPass(commandBuffer);

		m_window->frameReady();
	}

	void VulkanWindowRenderer::setCurrentFrame(const VideoFrame& frame)
	{
		std::lock_guard<std::mutex> lock(m_frameMutex);
		m_currentFrame = frame;
	}

	void VulkanWindowRenderer::setColorTransformMatrix(const QMatrix4x4& matrix)
	{
		std::lock_guard<std::mutex> lock(m_uboObjMutex);
		m_uboObj.colorTransform = matrix;
		std::fill(m_uboUploaded.begin(), m_uboUploaded.end(), false);
	}

	void VulkanWindowRenderer::setPosTransformMatrix(const QMatrix4x4& matrix)
	{
		std::lock_guard<std::mutex> lock(m_uboObjMutex);
		m_uboObj.posTransform = matrix;
		std::fill(m_uboUploaded.begin(), m_uboUploaded.end(), false);
	}

	VkShaderModule VulkanWindowRenderer::createShaderModule(const QByteArray& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;
		m_vulkanDeviceFunctions->vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule);

		return shaderModule;
	}

	void VulkanWindowRenderer::createGraphicsPipeline()
	{
		// 创建着色器
		VkShaderModule vertShaderModule = createShaderModule(readShaderFile("shaders/vert.spv"));
		VkShaderModule fragShaderModule = createShaderModule(readShaderFile("shaders/frag.spv"));

		// 着色器信息
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// 顶点信息
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

		auto bindingDescription = Vertex::getBindingDescription();			// 顶点绑定说明
		auto attributeDescriptions = Vertex::getAttributeDescriptions();	// 属性说明
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// 图元装配信息
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;	// 图元拓扑类型
		inputAssembly.primitiveRestartEnable = VK_FALSE;	// 是否启用图元重启

		// 视口和裁剪信息
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChainImageSize.width();
		viewport.height = (float)m_swapChainImageSize.height();
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { (uint32_t)m_swapChainImageSize.width(), (uint32_t)m_swapChainImageSize.height() };

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// 光栅化信息
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;	// 是否启用深度钳位
		rasterizer.rasterizerDiscardEnable = VK_FALSE;	// 是否启用光栅化
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;	// 填充模式
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;	// 面剔除模式	
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// 定义正面
		rasterizer.depthBiasEnable = VK_FALSE;	// 是否启用深度偏移

		// 多重采样信息
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;	// 是否启用样本着色
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// 每个像素的采样数

		// 颜色混合信息
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};	// 颜色混合附件信息
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;	// 颜色写入掩码
		colorBlendAttachment.blendEnable = VK_FALSE;	// 是否启用混合

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;	// 是否启用逻辑操作
		colorBlending.logicOp = VK_LOGIC_OP_COPY;	// 逻辑操作类型
		colorBlending.attachmentCount = 1;	// 颜色附件的数量
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// 管线布局信息
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;

		if (m_vulkanDeviceFunctions->vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
		{
			qWarning() << "failed to create pipeline layout!";
			return;
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;      // 关闭深度测试
		depthStencil.depthWriteEnable = VK_FALSE;     // 关闭深度写入
		depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS; // 比较操作（无实际意义，因为测试已关闭）
		depthStencil.depthBoundsTestEnable = VK_FALSE; // 关闭深度边界测试
		depthStencil.stencilTestEnable = VK_FALSE;    // 关闭模板测试

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.pDepthStencilState = &depthStencil;

		if (m_vulkanDeviceFunctions->vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
		{
			qWarning() << "failed to create graphics pipeline!";
			return;
		}

		m_vulkanDeviceFunctions->vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
		m_vulkanDeviceFunctions->vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
	}

	void VulkanWindowRenderer::createVertexBuffer()
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		m_vulkanDeviceFunctions->vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		m_vulkanDeviceFunctions->vkUnmapMemory(m_device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

		copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

		m_vulkanDeviceFunctions->vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		m_vulkanDeviceFunctions->vkFreeMemory(m_device, stagingBufferMemory, nullptr);
	}

	void VulkanWindowRenderer::createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;	// 绑定点索引
		uboLayoutBinding.descriptorCount = 1;	// 此绑定点的描述符数量
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// 描述符类型
		uboLayoutBinding.pImmutableSamplers = nullptr;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	// 指定哪些着色器可以访问此绑定点

		VkDescriptorSetLayoutBinding ySamplerLayoutBinding{};
		ySamplerLayoutBinding.binding = 1;
		ySamplerLayoutBinding.descriptorCount = 1;
		ySamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		ySamplerLayoutBinding.pImmutableSamplers = nullptr;
		ySamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding uSamplerLayoutBinding{};
		uSamplerLayoutBinding.binding = 2;
		uSamplerLayoutBinding.descriptorCount = 1;
		uSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		uSamplerLayoutBinding.pImmutableSamplers = nullptr;
		uSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding vSamplerLayoutBinding{};
		vSamplerLayoutBinding.binding = 3;
		vSamplerLayoutBinding.descriptorCount = 1;
		vSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		vSamplerLayoutBinding.pImmutableSamplers = nullptr;
		vSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings = { uboLayoutBinding, ySamplerLayoutBinding, uSamplerLayoutBinding, vSamplerLayoutBinding };

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (m_vulkanDeviceFunctions->vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		{
			qDebug() << "failed to create descriptor set layout!";
		}
	}

	void VulkanWindowRenderer::createUniformBuffers()
	{
		VkDeviceSize bufferSize = Matrix4x4Size * 2;

		m_uniformBuffers.resize(m_swapChainImageCount);
		m_uniformBuffersMemory.resize(m_swapChainImageCount);
		m_uboUploaded.resize(m_swapChainImageCount, false);

		for (size_t i = 0; i < m_swapChainImageCount; i++)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
		}
	}

	void VulkanWindowRenderer::createDescriptorPool()
	{
		{
			std::array<VkDescriptorPoolSize, 2> poolSizes{};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = 10;
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = 10;

			VkDescriptorPoolCreateInfo poolInfo{};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = 20;

			if (m_vulkanDeviceFunctions->vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
			{
				qDebug() << "failed to create descriptor pool!";
			}
			return;
		}

		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapChainImageCount);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapChainImageCount);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(m_swapChainImageCount);

		if (m_vulkanDeviceFunctions->vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		{
			qDebug() << "failed to create descriptor pool!";
		}
	}

	void VulkanWindowRenderer::createDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(m_swapChainImageCount, m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImageCount);
		allocInfo.pSetLayouts = layouts.data();

		m_descriptorSets.resize(m_swapChainImageCount);
		if (m_vulkanDeviceFunctions->vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
		{
			qDebug() << "failed to allocate descriptor sets!";
			return;
		}

		for (size_t i = 0; i < m_swapChainImageCount; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo yImageInfo{};
			yImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			yImageInfo.imageView = m_yTextureObjects[i].imageView;
			yImageInfo.sampler = m_textureSampler;

			VkDescriptorImageInfo uImageInfo{};
			uImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			uImageInfo.imageView = m_uTextureObjects[i].imageView;
			uImageInfo.sampler = m_textureSampler;

			VkDescriptorImageInfo vImageInfo{};
			vImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vImageInfo.imageView = m_vTextureObjects[i].imageView;
			vImageInfo.sampler = m_textureSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &yImageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = m_descriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &uImageInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = m_descriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &vImageInfo;

			m_vulkanDeviceFunctions->vkUpdateDescriptorSets(
				m_device,
				static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(),
				0, nullptr
			);
		}
	}

	void VulkanWindowRenderer::createTextureObject()
	{
		auto planeCount = m_currentFrame.planeCount();
		if (planeCount != 3)
		{
			qDebug() << "Invalid plane count, expected 3 for YUV format!";
			return;
		}

		m_yTextureObjects.resize(m_swapChainImageCount);
		m_uTextureObjects.resize(m_swapChainImageCount);
		m_vTextureObjects.resize(m_swapChainImageCount);

		VkImageSubresource subresource{};
		subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresource.mipLevel = 0;
		subresource.arrayLayer = 0;

		// 封装纹理创建的lambda函数
		auto createPlaneTextures = [&](std::vector<TextureObject>& textures, int planeIndex) {
			for (size_t i = 0; i < m_swapChainImageCount; ++i)
			{
				auto& texture = textures[i];
				texture.width = m_currentFrame.planeWidth(planeIndex);
				texture.height = m_currentFrame.planeHeight(planeIndex);

				createImage(
					texture.width,
					texture.height,
					VK_FORMAT_R8_UNORM,
					VK_IMAGE_TILING_LINEAR,
					VK_IMAGE_USAGE_SAMPLED_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					texture.image,
					texture.memory
				);

				texture.imageView = createImageView(texture.image, VK_FORMAT_R8_UNORM);

				m_vulkanDeviceFunctions->vkMapMemory(
					m_device,
					texture.memory,
					0,
					texture.width * texture.height,
					0,
					&texture.mappedMemory
				);

				m_vulkanDeviceFunctions->vkGetImageSubresourceLayout(
					m_device,
					texture.image,
					&subresource,
					&texture.layout
				);
			}
			};

		createPlaneTextures(m_yTextureObjects, 0);
		createPlaneTextures(m_uTextureObjects, 1);
		createPlaneTextures(m_vTextureObjects, 2);
	}


	void VulkanWindowRenderer::createTextureSampler()
	{
		VkPhysicalDeviceProperties properties{};
		m_vulkanFunctions->vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

		// 线性过滤
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		// 纹理坐标超出范围时钳位到边缘，避免视频边缘出现重复或杂色
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		// 禁用各向异性过滤
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (m_vulkanDeviceFunctions->vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS) {
			qDebug() << "failed to create texture sampler!";
		}
	}


	void VulkanWindowRenderer::cleanupGraphicsPipeline()
	{
		if (m_graphicsPipeline)
		{
			m_vulkanDeviceFunctions->vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
			m_graphicsPipeline = VK_NULL_HANDLE;
		}

		if (m_pipelineLayout)
		{
			m_vulkanDeviceFunctions->vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
			m_pipelineLayout = VK_NULL_HANDLE;
		}
	}

	void VulkanWindowRenderer::cleanupVertexBuffer()
	{
		if (m_vertexBuffer)
		{
			m_vulkanDeviceFunctions->vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
			m_vertexBuffer = VK_NULL_HANDLE;
		}

		if (m_vertexBufferMemory)
		{
			m_vulkanDeviceFunctions->vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
			m_vertexBufferMemory = VK_NULL_HANDLE;
		}
	}

	void VulkanWindowRenderer::cleanupDescriptorSetLayout()
	{
		if (m_descriptorSetLayout)
		{
			m_vulkanDeviceFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
			m_descriptorSetLayout = VK_NULL_HANDLE;
		}
	}

	void VulkanWindowRenderer::cleanupUniformBuffers()
	{
		for (size_t i = 0; i < m_swapChainImageCount; i++)
		{
			if (i >= m_uniformBuffers.size())
				break;

			if (i >= m_uniformBuffersMemory.size())
				break;

			if (m_uniformBuffers[i])
				m_vulkanDeviceFunctions->vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);

			if (m_uniformBuffersMemory[i])
				m_vulkanDeviceFunctions->vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
		}

		m_uniformBuffers.clear();
		m_uniformBuffersMemory.clear();
		m_uboUploaded.clear();
	}

	void VulkanWindowRenderer::cleanupDescriptorPool()
	{
		if (m_descriptorPool)
		{
			m_vulkanDeviceFunctions->vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
			m_descriptorPool = VK_NULL_HANDLE;
		}
	}

	void VulkanWindowRenderer::cleanupDescriptorSets()
	{
		m_descriptorSets.clear();
	}

	void VulkanWindowRenderer::cleanupTextureObject()
	{
		auto cleanupTextureObjectFun = [this](std::vector<TextureObject>& textureObjects) {
			for (auto& textureObject : textureObjects)
			{
				if (textureObject.memory != VK_NULL_HANDLE)
				{
					m_vulkanDeviceFunctions->vkUnmapMemory(m_device, textureObject.memory);
					textureObject.mappedMemory = nullptr;
				}

				if (textureObject.imageView != VK_NULL_HANDLE)
				{
					m_vulkanDeviceFunctions->vkDestroyImageView(m_device, textureObject.imageView, nullptr);
					textureObject.imageView = VK_NULL_HANDLE;
				}

				if (textureObject.image != VK_NULL_HANDLE)
				{
					m_vulkanDeviceFunctions->vkDestroyImage(m_device, textureObject.image, nullptr);
					textureObject.image = VK_NULL_HANDLE;
				}

				if (textureObject.memory != VK_NULL_HANDLE)
				{
					m_vulkanDeviceFunctions->vkFreeMemory(m_device, textureObject.memory, nullptr);
					textureObject.memory = VK_NULL_HANDLE;
				}
			}
			textureObjects.clear();
			};

		cleanupTextureObjectFun(m_yTextureObjects);
		cleanupTextureObjectFun(m_uTextureObjects);
		cleanupTextureObjectFun(m_vTextureObjects);
	}


	void VulkanWindowRenderer::cleanupTextureSampler()
	{
		if (m_textureSampler)
		{
			m_vulkanDeviceFunctions->vkDestroySampler(m_device, m_textureSampler, nullptr);
			m_textureSampler = VK_NULL_HANDLE;
		}
	}

	void VulkanWindowRenderer::updateUniformBuffer(uint32_t currentImage)
	{
		std::lock_guard<std::mutex> lock(m_uboObjMutex);

		if (m_uboUploaded[currentImage])
			return;
		m_uboUploaded[currentImage] = true;

		void* data;
		m_vulkanDeviceFunctions->vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(Matrix4x4Size * 2), 0, &data);
		char* ptr = static_cast<char*>(data);
		std::memcpy(ptr, m_uboObj.posTransform.constData(), Matrix4x4Size);
		std::memcpy(ptr + Matrix4x4Size, m_uboObj.colorTransform.constData(), Matrix4x4Size);
		m_vulkanDeviceFunctions->vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);
	}

	uint32_t VulkanWindowRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		m_vulkanFunctions->vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		qWarning() << "failed to find suitable memory type!";
		return 0;
	}

	void VulkanWindowRenderer::createBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory
	)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (m_vulkanDeviceFunctions->vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			qWarning() << "failed to create buffer!";
			return;
		}

		VkMemoryRequirements memRequirements;
		m_vulkanDeviceFunctions->vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (m_vulkanDeviceFunctions->vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			qWarning() << "failed to allocate buffer memory!";
			return;
		}

		m_vulkanDeviceFunctions->vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	}

	void VulkanWindowRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_window->graphicsCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		m_vulkanDeviceFunctions->vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		m_vulkanDeviceFunctions->vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		m_vulkanDeviceFunctions->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		m_vulkanDeviceFunctions->vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		m_vulkanDeviceFunctions->vkQueueSubmit(m_window->graphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		m_vulkanDeviceFunctions->vkQueueWaitIdle(m_window->graphicsQueue());

		m_vulkanDeviceFunctions->vkFreeCommandBuffers(m_device, m_window->graphicsCommandPool(), 1, &commandBuffer);
	}

	void VulkanWindowRenderer::createImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory
	)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (m_vulkanDeviceFunctions->vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
		{
			qDebug() << "failed to create image!";
			return;
		}

		VkMemoryRequirements memRequirements;
		m_vulkanDeviceFunctions->vkGetImageMemoryRequirements(m_device, image, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (m_vulkanDeviceFunctions->vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
		{
			qDebug() << "failed to allocate image memory!";
		}

		m_vulkanDeviceFunctions->vkBindImageMemory(m_device, image, imageMemory, 0);
	}

	VkImageView VulkanWindowRenderer::createImageView(VkImage image, VkFormat format)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;  // 颜色通道映射（直接使用原始通道）
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

		VkImageView imageView;
		if (m_vulkanDeviceFunctions->vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
			qDebug() << "failed to create texture image view!";
		}

		return imageView;
	}

	void VulkanWindowRenderer::updateTextureImage(uint32_t currentImage)
	{
		if (currentImage >= static_cast<uint32_t>(m_swapChainImageCount)) 
		{
			qDebug() << "updateTextureImage: Invalid currentImage index (" << currentImage << ")";
			return;
		}

		updateSinglePlaneTexture(m_yTextureObjects[currentImage], m_currentFrame.constBits(0));
		updateSinglePlaneTexture(m_uTextureObjects[currentImage], m_currentFrame.constBits(1));
		updateSinglePlaneTexture(m_vTextureObjects[currentImage], m_currentFrame.constBits(2));
	}

	void VulkanWindowRenderer::updateSinglePlaneTexture(const TextureObject& textureObj, const uchar* planeData)
	{
		if (!planeData) {
			qDebug() << "updateSinglePlaneTexture: Invalid plane data";
			return;
		}

		const uchar* srcData = planeData;
		uchar* dstData = static_cast<uchar*>(textureObj.mappedMemory) + textureObj.layout.offset;
		const uint32_t rowPitch = textureObj.layout.rowPitch;
		const uint32_t rowSize = textureObj.width;

		if (rowPitch == rowSize)
		{
			memcpy(dstData, srcData, rowPitch * textureObj.height);
		}
		else
		{
			for (uint32_t y = 0; y < textureObj.height; ++y)
			{
				memcpy(dstData, srcData, rowSize);
				srcData += rowSize;
				dstData += rowPitch;
			}
		}
	}

	bool VulkanWindowRenderer::checkTextureObject()
	{
		if (m_yTextureObjects.empty() || m_uTextureObjects.empty() || m_vTextureObjects.empty())
			return false;

		auto& yTextureObject = m_yTextureObjects[0];
		auto& uTextureObject = m_uTextureObjects[0];
		auto& vTextureObject = m_vTextureObjects[0];

		if (yTextureObject.width != m_currentFrame.planeWidth(0) || yTextureObject.height != m_currentFrame.planeHeight(0))
			return false;

		if (uTextureObject.width != m_currentFrame.planeWidth(1) || uTextureObject.height != m_currentFrame.planeHeight(1))
			return false;

		if (vTextureObject.width != m_currentFrame.planeWidth(2) || vTextureObject.height != m_currentFrame.planeHeight(2))
			return false;

		return true;
	}
}