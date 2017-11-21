//
// Created by chakmeshma on 20.11.2017.
//

#include <fstream>
#include <cygwin/stat.h>
#include "Vulkan Engine.h"
//#pragma comment(linker, "/STACK:20000000000")

//#pragma comment(linker, "/HEAP:2000000000")

VulkanEngine** ppUnstableInstance_img = NULL;

VulkanEngine::VulkanEngine(HINSTANCE hInstance, HWND windowHandle, VulkanEngine** ppUnstableInstance) {
    *ppUnstableInstance = this;
    ppUnstableInstance_img = ppUnstableInstance;

    this->hInstance = hInstance;
    this->windowHandle = windowHandle;

    init();
}

VulkanEngine::~VulkanEngine() {
    terminate();
}

void VulkanEngine::getInstanceExtensions() {
    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, nullptr);

    instanceExtensions.resize(instanceExtensionsCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, instanceExtensions.data());

    std::cout << "Instance Extensions:\n";

    for (uint32_t i = 0; i < instanceExtensionsCount; i++) {
        std::cout << "\t" << instanceExtensions[i].extensionName << "(" << std::to_string(instanceExtensions[i].specVersion) << ")\n";
    }
}

void VulkanEngine::showDeviceExtensions() {
    vkEnumerateDeviceExtensionProperties(physicalDevices[0], nullptr, &deviceExtensionsCount, nullptr);

    deviceExtensions.resize(deviceExtensionsCount);

    vkEnumerateDeviceExtensionProperties(physicalDevices[0], nullptr, &deviceExtensionsCount, deviceExtensions.data());

    std::cout << "Device Extensions:\n";

    for (int i = 0; i < deviceExtensionsCount; i++) {
        std::cout << "\t" << deviceExtensions[i].extensionName << "(" << std::to_string(deviceExtensions[0].specVersion) << ")\n";
    }
}

std::string VulkanEngine::getVersionString(uint32_t versionBitmask) {
    char versionString[128];

    uint32_t uMajorAPIVersion = versionBitmask >> 22;
    uint32_t uMinorAPIVersion = ((versionBitmask << 10) >> 10) >> 12;
    uint32_t uPatchAPIVersion = (versionBitmask << 20) >> 20;

    int majorAPIVersion = uMajorAPIVersion;
    int minorAPIVersion = uMinorAPIVersion;
    int patchAPIVersion = uPatchAPIVersion;

    sprintf(versionString, "%d.%d.%d", majorAPIVersion, minorAPIVersion, patchAPIVersion);

    return versionString;
}

void VulkanEngine::init() {
    getInstanceExtensions();
    getInstanceLayers();

    createInstance();
    enumeratePhysicalDevices();
    getPhysicalDevicePropertiesAndFeatures();
    createLogicalDevice();
    createSyncMeans();
    getDeviceLayers();
    loadMesh();
    createAllTextures();
    createAllBuffers();
    getQueue();
    getQueueFamilyPresentationSupport();
    createSurface(); // create surface
    createSwapchain();  // create swapchain
    getSupportedDepthFormat();
    createDepthImageAndImageview(); // create depth image and depth imageview (depth-stencil attachment)
    createSwapchainImageViews(); // create swapchain imageviews (framebuffer color attachment)
    createRenderpass(); // create renderpass
    createFramebuffers(); // create framebuffer
    createVertexGraphicsShaderModule(); // create vertex shader
    createFragmentGraphicsShaderModule(); // create fragment shader
//    createGeometryGraphicsShaderModule(); // create geometry shader
    createPipelineAndDescriptorSetsLayout(); // create pipeline layout
    createGraphicsPipeline(); //create pipeline
    createRenderCommandPool(); // create render commandpool
    createDescriptorPool(); // create descriptorpool
    createDescriptorSets();
    setupTimer();

    inited = true;
}

void VulkanEngine::setupTimer() {
    // get ticks per second
    QueryPerformanceFrequency(&frequency);

// start timer
    QueryPerformanceCounter(&t1);
}

void VulkanEngine::getSupportedDepthFormat() {

    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<VkFormat> depthFormats = {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
    };

    for (auto& format : depthFormats)
    {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(physicalDevices[0], format, &formatProps);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            depthFormat = format;
            return;
        }
    }

    throw VulkanException("Couldn't find depth stencil supported image format to create with.");
}

void VulkanEngine::draw() {
    if (!inited)
        return;

    uint32_t drawableImageIndex = acquireNextFramebufferImageIndex();
    render(drawableImageIndex);
    present(drawableImageIndex);
}

void VulkanEngine::createInstance() {
    appInfo.apiVersion = VK_API_VERSION_1_0;
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pApplicationName = "VulkanEngine Test";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "VulkanEngine Engine";
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;

    std::vector<const char*> layerNames = { "VK_LAYER_LUNARG_standard_validation", "VK_LAYER_LUNARG_monitor" };
    std::vector<const char*> extensionNames = { "VK_KHR_surface", "VK_KHR_win32_surface" };

    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = nullptr;
    instanceCreateInfo.enabledExtensionCount = 2;
#ifndef NDEBUG
    instanceCreateInfo.enabledLayerCount = 2;
#else
    instanceCreateInfo.enabledLayerCount = 0;
#endif
    instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
    instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
    instanceCreateInfo.flags = 0;

    instanceCreateInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) == VK_SUCCESS) {
        std::cout << "Instance created successfully.\n";
    }
    else {
        throw VulkanException("Instance creation failed.");
    }
}

void VulkanEngine::getInstanceLayers() {
    vkEnumerateInstanceLayerProperties(&layerPropertiesCount, nullptr);

    std::cout << "Number of Instance Layers available to physical device:\t" << std::to_string(layerPropertiesCount) << std::endl;

    layerProperties.resize(layerPropertiesCount);

    vkEnumerateInstanceLayerProperties(&layerPropertiesCount, layerProperties.data());

    std::cout << "Instance Layers:\n";

    for (int i = 0; i < layerPropertiesCount; i++) {
        std::cout << "\tLayer #" << std::to_string(i) << std::endl;
        std::cout << "\t\tName:\t" << layerProperties[i].layerName << std::endl;
        std::cout << "\t\tSpecification Version:\t" << getVersionString(layerProperties[i].specVersion) << std::endl;
        std::cout << "\t\tImplementation Version:\t" << layerProperties[i].implementationVersion << std::endl;
        std::cout << "\t\tDescription:\t" << layerProperties[i].description << std::endl;
    }
}

void VulkanEngine::createAllTextures() {
    uint32_t lastCoveredSize = 0;

    colorTexturesBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
    normalTexturesBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
    specTexturesBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        uint32_t requiredPadding = 0;

        VkDeviceSize colorTextureMemoryOffset;
        VkDeviceSize colorTextureMemorySize;
        VkDeviceSize normalTextureMemoryOffset;
        VkDeviceSize normalTextureMemorySize;
        VkDeviceSize specTextureMemoryOffset;
        VkDeviceSize specTextureMemorySize;

        VkMemoryRequirements colorTextureMemoryRequirement = createTexture(colorTextureImages + meshIndex);
        VkMemoryRequirements normalTextureMemoryRequirement = createTexture(normalTextureImages + meshIndex);
        VkMemoryRequirements specTextureMemoryRequirement = createTexture(specTextureImages + meshIndex);

        if (meshIndex == 0) {
            colorTextureMemoryOffset = 0;	colorTexturesBindOffsets[meshIndex] = colorTextureMemoryOffset;
            colorTextureMemorySize = colorTextureMemoryRequirement.size;

            lastCoveredSize += colorTextureMemoryOffset + colorTextureMemorySize;
        }
        else {
            requiredPadding = colorTextureMemoryRequirement.alignment - (lastCoveredSize % colorTextureMemoryRequirement.alignment);

            if (requiredPadding == colorTextureMemoryRequirement.alignment)
                requiredPadding = 0;

            colorTextureMemoryOffset = lastCoveredSize + requiredPadding;	colorTexturesBindOffsets[meshIndex] = colorTextureMemoryOffset;
            colorTextureMemorySize = colorTextureMemoryRequirement.size;

            lastCoveredSize += requiredPadding + colorTextureMemorySize;
        }

        requiredPadding = normalTextureMemoryRequirement.alignment - (lastCoveredSize % normalTextureMemoryRequirement.alignment);

        if (requiredPadding == normalTextureMemoryRequirement.alignment)
            requiredPadding = 0;

        normalTextureMemoryOffset = lastCoveredSize + requiredPadding;   normalTexturesBindOffsets[meshIndex] = normalTextureMemoryOffset;
        normalTextureMemorySize = normalTextureMemoryRequirement.size;

        lastCoveredSize += requiredPadding + normalTextureMemorySize;

        requiredPadding = specTextureMemoryRequirement.alignment - (lastCoveredSize % specTextureMemoryRequirement.alignment);

        if (requiredPadding == specTextureMemoryRequirement.alignment)
            requiredPadding = 0;

        specTextureMemoryOffset = lastCoveredSize + requiredPadding;     specTexturesBindOffsets[meshIndex] = specTextureMemoryOffset;
        specTextureMemorySize = specTextureMemoryRequirement.size;

        lastCoveredSize += requiredPadding + specTextureMemorySize;

    }

    VkDeviceSize totalRequiredMemorySize = lastCoveredSize;

    VkMemoryAllocateInfo uniMemoryAllocateInfo = {};
    uniMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniMemoryAllocateInfo.pNext = nullptr;
    uniMemoryAllocateInfo.allocationSize = totalRequiredMemorySize;
    uniMemoryAllocateInfo.memoryTypeIndex = hostVisibleDeviceLocalMemoryTypeIndex;

    VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &uniMemoryAllocateInfo, nullptr, &uniTexturesMemory));

    void *mappedMemory = NULL;

    VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], uniTexturesMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        uint16_t fileNumber = meshIndex;

        VkImageSubresource textureImageSubresource = {};
        VkSubresourceLayout subresourceLayout;

        textureImageSubresource.arrayLayer = 0;
        textureImageSubresource.mipLevel = 0;
        textureImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkGetImageSubresourceLayout(logicalDevices[0], colorTextureImages[meshIndex], &textureImageSubresource, &subresourceLayout);

        ILuint imgName = ilGenImage();

        ilBindImage(imgName);

        std::string colorFileName(texturesDirectoryPath);

        colorFileName.append("t");
        colorFileName.append(std::to_string(fileNumber));
        colorFileName.append("c.png");

        if (ilLoadImage(colorFileName.c_str()) != IL_TRUE)
            std::cerr << "DevIL: Couldn't load texture file '" << colorFileName.c_str() << "'." << std::endl;

        void *pTextureData = ilGetData();

        for (int i = 0; i < 1024; i++) {
            uint16_t rowPadding = -1;

            rowPadding = subresourceLayout.rowPitch - ((4 * 1024) % subresourceLayout.rowPitch);

            if (rowPadding == subresourceLayout.rowPitch) rowPadding = 0;

            memcpy((byte*)mappedMemory + (i * (1024 * 4 + rowPadding)) + colorTexturesBindOffsets[meshIndex], (byte*)pTextureData + (i * 1024 * 4), 1024 * 4);
        }

        ilDeleteImage(imgName);

        textureImageSubresource.arrayLayer = 0;
        textureImageSubresource.mipLevel = 0;
        textureImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkGetImageSubresourceLayout(logicalDevices[0], normalTextureImages[meshIndex], &textureImageSubresource, &subresourceLayout);

        imgName = ilGenImage();

        ilBindImage(imgName);

        std::string normalFileName(texturesDirectoryPath);

        normalFileName.append("t");
        normalFileName.append(std::to_string(fileNumber));
        normalFileName.append("n.png");

        if (ilLoadImage(normalFileName.c_str()) != IL_TRUE)
            std::cerr << "DevIL: Couldn't load texture file '" << normalFileName.c_str() << "'." << std::endl;

        pTextureData = ilGetData();

        for (int i = 0; i < 1024; i++) {
            uint16_t rowPadding = -1;

            rowPadding = subresourceLayout.rowPitch - ((4 * 1024) % subresourceLayout.rowPitch);

            if (rowPadding == subresourceLayout.rowPitch) rowPadding = 0;

            memcpy((byte*)mappedMemory + (i * (1024 * 4 + rowPadding)) + normalTexturesBindOffsets[meshIndex], (byte*)pTextureData + (i * 1024 * 4), 1024 * 4);
        }

        ilDeleteImage(imgName);

        textureImageSubresource.arrayLayer = 0;
        textureImageSubresource.mipLevel = 0;
        textureImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkGetImageSubresourceLayout(logicalDevices[0], specTextureImages[meshIndex], &textureImageSubresource, &subresourceLayout);

        imgName = ilGenImage();

        ilBindImage(imgName);

        std::string specFileName(texturesDirectoryPath);

        specFileName.append("t");
        specFileName.append(std::to_string(fileNumber));
        specFileName.append("s.png");

        if (ilLoadImage(specFileName.c_str()) != IL_TRUE)
            std::cerr << "DevIL: Couldn't load texture file '" << specFileName.c_str() << "'." << std::endl;

        pTextureData = ilGetData();

        for (int i = 0; i < 1024; i++) {
            uint16_t rowPadding = -1;

            rowPadding = subresourceLayout.rowPitch - ((4 * 1024) % subresourceLayout.rowPitch);

            if (rowPadding == subresourceLayout.rowPitch) rowPadding = 0;

            memcpy((byte*)mappedMemory + (i * (1024 * 4 + rowPadding)) + specTexturesBindOffsets[meshIndex], (byte*)pTextureData + (i * 1024 * 4), 1024 * 4);
        }

        ilDeleteImage(imgName);
    }

    vkUnmapMemory(logicalDevices[0], uniTexturesMemory);

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], colorTextureImages[meshIndex], uniTexturesMemory, colorTexturesBindOffsets[meshIndex]));
        VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], normalTextureImages[meshIndex], uniTexturesMemory, normalTexturesBindOffsets[meshIndex]));
        VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], specTextureImages[meshIndex], uniTexturesMemory, specTexturesBindOffsets[meshIndex]));
    }

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        uint32_t requiredPadding = 0;

        VkDeviceSize colorTextureMemoryOffset;
        VkDeviceSize colorTextureMemorySize;
        VkDeviceSize normalTextureMemoryOffset;
        VkDeviceSize normalTextureMemorySize;
        VkDeviceSize specTextureMemoryOffset;
        VkDeviceSize specTextureMemorySize;

        createTextureView(colorTextureViews + meshIndex, colorTextureImages[meshIndex]);
        createTextureView(normalTextureViews + meshIndex, normalTextureImages[meshIndex]);
        createTextureView(specTextureViews + meshIndex, specTextureImages[meshIndex]);
    }
}

void VulkanEngine::getDeviceLayers() {
    vkEnumerateDeviceLayerProperties(physicalDevices[0], &layerPropertiesCount, nullptr);

    std::cout << "Number of Device Layers available to physical device:\t" << std::to_string(layerPropertiesCount) << std::endl;

    layerProperties.resize(layerPropertiesCount);

    vkEnumerateDeviceLayerProperties(physicalDevices[0], &layerPropertiesCount, layerProperties.data());

    std::cout << "Device Layers:\n";

    for (int i = 0; i < layerPropertiesCount; i++) {
        std::cout << "\tLayer #" << std::to_string(i) << std::endl;
        std::cout << "\t\tName:\t" << layerProperties[i].layerName << std::endl;
        std::cout << "\t\tSpecification Version:\t" << getVersionString(layerProperties[i].specVersion) << std::endl;
        std::cout << "\t\tImplementation Version:\t" << layerProperties[i].implementationVersion << std::endl;
        std::cout << "\t\tDescription:\t" << layerProperties[i].description << std::endl;
    }
}

void VulkanEngine::enumeratePhysicalDevices() {
    if (vkEnumeratePhysicalDevices(instance, &numberOfSupportedDevices, physicalDevices) == VK_SUCCESS) {
        std::cout << "Physical device enumeration succeeded, first available device selected for logical device creation.\n";
    }
    else {
        throw VulkanException("Physical device enumeration failed.");
    }
}

void VulkanEngine::getPhysicalDevicePropertiesAndFeatures() {
    vkGetPhysicalDeviceProperties(physicalDevices[0], &deviceProperties);

    std::cout << "Highest Supported VulkanEngine Version:\t" << getVersionString(deviceProperties.apiVersion) << std::endl;

    vkGetPhysicalDeviceFeatures(physicalDevices[0], &supportedDeviceFeatures);

    std::cout << "Supports Tesselation Shader Feature:\t" << ((supportedDeviceFeatures.tessellationShader) ? ("Yes") : ("No")) << std::endl;

    std::cout << "Maximum Supported Image Size:\t" << deviceProperties.limits.maxImageDimension1D << "x" << deviceProperties.limits.maxImageDimension2D << "x" << deviceProperties.limits.maxImageDimension3D << std::endl;

    std::cout << "Minimum Memory Map Alignment:\t" << deviceProperties.limits.minMemoryMapAlignment << std::endl;

    std::cout << "Max Local Working Group Size: " << deviceProperties.limits.maxComputeWorkGroupSize[0] << "x" << deviceProperties.limits.maxComputeWorkGroupSize[1] << "x" << deviceProperties.limits.maxComputeWorkGroupSize[2] << std::endl;

    std::cout << "Max Total Working Group Invocations: " << deviceProperties.limits.maxComputeWorkGroupInvocations << std::endl;

    std::cout << "Max bound descriptor sets in a pipeline layout: " << deviceProperties.limits.maxBoundDescriptorSets << std::endl;

    std::cout << "Minimum Uniform Buffer offest alignment: " << deviceProperties.limits.minUniformBufferOffsetAlignment << std::endl;

    std::cout << "Maximum Texel Buffer elements: " << deviceProperties.limits.maxTexelBufferElements << std::endl;

    std::cout << "Maximum Subpass Color Attachments: " << deviceProperties.limits.maxColorAttachments << std::endl;

    std::cout << "Maximum Framebuffer Width: " << deviceProperties.limits.maxFramebufferWidth << std::endl;

    std::cout << "Maximum Framebuffer Height: " << deviceProperties.limits.maxFramebufferHeight << std::endl;

    std::cout << "Maximum Framebuffer Layers: " << deviceProperties.limits.maxFramebufferLayers << std::endl;

    std::cout << "Maximum Vertex Input bindings: " << deviceProperties.limits.maxVertexInputBindings << std::endl;

    std::cout << "Maximum Vertex Input Attributes: " << deviceProperties.limits.maxVertexInputAttributes << std::endl;

    std::cout << "Maximum Vertex Input Binding Stride: " << deviceProperties.limits.maxVertexInputBindingStride << std::endl;

    std::cout << "Maximum Vertex Input Attribute Offset: " << deviceProperties.limits.maxVertexInputAttributeOffset << std::endl;

    std::cout << "Maximum Viewports: " << deviceProperties.limits.maxViewports << std::endl;

    std::cout << "Minimum Line Width: " << deviceProperties.limits.lineWidthRange[0] << std::endl;

    std::cout << "Maximum Line Width: " << deviceProperties.limits.lineWidthRange[1] << std::endl;

    std::cout << "Line Width Granularity: " << deviceProperties.limits.lineWidthGranularity << std::endl;

    std::cout << "Max Index Value: " << deviceProperties.limits.maxDrawIndexedIndexValue << std::endl;

    std::cout << "Max Geometry Produced Vertices: " << deviceProperties.limits.maxGeometryOutputVertices << std::endl;

    std::cout << "Max Geometry Produced Components: " << deviceProperties.limits.maxGeometryOutputComponents << std::endl;

    std::cout << "Max Geometry Total Produced Components: " << deviceProperties.limits.maxGeometryTotalOutputComponents << std::endl;

    std::cout << "Max Geometry Consumed Components: " << deviceProperties.limits.maxGeometryInputComponents << std::endl;

    vkGetPhysicalDeviceMemoryProperties(physicalDevices[0], &deviceMemoryProperties);

    memoryTypeCount = deviceMemoryProperties.memoryTypeCount;

    std::cout << "Memory Type count: " << memoryTypeCount << std::endl;

    std::string flagsString;

    for (int i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
        flagsString = "";

        int heapIndex = deviceMemoryProperties.memoryTypes[i].heapIndex;

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0 && (deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
            hostInvisibleDeviceLocalMemoryTypeIndex = i;

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 && (deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
            hostVisibleDeviceLocalMemoryTypeIndex = i;

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
            deviceLocalMemoryTypeIndex = i;

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            flagsString += "DEVICE_LOCAL ";
        }

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
            flagsString += "HOST_VISIBLE ";
        }

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
            flagsString += "HOST_COHERENT ";
        }

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
            flagsString += "HOST_CACHED ";
        }

        if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0) {
            flagsString += "LAZILY_ALLOCATED ";
        }


        std::cout << "Memory type " << i << ":\n";
        std::cout << "\tFlags: " << flagsString << std::endl;
        std::cout << "\tHeap Index: " << heapIndex << std::endl;
        std::cout << "\tHeap Size: " << ((float)deviceMemoryProperties.memoryHeaps[heapIndex].size) / (1024 * 1024 * 1024) << "GB" << std::endl;

        //if ((((deviceMemoryProperties.memoryTypes[i].propertyFlags << (31 - VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) >> (31 - VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) >> VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
        totalHeapMemorySize += deviceMemoryProperties.memoryHeaps[heapIndex].size;
    }

    if (hostVisibleDeviceLocalMemoryTypeIndex == -1)
        throw VulkanException("No desired memory type found.");



    std::cout << "\nTotal Heaps Size:\t" << std::to_string((float)totalHeapMemorySize / (1024 * 1024 * 1024)) << "GB" << std::endl;
}

void VulkanEngine::terminate() {
    this->terminating = true;
    //if (*ppUnstableInstance_img != NULL)
    //	instanceToTerminate = *ppUnstableInstance_img;
    //else
    //Shutting Down
    //releasing memory
    delete deviceQueueCreateInfo.pQueuePriorities;


    //Terminte other threads
    //Working current thread possible command buffer generation through flags

    std::cout << "Waiting for logical device to return...\n";

    vkDeviceWaitIdle(logicalDevices[0]);


    destroySyncMeans();

    vkDestroySampler(logicalDevices[0], textureSampler, nullptr);

    std::cout << "Texture Sampler destroyed.\n";

    for (uint16_t i = 0; i < cachedScene->mNumMeshes; i++) {
        vkDestroyBuffer(logicalDevices[0], uniformBuffers[i], nullptr);
        std::cout << "Buffer destroyed.\n";

        vkDestroyBuffer(logicalDevices[0], vertexBuffers[i], nullptr);
        std::cout << "Buffer destroyed.\n";

        vkDestroyBuffer(logicalDevices[0], indexBuffers[i], nullptr);
        std::cout << "Buffer destroyed.\n";
    }

    vkFreeMemory(logicalDevices[0], uniBuffersMemory, nullptr);
    std::cout << "Buffers Memory released.\n";

    for (uint16_t i = 0; i < cachedScene->mNumMeshes; i++) {
        vkDestroyImageView(logicalDevices[0], specTextureViews[i], nullptr);
        std::cout << "Spec ImageView destroyed.\n";

        vkDestroyImage(logicalDevices[0], specTextureImages[i], nullptr);
        std::cout << "Spec Image destroyed.\n";

        vkDestroyImageView(logicalDevices[0], normalTextureViews[i], nullptr);
        std::cout << "Normal ImageView destroyed.\n";

        vkDestroyImage(logicalDevices[0], normalTextureImages[i], nullptr);
        std::cout << "Normal Image destroyed.\n";

        vkDestroyImageView(logicalDevices[0], colorTextureViews[i], nullptr);
        std::cout << "Color Texture ImageView destroyed.\n";

        vkDestroyImage(logicalDevices[0], colorTextureImages[i], nullptr);
        std::cout << "Color Texture Image destroyed.\n";
    }

    vkFreeMemory(logicalDevices[0], uniTexturesMemory, nullptr);
    std::cout << "Textures Memory released.\n";



    vkDestroyDescriptorSetLayout(logicalDevices[0], graphicsDescriptorSetLayout, nullptr);
    std::cout << "DescriptorSet Layout destroyed successfully." << std::endl;


    vkDestroyDescriptorPool(logicalDevices[0], descriptorPool, nullptr);
    std::cout << "Descriptor Set Pool destroyed successfully." << std::endl;


    vkDestroyImageView(logicalDevices[0], depthImageView, nullptr);
    std::cout << "Depth-Stencil ImageView destroyed.\n";

    vkDestroyImage(logicalDevices[0], depthImage, nullptr);
    std::cout << "Depth-Stencil Image destroyed.\n";

    vkFreeMemory(logicalDevices[0], depthImageMemory, nullptr);
    std::cout << "Depth-Stencil Image Memory freed.\n";


    for (int i = 0; i < swapchainImagesCount; i++)
        vkDestroyImageView(logicalDevices[0], swapchainImageViews[i], nullptr);

    std::cout << "Swapchain ImageViews destroyed successfully." << std::endl;

    vkDestroyCommandPool(logicalDevices[0], renderCommandPool, nullptr);

    std::cout << "Command Pool destroyed successfully." << std::endl;


    vkDestroyShaderModule(logicalDevices[0], graphicsVertexShaderModule, nullptr);
    vkDestroyShaderModule(logicalDevices[0], graphicsFragmentShaderModule, nullptr);

    std::cout << "Shader modules destroyed successfully." << std::endl;

    vkDestroyPipelineLayout(logicalDevices[0], graphicsPipelineLayout, nullptr);
    std::cout << "Graphics Pipeline layout destroyed successfully." << std::endl;

    vkDestroyRenderPass(logicalDevices[0], renderPass, nullptr);
    std::cout << "Renderpass destroyed successfully." << std::endl;

    vkDestroyPipeline(logicalDevices[0], graphicsPipeline, nullptr);
    std::cout << "Pipeline destroyed successfully." << std::endl;

    for (int i = 0; i < swapchainImagesCount; i++)
        vkDestroyFramebuffer(logicalDevices[0], framebuffers[i], nullptr);
    std::cout << "Framebuffer(s) destroyed successfully." << std::endl;


    vkDestroySwapchainKHR(logicalDevices[0], swapchain, nullptr);
    std::cout << "Swapchain destroyed successfully." << std::endl;


    vkDestroyDevice(logicalDevices[0], nullptr);
    std::cout << "Logical Device destroyed.\n";

    vkDestroyInstance(instance, nullptr);
    std::cout << "Instance destroyed.\n";

//    getchar();
}

void VulkanEngine::createLogicalDevice() {
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[0], &numQueueFamilies, nullptr);

    std::cout << "Number of Queue Families:\t" << std::to_string(numQueueFamilies) << std::endl;



    queueFamilyProperties.resize(numQueueFamilies);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[0], &numQueueFamilies, queueFamilyProperties.data());

    for (int i = 0; i < numQueueFamilies; i++) {
        std::string queueFlagsString = "";

        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT != 0) {
            queueFlagsString += "GRAPHICS ";
        }

        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT != 0) {
            queueFlagsString += "COMPUTE ";
        }

        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT != 0) {
            queueFlagsString += "TRANSFER ";
        }

        if (queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT != 0) {
            queueFlagsString += "SPARSE-BINDING ";
        }


        std::cout << std::endl;
        std::cout << "Queue Family " << i << ":\n";
        std::cout << "\t Queue Family Queue Amount: " << queueFamilyProperties[i].queueCount << std::endl;
        std::cout << "\t Queue Family Min Image Transfer Granularity Width: " << queueFamilyProperties[i].minImageTransferGranularity.width << std::endl;
        std::cout << "\t Queue Family Min Image Transfer Granularity Height: " << queueFamilyProperties[i].minImageTransferGranularity.height << std::endl;
        std::cout << "\t Queue Family Min Image Transfer Granularity Depth: " << queueFamilyProperties[i].minImageTransferGranularity.depth << std::endl;
        std::cout << "\t Queue Family Flags: " << queueFlagsString << std::endl;
        std::cout << "\t Queue Family Timestamp Valid Bits: " << queueFamilyProperties[i].timestampValidBits << "Bits" << std::endl;


        if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && graphicQueueFamilyIndex == -1) {
            graphicQueueFamilyIndex = i;
            graphicQueueFamilyNumQueue = queueFamilyProperties[i].queueCount;
        }

        queueCount += queueFamilyProperties[i].queueCount;
    }

    if (graphicQueueFamilyIndex == -1 || graphicQueueFamilyNumQueue == 0) {
        throw VulkanException("Cannot find queue family that supports graphical rendering.");
    }

    std::cout << "Total Device Queue Count:\t" << std::to_string(queueCount) << std::endl;


    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.queueFamilyIndex = graphicQueueFamilyIndex;
    deviceQueueCreateInfo.queueCount = graphicQueueFamilyNumQueue;
    float *queuePriorities = new float[graphicQueueFamilyNumQueue];

    for (int i = 0; i < graphicQueueFamilyNumQueue; i++)
        queuePriorities[i] = 1.0f;

    deviceQueueCreateInfo.pQueuePriorities = new const float[1]{ 1.0f };


    std::vector<const char*> extensionNames = { "VK_KHR_swapchain" };

    desiredDeviceFeatures.geometryShader = VK_TRUE;

    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceCreateInfo.flags = 0;
    logicalDeviceCreateInfo.pNext = nullptr;
    logicalDeviceCreateInfo.queueCreateInfoCount = 1;
    logicalDeviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
    logicalDeviceCreateInfo.enabledExtensionCount = 1;
    logicalDeviceCreateInfo.enabledLayerCount = 0;
    logicalDeviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
    logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
    logicalDeviceCreateInfo.pEnabledFeatures = &desiredDeviceFeatures;


    if (vkCreateDevice(physicalDevices[0], &logicalDeviceCreateInfo, nullptr, logicalDevices) == VK_SUCCESS) {
        std::cout << "Logical device creation succeeded.\n";
    }
    else {
        throw VulkanException("Logical Device creation failed.\n");
    }
}

void VulkanEngine::createAllBuffers() {
    uint32_t lastCoveredSize = 0;

    uniformBuffersBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
    vertexBuffersBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
    indexBuffersBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        uint32_t requiredPadding = 0;

        VkDeviceSize uniformBufferMemoryOffset;
        VkDeviceSize uniformBufferMemorySize;
        VkDeviceSize vertexBufferMemoryOffset;
        VkDeviceSize vertexBufferMemorySize;
        VkDeviceSize indexBufferMemoryOffset;
        VkDeviceSize indexBufferMemorySize;

        VkMemoryRequirements uniformBufferMemoryRequirements = createBuffer(uniformBuffers + meshIndex, totalUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
        VkMemoryRequirements vertexBufferMemoryRequirements = createBuffer(vertexBuffers + meshIndex, vertexBuffersSizes[meshIndex], VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        VkMemoryRequirements indexBufferMemoryRequirements = createBuffer(indexBuffers + meshIndex, indexBuffersSizes[meshIndex], VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        if (meshIndex == 0) {
            uniformBufferMemoryOffset = 0;	uniformBuffersBindOffsets[meshIndex] = uniformBufferMemoryOffset;
            uniformBufferMemorySize = totalUniformBufferSize;

            lastCoveredSize += uniformBufferMemoryOffset + uniformBufferMemorySize;
        }
        else {
            requiredPadding = uniformBufferMemoryRequirements.alignment - (lastCoveredSize % uniformBufferMemoryRequirements.alignment);

            if (requiredPadding == uniformBufferMemoryRequirements.alignment)
                requiredPadding = 0;

            uniformBufferMemoryOffset = lastCoveredSize + requiredPadding;	uniformBuffersBindOffsets[meshIndex] = uniformBufferMemoryOffset;
            uniformBufferMemorySize = totalUniformBufferSize;

            lastCoveredSize += requiredPadding + uniformBufferMemorySize;
        }

        requiredPadding = vertexBufferMemoryRequirements.alignment - (lastCoveredSize % vertexBufferMemoryRequirements.alignment);

        if (requiredPadding == vertexBufferMemoryRequirements.alignment)
            requiredPadding = 0;

        vertexBufferMemoryOffset = lastCoveredSize + requiredPadding;   vertexBuffersBindOffsets[meshIndex] = vertexBufferMemoryOffset;
        vertexBufferMemorySize = vertexBuffersSizes[meshIndex];

        lastCoveredSize += requiredPadding + vertexBufferMemorySize;

        requiredPadding = indexBufferMemoryRequirements.alignment - (lastCoveredSize % indexBufferMemoryRequirements.alignment);

        if (requiredPadding == indexBufferMemoryRequirements.alignment)
            requiredPadding = 0;

        indexBufferMemoryOffset = lastCoveredSize + requiredPadding;     indexBuffersBindOffsets[meshIndex] = indexBufferMemoryOffset;
        indexBufferMemorySize = indexBuffersSizes[meshIndex];

        lastCoveredSize += requiredPadding + indexBufferMemorySize;

    }

    VkDeviceSize totalRequiredMemorySize = lastCoveredSize;

    VkMemoryAllocateInfo uniMemoryAllocateInfo = {};
    uniMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    uniMemoryAllocateInfo.pNext = nullptr;
    uniMemoryAllocateInfo.allocationSize = totalRequiredMemorySize;
    uniMemoryAllocateInfo.memoryTypeIndex = hostVisibleDeviceLocalMemoryTypeIndex;

    VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &uniMemoryAllocateInfo, nullptr, &uniBuffersMemory));

    void *mappedMemory = NULL;

    VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], uniBuffersMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        float xRotation = (3.1415926536f / 180.0f) * 180;
        float yRotation = (3.1415926536f / 180.0f) * 90;

        ModelMatrix modelMatrix;
        float modelMatrices[3][16];

        float scale = 6.6f;

        modelMatrices[0][0] = 1.0f;		modelMatrices[0][4] = 0.0f;			modelMatrices[0][8] = 0.0f;			modelMatrices[0][12] = 0.0f;
        modelMatrices[0][1] = 0.0f;		modelMatrices[0][5] = cosf(xRotation);	modelMatrices[0][9] = -sinf(xRotation);	modelMatrices[0][13] = 0.0f;
        modelMatrices[0][2] = 0.0f;		modelMatrices[0][6] = sinf(xRotation);	modelMatrices[0][10] = cosf(xRotation);	modelMatrices[0][14] = 0.0f;
        modelMatrices[0][3] = 0.0f;		modelMatrices[0][7] = 0.0f;			modelMatrices[0][11] = 0.0f;			modelMatrices[0][15] = 1.0f;

        modelMatrices[1][0] = cosf(yRotation);	modelMatrices[1][4] = 0.0f;	modelMatrices[1][8] = sinf(yRotation);	modelMatrices[1][12] = 0.0f;
        modelMatrices[1][1] = 0.0f;				modelMatrices[1][5] = 1.0f;	modelMatrices[1][9] = 0.0f;				modelMatrices[1][13] = 0.0f;
        modelMatrices[1][2] = -sinf(yRotation);	modelMatrices[1][6] = 0.0f;	modelMatrices[1][10] = cosf(yRotation);	modelMatrices[1][14] = 0.0f;
        modelMatrices[1][3] = 0.0f;				modelMatrices[1][7] = 0.0f;	modelMatrices[1][11] = 0.0f;				modelMatrices[1][15] = 1.0f;

        modelMatrices[2][0] = scale;			modelMatrices[2][4] = 0.0f;		modelMatrices[2][8] = 00.0f;			modelMatrices[2][12] = 0.0f;
        modelMatrices[2][1] = 0.0f;				modelMatrices[2][5] = scale;	modelMatrices[2][9] = 0.0f;				modelMatrices[2][13] = 0.0f;
        modelMatrices[2][2] = 0.0f;				modelMatrices[2][6] = 0.0f;		modelMatrices[2][10] = scale;			modelMatrices[2][14] = 0.0f;
        modelMatrices[2][3] = 0.0f;				modelMatrices[2][7] = 0.0f;		modelMatrices[2][11] = 0.0f;			modelMatrices[2][15] = 1.0f;

        float temp[16];

        multiplyMatrix<float>(temp, modelMatrices[2], modelMatrices[0]);

        multiplyMatrix<float>(modelMatrix.modelMatrix, temp, modelMatrices[1]);

        memcpy((byte*)mappedMemory + uniformBuffersBindOffsets[meshIndex], &modelMatrix, totalUniformBufferSize);

        memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryFlushRange.pNext = nullptr;
        memoryFlushRange.size = totalUniformBufferSize;
        memoryFlushRange.offset = uniformBuffersBindOffsets[meshIndex];
        memoryFlushRange.memory = uniBuffersMemory;

        VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));

        memcpy((byte*)mappedMemory + vertexBuffersBindOffsets[meshIndex], sortedAttributes[meshIndex].data(), vertexBuffersSizes[meshIndex]);

        memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryFlushRange.pNext = nullptr;
        memoryFlushRange.size = vertexBuffersSizes[meshIndex];
        memoryFlushRange.offset = vertexBuffersBindOffsets[meshIndex];
        memoryFlushRange.memory = uniBuffersMemory;

        VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));

        memcpy((byte*)mappedMemory + indexBuffersBindOffsets[meshIndex], sortedIndices[meshIndex].data(), indexBuffersSizes[meshIndex]);

        memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryFlushRange.pNext = nullptr;
        memoryFlushRange.size = indexBuffersSizes[meshIndex];
        memoryFlushRange.offset = indexBuffersBindOffsets[meshIndex];
        memoryFlushRange.memory = uniBuffersMemory;

        VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
    }

    vkUnmapMemory(logicalDevices[0], uniBuffersMemory);

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], uniformBuffers[meshIndex], uniBuffersMemory, uniformBuffersBindOffsets[meshIndex]));
        VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], vertexBuffers[meshIndex], uniBuffersMemory, vertexBuffersBindOffsets[meshIndex]));
        VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], indexBuffers[meshIndex], uniBuffersMemory, indexBuffersBindOffsets[meshIndex]));
    }
}

void VulkanEngine::createDepthImageAndImageview() {

    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthFormat;
    imageCreateInfo.extent.width = swapchainCreateInfo.imageExtent.width;
    imageCreateInfo.extent.height = swapchainCreateInfo.imageExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.queueFamilyIndexCount = 0;
    imageCreateInfo.pQueueFamilyIndices = nullptr;

    VKASSERT_SUCCESS(vkCreateImage(logicalDevices[0], &imageCreateInfo, nullptr, &depthImage));

    VkMemoryRequirements depthImageMemoryRequirements;

    vkGetImageMemoryRequirements(logicalDevices[0], depthImage, &depthImageMemoryRequirements);

    VkMemoryAllocateInfo depthImageMemoryAllocateInfo = {  };

    depthImageMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    depthImageMemoryAllocateInfo.pNext = nullptr;
    depthImageMemoryAllocateInfo.allocationSize = depthImageMemoryRequirements.size;
    depthImageMemoryAllocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

    VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &depthImageMemoryAllocateInfo, nullptr, &depthImageMemory));

    VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], depthImage, depthImageMemory, 0));

    VkImageViewCreateInfo depthImageViewCreateInfo = {};

    depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    depthImageViewCreateInfo.pNext = NULL;
    depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    depthImageViewCreateInfo.format = depthFormat;
    depthImageViewCreateInfo.flags = 0;
    depthImageViewCreateInfo.image = depthImage;
    depthImageViewCreateInfo.subresourceRange = {};
    depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    depthImageViewCreateInfo.subresourceRange.levelCount = 1;
    depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    depthImageViewCreateInfo.subresourceRange.layerCount = 1;

    VKASSERT_SUCCESS(vkCreateImageView(logicalDevices[0], &depthImageViewCreateInfo, nullptr, &depthImageView));
}

void VulkanEngine::getPhysicalDeviceImageFormatProperties() {
    vkGetPhysicalDeviceFormatProperties(physicalDevices[0], imageFormat, &formatProperties);

    std::string bufferFormatFeatureFlagsString = "";
    std::string linearTilingFormatFeatureFlagsString = "";
    std::string optimalTilingFormatFeatureFlagsString = "";

#pragma region format features bits
    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT != 0) {
        bufferFormatFeatureFlagsString += "SAMPLED_IMAGED ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT != 0) {
        bufferFormatFeatureFlagsString += "SAMPLED_IMAGE_FILTER_LINEAR ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT != 0) {
        bufferFormatFeatureFlagsString += "STORAGE_IMAGE ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT != 0) {
        bufferFormatFeatureFlagsString += "STORAGE_IMAGE_ATOMIC ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT != 0) {
        bufferFormatFeatureFlagsString += "UNIFORM_TEXEL_BUFFER ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT != 0) {
        bufferFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT != 0) {
        bufferFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER_ATOMIC ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT != 0) {
        bufferFormatFeatureFlagsString += "VERTEX_BUFFER ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT != 0) {
        bufferFormatFeatureFlagsString += "COLOR_ATTACHMENT ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT != 0) {
        bufferFormatFeatureFlagsString += "COLOR_ATTACHMENT_BLEND ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT != 0) {
        bufferFormatFeatureFlagsString += "DEPTH_STENCIL_ATTACHMENT ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT != 0) {
        bufferFormatFeatureFlagsString += "BLIT_SRC ";
    }

    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT != 0) {
        bufferFormatFeatureFlagsString += "BLIT_DST ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "SAMPLED_IMAGED ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "SAMPLED_IMAGE_FILTER_LINEAR ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "STORAGE_IMAGE ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "STORAGE_IMAGE_ATOMIC ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "UNIFORM_TEXEL_BUFFER ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER_ATOMIC ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "VERTEX_BUFFER ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT_BLEND ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "DEPTH_STENCIL_ATTACHMENT ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "BLIT_SRC ";
    }

    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT != 0) {
        linearTilingFormatFeatureFlagsString += "BLIT_DST ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "SAMPLED_IMAGED ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "SAMPLED_IMAGE_FILTER_LINEAR ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "STORAGE_IMAGE ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "STORAGE_IMAGE_ATOMIC ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "UNIFORM_TEXEL_BUFFER ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER_ATOMIC ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "VERTEX_BUFFER ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT_BLEND ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "DEPTH_STENCIL_ATTACHMENT ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "BLIT_SRC ";
    }

    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT != 0) {
        optimalTilingFormatFeatureFlagsString += "BLIT_DST ";
    }
#pragma endregion

    std::cout << "Physical Device format support features pertaining to format " << imageFormatString << std::endl;
    std::cout << "\tBuffer format features flags: " << bufferFormatFeatureFlagsString << std::endl;
    std::cout << "\tLinear tiling format feature flags: " << linearTilingFormatFeatureFlagsString << std::endl;
    std::cout << "\tOptimal tiling format feature flags: " << optimalTilingFormatFeatureFlagsString << std::endl;

    VkResult result = vkGetPhysicalDeviceImageFormatProperties(physicalDevices[0], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, &imageFormatProperties);
    float maxResourceSizeGB;
    VkSampleCountFlags sampleCountFlags;
    uint32_t sampleCount;

    switch (result) {
        case VK_SUCCESS:
            std::cout << "Physical Device support extent pertaining to image format R8G8B8A8_UNORM (2D) with optimal tiling usable as source and destination of transfer commands, allowing image view creation off itself:\n";

            maxResourceSizeGB = ((float)imageFormatProperties.maxResourceSize) / (1024 * 1024 * 1024);

            std::cout << "\tMax Array Layers: " << imageFormatProperties.maxArrayLayers << std::endl;
            std::cout << "\tMax MimMap Levels: " << imageFormatProperties.maxMipLevels << std::endl;
            std::cout << "\tMax Resource Size: " << maxResourceSizeGB << "GB" << std::endl;
            std::cout << "\tMax Sample Count: " << imageFormatProperties.sampleCounts << std::endl;
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            throw VulkanException("Couldn't fetch image format properties, out of host memory.");
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            throw VulkanException("Couldn't fetch image format properties, out of device memory.");
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            throw VulkanException("Format not supported.");
            break;
    }
}

//void VulkanEngine Engine::createImageView() {
//	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//	imageViewCreateInfo.pNext = nullptr;
//	imageViewCreateInfo.flags = 0;
//	imageViewCreateInfo.image = depthImage[0];
//	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//	imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
//	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
//	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
//	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
//	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
//	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
//	imageViewCreateInfo.subresourceRange.levelCount = 1;
//	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
//	imageViewCreateInfo.subresourceRange.layerCount = 1;
//
//	imageViews = new VkImageView[1];
//
//	VkResult result = vkCreateImageView(logicalDevices[0], &imageViewCreateInfo, &imageViewCreationCallbacks, imageViews);
//
//	switch (result) {
//	case VK_SUCCESS:
//		std::cout << "Image View creation succeeded.\n";
//		break;
//	default:
//		throw VulkanException("Image View creation failed.");
//	}
//}

//void VulkanEngine Engine::allocateDeviceMemories() {
//	VkMemoryAllocateInfo deviceMemoryAllocateInfo;
//
//	for (uint16_t uniformBufferIndex = 0; uniformBufferIndex < cachedScene->mNumMeshes * 3; uniformBufferIndex += 3) {
//		deviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//		deviceMemoryAllocateInfo.pNext = nullptr;
//		deviceMemoryAllocateInfo.allocationSize = totalUniformBufferSize;
//		deviceMemoryAllocateInfo.memoryTypeIndex = hostVisibleDeviceLocalMemoryTypeIndex;
//
//		VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &deviceMemoryAllocateInfo, nullptr, bufferMemories + uniformBufferIndex + 0));
//
//
//		deviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//		deviceMemoryAllocateInfo.pNext = nullptr;
//		deviceMemoryAllocateInfo.allocationSize = vertexBufferSizes[uniformBufferIndex / 3];
//		deviceMemoryAllocateInfo.memoryTypeIndex = hostVisibleDeviceLocalMemoryTypeIndex;
//
//		VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &deviceMemoryAllocateInfo, nullptr, bufferMemories + uniformBufferIndex + 1) );
//
//		deviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//		deviceMemoryAllocateInfo.pNext = nullptr;
//		deviceMemoryAllocateInfo.allocationSize = indexBufferSizes[uniformBufferIndex / 3];
//		deviceMemoryAllocateInfo.memoryTypeIndex = hostVisibleDeviceLocalMemoryTypeIndex;
//
//		VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &deviceMemoryAllocateInfo, nullptr, bufferMemories + uniformBufferIndex + 2));
//	}
//}

//void VulkanEngine Engine::writeBuffers() {
//	void *mappedMemory;
//
//
//	for (uint16_t uniformBufferIndex = 0; uniformBufferIndex < cachedScene->mNumMeshes * 3; uniformBufferIndex += 3) {
//		VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex], 0, totalUniformBufferSize, 0, &mappedMemory));
//
//		float xRotation = (3.1415926536f / 180.0f) * 180;
//		float yRotation = (3.1415926536f / 180.0f) * 90;
//
//		ModelMatrix modelMatrix;
//		float rotationMatrices[2][16];
//		float translationMatrices[2][16];
//
//		rotationMatrices[0][0] = 1.0f;		rotationMatrices[0][4] = 0.0f;			rotationMatrices[0][8] = 0.0f;			rotationMatrices[0][12] = 0.0f;
//		rotationMatrices[0][1] = 0.0f;		rotationMatrices[0][5] = cosf(xRotation);	rotationMatrices[0][9] = -sinf(xRotation);	rotationMatrices[0][13] = 0.0f;
//		rotationMatrices[0][2] = 0.0f;		rotationMatrices[0][6] = sinf(xRotation);	rotationMatrices[0][10] = cosf(xRotation);	rotationMatrices[0][14] = 0.0f;
//		rotationMatrices[0][3] = 0.0f;		rotationMatrices[0][7] = 0.0f;			rotationMatrices[0][11] = 0.0f;			rotationMatrices[0][15] = 1.0f;
//
//		rotationMatrices[1][0] = cosf(yRotation);	rotationMatrices[1][4] = 0.0f;	rotationMatrices[1][8] = sinf(yRotation);	rotationMatrices[1][12] = 0.0f;
//		rotationMatrices[1][1] = 0.0f;				rotationMatrices[1][5] = 1.0f;	rotationMatrices[1][9] = 0.0f;				rotationMatrices[1][13] = 0.0f;
//		rotationMatrices[1][2] = -sinf(yRotation);	rotationMatrices[1][6] = 0.0f;	rotationMatrices[1][10] = cosf(yRotation);	rotationMatrices[1][14] = 0.0f;
//		rotationMatrices[1][3] = 0.0f;				rotationMatrices[1][7] = 0.0f;	rotationMatrices[1][11] = 0.0f;				rotationMatrices[1][15] = 1.0f;
//
//		multiplyMatrix<float>(modelMatrix.modelMatrix, rotationMatrices[0], rotationMatrices[1]);
//
//		memcpy(mappedMemory, &modelMatrix, totalUniformBufferSize);
//
//		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//		memoryFlushRange.pNext = nullptr;
//		memoryFlushRange.size = VK_WHOLE_SIZE;
//		memoryFlushRange.offset = 0;
//		memoryFlushRange.memory = bufferMemories[uniformBufferIndex];
//
//		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
//
//		vkUnmapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex]);
//
//
//		VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 1], 0, vertexBufferSizes[uniformBufferIndex / 3], 0, &mappedMemory));
//
//		memcpy(mappedMemory, sortedAttributes[uniformBufferIndex / 3].data(), vertexBufferSizes[uniformBufferIndex / 3]);
//
//		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//		memoryFlushRange.pNext = nullptr;
//		memoryFlushRange.size = VK_WHOLE_SIZE;
//		memoryFlushRange.offset = 0;
//		memoryFlushRange.memory = bufferMemories[uniformBufferIndex + 1];
//
//		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
//
//		vkUnmapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 1]);
//
//
//
//
//		VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 2], 0, indexBufferSizes[uniformBufferIndex / 3], 0, &mappedMemory));
//
//		memcpy(mappedMemory, sortedIndices[uniformBufferIndex / 3].data(), indexBufferSizes[uniformBufferIndex / 3]);
//
//		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//		memoryFlushRange.pNext = nullptr;
//		memoryFlushRange.size = VK_WHOLE_SIZE;
//		memoryFlushRange.offset = 0;
//		memoryFlushRange.memory = bufferMemories[uniformBufferIndex + 2];
//
//		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
//
//		vkUnmapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 2]);
//
//
//	}
//}
//
//void VulkanEngine Engine::bindBufferMemories() {
//
//	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
//		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], buffers[(meshIndex * 3) + 0], bufferMemories[(meshIndex * 3) + 0], 0));
//
//		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], buffers[(meshIndex * 3) + 1], bufferMemories[(meshIndex * 3) + 1], 0));
//
//		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], buffers[(meshIndex * 3) + 2], bufferMemories[(meshIndex * 3) + 2], 0));
//	}
//}

void VulkanEngine::createSparseImage() {
    sparseImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    sparseImageCreateInfo.pNext = nullptr;
    sparseImageCreateInfo.flags = VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
    sparseImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    sparseImageCreateInfo.format = VK_FORMAT_R32G32B32A32_UINT;
    sparseImageCreateInfo.extent.width = 4096;
    sparseImageCreateInfo.extent.height = 4096;
    sparseImageCreateInfo.extent.depth = 1;
    sparseImageCreateInfo.arrayLayers = 1;
    sparseImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    sparseImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    sparseImageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    sparseImageCreateInfo.mipLevels = imageFormatProperties.maxMipLevels;
    sparseImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sparseImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    sparseImageCreateInfo.queueFamilyIndexCount = 0;
    sparseImageCreateInfo.pQueueFamilyIndices = nullptr;

    sparseImages = new VkImage[1];

    VkResult result = vkCreateImage(logicalDevices[0], &sparseImageCreateInfo, nullptr, sparseImages);

    switch (result) {
        case VK_SUCCESS:
            std::cout << "Sparse Image (" << 2048 << "x" << 2048 << ", with different other parameters) created successfully.\n";
            break;
        default:
            throw VulkanException("Sparse Image creation failed.");
    }

    vkGetImageSparseMemoryRequirements(logicalDevices[0], sparseImages[0], &sparseMemoryRequirementsCount, nullptr);

    sparseImageMemoryRequirements = new VkSparseImageMemoryRequirements[sparseMemoryRequirementsCount];

    vkGetImageSparseMemoryRequirements(logicalDevices[0], sparseImages[0], &sparseMemoryRequirementsCount, sparseImageMemoryRequirements);

    std::cout << "Sparse Image Memory Requirements:\n";

    for (uint32_t i = 0; i < sparseMemoryRequirementsCount; i++) {
        VkSparseImageMemoryRequirements sparseImageMemoryRequirementsElement = sparseImageMemoryRequirements[i];

        std::string aspectMaskString = "";

        if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0) {
            aspectMaskString += "COLOR_BIT ";
        }

        if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) {
            aspectMaskString += "DEPTH_BIT ";
        }

        if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
            aspectMaskString += "STENCIL_BIT ";
        }

        if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) != 0) {
            aspectMaskString += "METADATA_BIT ";
        }

        std::cout << "\tApplied to following aspects: " << aspectMaskString << std::endl;

        std::cout << "\tImage Granularity Width: " << sparseImageMemoryRequirementsElement.formatProperties.imageGranularity.width << std::endl;
        std::cout << "\tImage Granularity Height: " << sparseImageMemoryRequirementsElement.formatProperties.imageGranularity.height << std::endl;
        std::cout << "\tImage Granularity Depth: " << sparseImageMemoryRequirementsElement.formatProperties.imageGranularity.depth << std::endl;

        std::string flagsString = "";

        if ((sparseImageMemoryRequirementsElement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0) {
            flagsString += "SINGLE_MIPTAIL ";
        }

        if ((sparseImageMemoryRequirementsElement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT) != 0) {
            flagsString += "ALIGNED_MIP_SIZE ";
        }

        if ((sparseImageMemoryRequirementsElement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT) != 0) {
            flagsString += "NONSTANDARD_BLOCK_SIZE ";
        }

        std::cout << "\tFlags: " << flagsString << std::endl;

        std::cout << "\tFirst Mip-Tail Level: " << sparseImageMemoryRequirementsElement.imageMipTailFirstLod << std::endl;
        std::cout << "\tFirst Mip-Tail Size: " << sparseImageMemoryRequirementsElement.imageMipTailSize << std::endl;
        std::cout << "\tFirst Mip-Tail Offset (in memory binding region): " << sparseImageMemoryRequirementsElement.imageMipTailOffset << std::endl;
        std::cout << "\tFirst Mip-Tail Stride (between deviant miptails of array): " << sparseImageMemoryRequirementsElement.imageMipTailStride << std::endl;

    }
}

void VulkanEngine::getPhysicalDeviceSparseImageFormatProperties() {
    VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    VkFormat format = VK_FORMAT_R16G16B16A16_UNORM;
    VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkImageType type = VK_IMAGE_TYPE_2D;


    vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevices[0], format, type, samplesCount, usageFlags, tiling, &physicalDeviceSparseImageFormatPropertiesCount, nullptr);

    physicalDeviceSparseImageFormatProperties = new VkSparseImageFormatProperties[physicalDeviceSparseImageFormatPropertiesCount];

    vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevices[0], format, type, samplesCount, usageFlags, tiling, &physicalDeviceSparseImageFormatPropertiesCount, physicalDeviceSparseImageFormatProperties);

    std::cout << "Physical Device support extent pertaining to sparse image format VK_FORMAT_R16G16B16A16_UNORM(2D) with optimal tiling usable as source and destination of transfer commands, as well as sampling, and allowing image view creation off itself, with one multisampling:\n";

    for (uint32_t i = 0; i < physicalDeviceSparseImageFormatPropertiesCount; i++) {

        std::string aspectMaskString = "";

        if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0) {
            aspectMaskString += "COLOR_BIT ";
        }

        if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) {
            aspectMaskString += "DEPTH_BIT ";
        }

        if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
            aspectMaskString += "STENCIL_BIT ";
        }

        if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) != 0) {
            aspectMaskString += "METADATA_BIT ";
        }

        std::cout << "\tApplied to following aspects: " << aspectMaskString << std::endl;

        std::cout << "\tImage Granularity Width: " << physicalDeviceSparseImageFormatProperties[i].imageGranularity.width << std::endl;
        std::cout << "\tImage Granularity Height: " << physicalDeviceSparseImageFormatProperties[i].imageGranularity.height << std::endl;
        std::cout << "\tImage Granularity Depth: " << physicalDeviceSparseImageFormatProperties[i].imageGranularity.depth << std::endl;

        std::string flagsString = "";

        if ((physicalDeviceSparseImageFormatProperties[i].flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0) {
            flagsString += "SINGLE_MIPTAIL ";
        }

        if ((physicalDeviceSparseImageFormatProperties[i].flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT) != 0) {
            flagsString += "ALIGNED_MIP_SIZE ";
        }

        if ((physicalDeviceSparseImageFormatProperties[i].flags & VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT) != 0) {
            flagsString += "NONSTANDARD_BLOCK_SIZE ";
        }

        std::cout << "\tFlags: " << flagsString << std::endl;
    }
}

void VulkanEngine::getQueue() {
    vkGetDeviceQueue(logicalDevices[0], graphicQueueFamilyIndex, 0, &queue);

    if (queue == VK_NULL_HANDLE) {
        throw "Couldn't obtain queue from device.";
    }
    else {
        std::cout << "Queue obtained successfully.\n";
    }
}

void VulkanEngine::getQueueFamilyPresentationSupport() {
    if (vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevices[0], graphicQueueFamilyIndex) == VK_TRUE) {
        std::cout << "Selected Graphical Queue Family supports presentation." << std::endl;
    }
    else
        throw VulkanException("Selected queue family (graphical) doesn't support presentation.");
}

void VulkanEngine::createSurface() {
    VkResult result;

    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.pNext = nullptr;
    surfaceCreateInfo.flags = 0;
    surfaceCreateInfo.hinstance = hInstance;
    surfaceCreateInfo.hwnd = windowHandle;

    result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);

    if (result == VK_SUCCESS) {
        std::cout << "Surface created and associated with the window." << std::endl;
    }
    else
        throw VulkanException("Failed to create and/or assocate surface with the window");
}

void VulkanEngine::createSwapchain() {
    VkResult surfaceSupportResult = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], graphicQueueFamilyIndex, surface, &physicalDeviceSurfaceSupported);

    if (surfaceSupportResult == VK_SUCCESS && physicalDeviceSurfaceSupported == VK_TRUE) {
        std::cout << "Physical Device selected graphical queue family supports presentation." << std::endl;
    }
    else
        throw VulkanException("Physical Device selected graphical queue family doesn't support presentation.");

    VkResult surfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[0], surface, &surfaceCapabilities);

    if (surfaceCapabilitiesResult == VK_SUCCESS) {
        std::cout << "Successfully fetched device surface capabilities." << std::endl;

        std::cout << "Minimum swap chain image count: " << surfaceCapabilities.minImageCount << std::endl;
        std::cout << "Maximum swap chain image count: " << surfaceCapabilities.maxImageCount << std::endl;
    }
    else
        throw VulkanException("Couldn't fetch device surface capabilities.");

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[0], surface, &surfaceSupportedFormatsCount, nullptr) != VK_SUCCESS)
        throw VulkanException("Couldn't get surface supported formats.");

    surfaceSupportedFormats = new VkSurfaceFormatKHR[surfaceSupportedFormatsCount];

    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[0], surface, &surfaceSupportedFormatsCount, surfaceSupportedFormats) != VK_SUCCESS)
        throw VulkanException("Couldn't get surface supported formats.");


    uint32_t supportedFormatColorSpacePairIndex = -1;
    uint32_t supportedPresentModeIndex = -1;

    for (int i = 0; i < surfaceSupportedFormatsCount; i++) {
        if (surfaceSupportedFormats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            supportedFormatColorSpacePairIndex = i;
            break;
        }
    }

    if (supportedFormatColorSpacePairIndex == -1)
        throw VulkanException("Couldn't find R8G8B8A8_UNORM format in supported formats.");

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[0], surface, &surfaceSupportedPresentModesCount, nullptr) != VK_SUCCESS) {
        throw VulkanException("Couldn't get surface supported presentation modes.");
    }

    surfaceSupportedPresentModes = new VkPresentModeKHR[surfaceSupportedPresentModesCount];

    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[0], surface, &surfaceSupportedPresentModesCount, surfaceSupportedPresentModes) != VK_SUCCESS) {
        throw VulkanException("Couldn't get surface supported presentation modes.");
    }

    for (int i = 0; i < surfaceSupportedPresentModesCount; i++) {
        if (surfaceSupportedPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            supportedPresentModeIndex = i;
            break;
        }
    }

    if (surfaceSupportedPresentModesCount == -1)
        throw VulkanException("Couldn't find IMMEDIATE present mode in supported present modes.");

    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.pNext = nullptr;
    swapchainCreateInfo.flags = 0;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = (2 >= surfaceCapabilities.minImageCount && 2 <= surfaceCapabilities.maxImageCount) ? (2) : (surfaceCapabilities.minImageCount);
    swapchainCreateInfo.imageFormat = surfaceSupportedFormats[supportedFormatColorSpacePairIndex].format;
    swapchainCreateInfo.imageColorSpace = surfaceSupportedFormats[supportedFormatColorSpacePairIndex].colorSpace;
    swapchainCreateInfo.imageExtent.width = surfaceCapabilities.currentExtent.width;
    swapchainCreateInfo.imageExtent.height = surfaceCapabilities.currentExtent.height;
    swapchainCreateInfo.imageArrayLayers = 1;

    if ((surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
    {
        std::cout << "Surface supports COLOR_ATTACHMENT usage bits." << std::endl;
    }
    else
        throw VulkanException("Surface doesn't support COLOR_ATTACHMENT usage bits.");

    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    swapchainCreateInfo.queueFamilyIndexCount = 0;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(logicalDevices[0], &swapchainCreateInfo, nullptr, &swapchain);

    if (result == VK_SUCCESS) {
        std::cout << "Swapchain created successfully." << std::endl;
    }
    else
        throw VulkanException("Swapchain creation failed.");

    VkResult swapchainImageResult;

    if ((swapchainImageResult = vkGetSwapchainImagesKHR(logicalDevices[0], swapchain, &swapchainImagesCount, nullptr)) != VK_SUCCESS)
        throw VulkanException("Couldn't get swapchain images.");

    swapchainImages = new VkImage[swapchainImagesCount];

    if ((swapchainImageResult = vkGetSwapchainImagesKHR(logicalDevices[0], swapchain, &swapchainImagesCount, swapchainImages)) == VK_SUCCESS) {
        std::cout << "Swapchain images obtained successfully." << std::endl;
    }
    else
        throw VulkanException("Couldn't get swapchain images.");
}

uint32_t VulkanEngine::acquireNextFramebufferImageIndex() {
    uint32_t imageIndex = -1;

    //if (pAcquireNextImageIndexFence == NULL) {
    //	pAcquireNextImageIndexFence = new VkFence;

    //	VkFenceCreateInfo fenceCreateInfo = {};

    //	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    //	fenceCreateInfo.pNext = nullptr;
    //	fenceCreateInfo.flags = 0;

    //	VkResult createFenceResult = vkCreateFence(logicalDevices[0], &fenceCreateInfo, nullptr, pAcquireNextImageIndexFence);
    //}

    //vkResetFences(logicalDevices[0], 1, pAcquireNextImageIndexFence);

    vkDeviceWaitIdle(logicalDevices[0]);

    VkResult acquireNextImageIndexResult = vkAcquireNextImageKHR(logicalDevices[0], swapchain, 0, indexAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);

    //vkDestroyFence(logicalDevices[0], *pAcquireNextImageIndexFence, nullptr);

    //pAcquireNextImageIndexFence = NULL;

    //std::cout << "Next available swapchain image index acquired." << std::endl;

    return imageIndex;
}


void VulkanEngine::createQueueDoneFence() {
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.pNext = nullptr;
    fenceCreateInfo.flags = 0;

    VKASSERT_SUCCESS(vkCreateFence(logicalDevices[0], &fenceCreateInfo, nullptr, &queueDoneFence));
}

void VulkanEngine::present(uint32_t swapchainPresentImageIndex) {
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pImageIndices = &swapchainPresentImageIndex;
    presentInfo.pResults = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitToPresentSemaphore;

    VkResult result = vkQueuePresentKHR(queue, &presentInfo);


    if (result == VK_SUBOPTIMAL_KHR)
        std::cout << "Image presentation command successfully submitted to queue, but the swapchain is not longer optimal for the target surface." << std::endl;
    else if (result != VK_SUCCESS)
        throw VulkanException("Failed to present.");

    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(logicalDevices[0], renderCommandPool, 1, &renderCommandBuffer);
}

void VulkanEngine::createRenderpass() {
    VkAttachmentDescription attachments[2];
    attachments[0].flags = 0;
    attachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachments[1].flags = 0;
    attachments[1].format = depthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};


    VkAttachmentReference colorAttachment = {};
    colorAttachment.attachment = 0;
    colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthStencilAttachment = {};
    depthStencilAttachment.attachment = 1;
    depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;



    subpass.flags = 0;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pDepthStencilAttachment = &depthStencilAttachment;
    subpass.pColorAttachments = &colorAttachment;
    subpass.pResolveAttachments = nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.pNext = nullptr;
    renderPassCreateInfo.flags = 0;
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = attachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;


    VkSubpassDependency subpassDependencies[1];

    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Producer of the dependency
    subpassDependencies[0].dstSubpass = 0;													// Consumer is our single subpass that will wait for the execution depdendency
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //// Second dependency at the end the renderpass
    //// Does the transition from the initial to the final layout
    //subpassDependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
    //subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
    //subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = subpassDependencies;

    VkResult result = vkCreateRenderPass(logicalDevices[0], &renderPassCreateInfo, nullptr, &renderPass);

    if (result == VK_SUCCESS)
        std::cout << "Renderpass created successfully." << std::endl;
    else
        throw VulkanException("Couldn't create Renderpass.");
}

void VulkanEngine::createFramebuffers() {
    framebuffers = new VkFramebuffer[swapchainImagesCount];

    for (int i = 0; i < swapchainImagesCount; i++) {
        VkImageView attachments[2] = { swapchainImageViews[i], depthImageView };

        VkFramebufferCreateInfo framebufferCreateInfo = {};

        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.pNext = nullptr;
        framebufferCreateInfo.flags = 0;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 2;
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = surfaceCapabilities.currentExtent.width;
        framebufferCreateInfo.height = surfaceCapabilities.currentExtent.height;
        framebufferCreateInfo.layers = 1;

        VkResult result = vkCreateFramebuffer(logicalDevices[0], &framebufferCreateInfo, nullptr, framebuffers + i);

        if (result != VK_SUCCESS)
            throw VulkanException("Couldn't create framebuffer.");
    }

    std::cout << "Framebuffer(s) created successfully." << std::endl;
}

void VulkanEngine::createSwapchainImageViews() {

    swapchainImageViews = new VkImageView[swapchainImagesCount];

    for (int i = 0; i < swapchainImagesCount; i++) {
        VkImage swapchainImage = swapchainImages[i];

        VkImageViewCreateInfo swapchainImageViewCreateInfo = {};
        swapchainImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        swapchainImageViewCreateInfo.pNext = nullptr;
        swapchainImageViewCreateInfo.flags = 0;
        swapchainImageViewCreateInfo.image = swapchainImages[i];
        swapchainImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        swapchainImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        swapchainImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        swapchainImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        swapchainImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        swapchainImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        swapchainImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        swapchainImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        swapchainImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        swapchainImageViewCreateInfo.subresourceRange.layerCount = 1;
        swapchainImageViewCreateInfo.subresourceRange.levelCount = 1;

        VkResult result = vkCreateImageView(logicalDevices[0], &swapchainImageViewCreateInfo, nullptr, swapchainImageViews + i);

        if (result == VK_SUCCESS)
            std::cout << "ImageView for swap chain image (" << i << ") created successfully." << std::endl;
        else
            throw VulkanException("ImageView creation failed.");
    }

}

void VulkanEngine::createGraphicsPipeline() {
    VkPipelineShaderStageCreateInfo stageCreateInfos[2];

    stageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageCreateInfos[0].pNext = nullptr;
    stageCreateInfos[0].flags = 0;
    stageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stageCreateInfos[0].module = graphicsVertexShaderModule;
    stageCreateInfos[0].pName = u8"main";
    stageCreateInfos[0].pSpecializationInfo = nullptr;

    stageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageCreateInfos[1].pNext = nullptr;
    stageCreateInfos[1].flags = 0;
    stageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stageCreateInfos[1].module = graphicsFragmentShaderModule;
    stageCreateInfos[1].pName = u8"main";
    stageCreateInfos[1].pSpecializationInfo = nullptr;

    vertexBindingDescription.binding = 0;
    vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    vertexBindingDescription.stride = sizeof(attribute);

    VkVertexInputAttributeDescription vertexAttributeDescriptions[5];

    vertexAttributeDescriptions[0].binding = 0;
    vertexAttributeDescriptions[0].location = 0;
    vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[0].offset = offsetof(attribute, position);

    vertexAttributeDescriptions[1].binding = 0;
    vertexAttributeDescriptions[1].location = 1;
    vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[1].offset = offsetof(attribute, normal);

    vertexAttributeDescriptions[2].binding = 0;
    vertexAttributeDescriptions[2].location = 2;
    vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttributeDescriptions[2].offset = offsetof(attribute, uv);

    vertexAttributeDescriptions[3].binding = 0;
    vertexAttributeDescriptions[3].location = 3;
    vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[3].offset = offsetof(attribute, tangent);

    vertexAttributeDescriptions[4].binding = 0;
    vertexAttributeDescriptions[4].location = 4;
    vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    vertexAttributeDescriptions[4].offset = offsetof(attribute, bitangent);

    vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateCreateInfo.pNext = nullptr;
    vertexInputStateCreateInfo.flags = 0;
    vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
    vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

    inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateCreateInfo.pNext = nullptr;
    inputAssemblyStateCreateInfo.flags = 0;
    inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

    viewport.width = swapchainCreateInfo.imageExtent.width;
    viewport.height = swapchainCreateInfo.imageExtent.height;
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;

    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.pNext = nullptr;
    viewportStateCreateInfo.flags = 0;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateCreateInfo.pNext = nullptr;
    rasterizationStateCreateInfo.flags = 0;
    rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE; // no back-face/front-face culling!
    rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
    rasterizationStateCreateInfo.lineWidth = 1.0f;


    multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateCreateInfo.pNext = nullptr;
    multisampleStateCreateInfo.flags = 0;
    multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateCreateInfo.pSampleMask = nullptr;

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
    colorBlendAttachmentState.blendEnable = VK_FALSE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_MAX;
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateCreateInfo.pNext = nullptr;
    colorBlendStateCreateInfo.flags = 0;
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = { };
    depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStateCreateInfo.pNext = nullptr;
    depthStencilStateCreateInfo.flags = 0;
    depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
    depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
    depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.pNext = nullptr;
    graphicsPipelineCreateInfo.flags = 0;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = stageCreateInfos;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
    graphicsPipelineCreateInfo.pTessellationState = nullptr;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = nullptr;
    graphicsPipelineCreateInfo.layout = graphicsPipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;

    VkResult result = vkCreateGraphicsPipelines(logicalDevices[0], VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);

    if (result == VK_SUCCESS)
        std::cout << "Graphics Pipeline created successfully." << std::endl;
    else
        throw VulkanException("Couldn't create graphics pipeline.");
}

void VulkanEngine::createVertexGraphicsShaderModule() {
    void * pShaderData;
    DWORD shaderSize = loadShaderCode("..\\Resources\\vert.spv", (char**)&pShaderData);

    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.codeSize = shaderSize;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.pCode = (uint32_t*)pShaderData;

    if (vkCreateShaderModule(logicalDevices[0], &shaderModuleCreateInfo, nullptr, &graphicsVertexShaderModule) == VK_SUCCESS) {
        std::cout << "Graphics Shader Module created successfully." << std::endl;
    }
    else
        throw VulkanException("Couldn't create vertex graphics shader module.");
}

unsigned long VulkanEngine::loadShaderCode(const char* fileName, char** fileData) {
    std::ifstream inTemp(fileName, std::ifstream::ate | std::ifstream::binary);
    unsigned long fileSize =  inTemp.tellg();
    inTemp.close();

    std::ifstream inFile (fileName, std::ios::in | std::ios::binary);

    *fileData = new char[fileSize];

    inFile.read(*fileData, fileSize);

    inFile.close();

    return fileSize;
}

void VulkanEngine::createGeometryGraphicsShaderModule() {

    void * pShaderData;
    DWORD shaderSize = loadShaderCode("..\\Resources\\geom.spv", ( char**)&pShaderData);

    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.codeSize = shaderSize;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.pCode = (uint32_t*)pShaderData;

    if (vkCreateShaderModule(logicalDevices[0], &shaderModuleCreateInfo, nullptr, &graphicsGeometryShaderModule) == VK_SUCCESS) {
        std::cout << "Graphics Shader Module created successfully." << std::endl;
    }
    else
        throw VulkanException("Couldn't create fragment graphics shader module.");
}

void VulkanEngine::createFragmentGraphicsShaderModule() {
    void * pShaderData;
    DWORD shaderSize = loadShaderCode("..\\Resources\\frag.spv", (char**)&pShaderData);

    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.pNext = nullptr;
    shaderModuleCreateInfo.codeSize = shaderSize;
    shaderModuleCreateInfo.flags = 0;
    shaderModuleCreateInfo.pCode = (uint32_t*)pShaderData;

    if (vkCreateShaderModule(logicalDevices[0], &shaderModuleCreateInfo, nullptr, &graphicsFragmentShaderModule) == VK_SUCCESS) {
        std::cout << "Graphics Shader Module created successfully." << std::endl;
    }
    else
        throw VulkanException("Couldn't create fragment graphics shader module.");
}

void VulkanEngine::createPipelineAndDescriptorSetsLayout() {
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};

    VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[4];
    descriptorSetLayoutBindings[0].binding = 0;
    descriptorSetLayoutBindings[0].descriptorCount = 1;
    descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
    descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutBindings[1].binding = 1;
    descriptorSetLayoutBindings[1].descriptorCount = 1;
    descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
    descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutBindings[2].binding = 2;
    descriptorSetLayoutBindings[2].descriptorCount = 1;
    descriptorSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;
    descriptorSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutBindings[3].binding = 3;
    descriptorSetLayoutBindings[3].descriptorCount = 1;
    descriptorSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorSetLayoutBindings[3].pImmutableSamplers = nullptr;
    descriptorSetLayoutBindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCreateInfo.pNext = nullptr;
    descriptorSetLayoutCreateInfo.flags = 0;
    descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
    descriptorSetLayoutCreateInfo.bindingCount = 4;

    VkResult result = vkCreateDescriptorSetLayout(logicalDevices[0], &descriptorSetLayoutCreateInfo, nullptr, &graphicsDescriptorSetLayout);

    if (result == VK_SUCCESS)
        std::cout << "Descriptor Set Layout created successfully." << std::endl;
    else
        throw VulkanException("Couldn't create descriptor set layout");

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ViewProjectionMatrices);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

    graphicsPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    graphicsPipelineLayoutCreateInfo.pNext = nullptr;
    graphicsPipelineLayoutCreateInfo.flags = 0;
    graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    graphicsPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
    graphicsPipelineLayoutCreateInfo.pSetLayouts = &graphicsDescriptorSetLayout;
    graphicsPipelineLayoutCreateInfo.setLayoutCount = 1;

    result = vkCreatePipelineLayout(logicalDevices[0], &graphicsPipelineLayoutCreateInfo, nullptr, &graphicsPipelineLayout);

    if (result == VK_SUCCESS) {
        std::cout << "Graphics Pipeline Layout created successfully." << std::endl;
    }
    else
        throw VulkanException("Couldn't create graphics pipeline layout.");
}

void VulkanEngine::createRenderCommandPool() {
    VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, graphicQueueFamilyIndex };
    VkResult result = vkCreateCommandPool(logicalDevices[0], &commandPoolCreateInfo, nullptr, &renderCommandPool);

    if (result == VK_SUCCESS) {
        std::cout << "Command Pool created successfully." << std::endl;
    }
    else
        throw VulkanException("Couldn't create command pool.");
}



void VulkanEngine::render(uint32_t drawableImageIndex) {
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.pNext = nullptr;
    commandBufferAllocateInfo.commandBufferCount = 1;
    commandBufferAllocateInfo.commandPool = renderCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VKASSERT_SUCCESS(vkAllocateCommandBuffers(logicalDevices[0], &commandBufferAllocateInfo, &renderCommandBuffer));


    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.pInheritanceInfo = nullptr;
    commandBufferBeginInfo.flags = 0;

    VKASSERT_SUCCESS(vkBeginCommandBuffer(renderCommandBuffer, &commandBufferBeginInfo));

    VkRenderPassBeginInfo renderPassBeginInfo = {};

    VkClearValue clearValues[2];
    clearValues[0].color.float32[0] = 0.1f; //gray clear color
    clearValues[0].color.float32[1] = 0.1f;
    clearValues[0].color.float32[2] = 0.1f;
    clearValues[0].color.float32[3] = 1.0f;

    clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.clearValueCount = 2;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.renderArea.extent.width = swapchainCreateInfo.imageExtent.width;
    renderPassBeginInfo.renderArea.extent.height = swapchainCreateInfo.imageExtent.height;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffers[drawableImageIndex];


    vkCmdBeginRenderPass(renderCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

//    const long long sysTimeMS = GetTickCount();

    vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        QueryPerformanceCounter(&t2);
        elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;

        float xRotation = (3.1415926536f / 180.0f) * 90.0f;
        float yRotation = (3.1415926536 / 180) * elapsedTime / 10;

        float viewMatrices[2][16];


        viewMatrices[0][0] = 1.0f;	viewMatrices[0][4] = 0.0f;	viewMatrices[0][8] = 0.0f;	viewMatrices[0][12] = 0.00f;	// x translate
        viewMatrices[0][1] = 0.0f;	viewMatrices[0][5] = 1.0f;	viewMatrices[0][9] = 0.0f;	viewMatrices[0][13] = 0.8f;		// y translate
        viewMatrices[0][2] = 0.0f;	viewMatrices[0][6] = 0.0f;	viewMatrices[0][10] = 1.0f;	viewMatrices[0][14] = -1.85f;	// z translate
        viewMatrices[0][3] = 0.0f;	viewMatrices[0][7] = 0.0f;	viewMatrices[0][11] = 0.0f;	viewMatrices[0][15] = 1.0f;

        viewMatrices[1][0] = cos(yRotation);	viewMatrices[1][4] = 0.0f;	viewMatrices[1][8] = sin(yRotation);	viewMatrices[1][12] = 0.0f;
        viewMatrices[1][1] = 0.0f;				viewMatrices[1][5] = 1.0f;	viewMatrices[1][9] = 0.0f;				viewMatrices[1][13] = 0.0f;
        viewMatrices[1][2] = -sin(yRotation);	viewMatrices[1][6] = 0.0f;	viewMatrices[1][10] = cos(yRotation);	viewMatrices[1][14] = 0.0f;
        viewMatrices[1][3] = 0.0f;				viewMatrices[1][7] = 0.0f;	viewMatrices[1][11] = 0.0f;				viewMatrices[1][15] = 1.0f;

        multiplyMatrix<float>(viewProjection.viewMatrix, viewMatrices[0], viewMatrices[1]);

        float frameBufferAspectRatio = ((float)swapchainCreateInfo.imageExtent.width) / ((float)swapchainCreateInfo.imageExtent.height);

        calculateProjectionMatrix<float>((float*)viewProjection.projectionMatrix, (3.1415956536f / 180.0f) * 60.0f, frameBufferAspectRatio, 0.1f, 500.0f); // calculate perspective matrix

        vkCmdPushConstants(renderCommandBuffer, graphicsPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ViewProjectionMatrices), &viewProjection);


        vkCmdBindDescriptorSets(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1, meshDescriptorSets + meshIndex, 0, nullptr);



        VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(renderCommandBuffer, 0, 1, vertexBuffers + meshIndex, &offset);

        vkCmdBindIndexBuffer(renderCommandBuffer, indexBuffers[meshIndex], 0, VK_INDEX_TYPE_UINT32);


        vkCmdDrawIndexed(renderCommandBuffer, sortedIndices[meshIndex].size(), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(renderCommandBuffer);

    VKASSERT_SUCCESS(vkEndCommandBuffer(renderCommandBuffer));

    VkSubmitInfo queueSubmit = {};
    queueSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queueSubmit.pNext = nullptr;
    queueSubmit.waitSemaphoreCount = 1;
    queueSubmit.pWaitSemaphores = &indexAcquiredSemaphore;
    VkPipelineStageFlags dstSemaphoreStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    queueSubmit.pWaitDstStageMask = &dstSemaphoreStageFlags;
    queueSubmit.commandBufferCount = 1;
    queueSubmit.pCommandBuffers = &renderCommandBuffer;
    queueSubmit.signalSemaphoreCount = 1;
    queueSubmit.pSignalSemaphores = &waitToPresentSemaphore;

    VKASSERT_SUCCESS(vkQueueSubmit(queue, 1, &queueSubmit, queueDoneFence));

    VkResult waitForFenceResult = vkWaitForFences(logicalDevices[0], 1, &queueDoneFence, VK_TRUE, 1000000000); // 1sec timeout

    switch (waitForFenceResult) {
        case VK_TIMEOUT:
            throw VulkanException("Queue execution timeout!");
            break;
        case VK_SUCCESS:
            break;
        default:
            throw VulkanException("Queue submission failed!");
    }

    VKASSERT_SUCCESS(vkResetFences(logicalDevices[0], 1, &queueDoneFence));

    //vkResetCommandPool(logicalDevices[0], renderCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

    //vkResetDescriptorPool(logicalDevices[0], descriptorPool, 0);
}

void VulkanEngine::createWaitToDrawSemaphore() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};

    semaphoreCreateInfo.flags = 0;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;

    VKASSERT_SUCCESS(vkCreateSemaphore(logicalDevices[0], &semaphoreCreateInfo, nullptr, &indexAcquiredSemaphore));
}

void VulkanEngine::createWaitToPresentSemaphore() {
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};

    semaphoreCreateInfo.flags = 0;
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.pNext = nullptr;

    VKASSERT_SUCCESS(vkCreateSemaphore(logicalDevices[0], &semaphoreCreateInfo, nullptr, &waitToPresentSemaphore));
}

void VulkanEngine::createSyncMeans() {
    createWaitToDrawSemaphore();
    createWaitToPresentSemaphore();
    createQueueDoneFence();
}

void VulkanEngine::destroySyncMeans() {

    vkDestroySemaphore(logicalDevices[0], indexAcquiredSemaphore, nullptr);
    std::cout << "Semaphore destroyed." << std::endl;


    vkDestroySemaphore(logicalDevices[0], waitToPresentSemaphore, nullptr);
    std::cout << "Semaphore destroyed." << std::endl;


    vkDestroyFence(logicalDevices[0], queueDoneFence, nullptr);
    std::cout << "Fence destroyed." << std::endl;
}


void VulkanEngine::createDescriptorPool() {
    VkDescriptorPoolSize descriptorPoolSizes[2];
    descriptorPoolSizes[0].descriptorCount = cachedScene->mNumMeshes;
    descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    descriptorPoolSizes[1].descriptorCount = cachedScene->mNumMeshes * 3;
    descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    descriptorPoolCreateInfo = {};
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = cachedScene->mNumMeshes;
    descriptorPoolCreateInfo.poolSizeCount = 2;
    descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

    VKASSERT_SUCCESS(vkCreateDescriptorPool(logicalDevices[0], &descriptorPoolCreateInfo, nullptr, &descriptorPool));
}


void VulkanEngine::createDescriptorSets() {
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.pNext = nullptr;
    samplerCreateInfo.flags = 0;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 1.0f;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

    VKASSERT_SUCCESS(vkCreateSampler(logicalDevices[0], &samplerCreateInfo, nullptr, &textureSampler));

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
        descriptorSetAllocateInfo = {};
        descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetAllocateInfo.pNext = nullptr;
        descriptorSetAllocateInfo.descriptorPool = descriptorPool;
        descriptorSetAllocateInfo.descriptorSetCount = 1;
        descriptorSetAllocateInfo.pSetLayouts = &graphicsDescriptorSetLayout;

        VKASSERT_SUCCESS(vkAllocateDescriptorSets(logicalDevices[0], &descriptorSetAllocateInfo, meshDescriptorSets + meshIndex));

        VkDescriptorImageInfo descriptorSetImageInfo = {};
        descriptorSetImageInfo.imageView = colorTextureViews[meshIndex];
        descriptorSetImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorSetImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo descriptorSetNormalImageInfo = {};
        descriptorSetNormalImageInfo.imageView = normalTextureViews[meshIndex];
        descriptorSetNormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorSetNormalImageInfo.sampler = textureSampler;

        VkDescriptorImageInfo descriptorSetSpecImageInfo = {};
        descriptorSetSpecImageInfo.imageView = specTextureViews[meshIndex];
        descriptorSetSpecImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorSetSpecImageInfo.sampler = textureSampler;


        VkDescriptorBufferInfo descriptorSetBufferInfo = {};
        descriptorSetBufferInfo.buffer = uniformBuffers[meshIndex];
        descriptorSetBufferInfo.offset = 0;
        descriptorSetBufferInfo.range = VK_WHOLE_SIZE;

        VkWriteDescriptorSet  descriptorSetWrites[4];
        descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[0].pNext = nullptr;
        descriptorSetWrites[0].dstSet = meshDescriptorSets[meshIndex];
        descriptorSetWrites[0].dstBinding = 0;
        descriptorSetWrites[0].dstArrayElement = 0;
        descriptorSetWrites[0].descriptorCount = 1;
        descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorSetWrites[0].pImageInfo = nullptr;
        descriptorSetWrites[0].pBufferInfo = &descriptorSetBufferInfo;
        descriptorSetWrites[0].pTexelBufferView = nullptr;

        descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[1].pNext = nullptr;
        descriptorSetWrites[1].dstSet = meshDescriptorSets[meshIndex];
        descriptorSetWrites[1].dstBinding = 1;
        descriptorSetWrites[1].dstArrayElement = 0;
        descriptorSetWrites[1].descriptorCount = 1;
        descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetWrites[1].pImageInfo = &descriptorSetImageInfo;
        descriptorSetWrites[1].pBufferInfo = nullptr;
        descriptorSetWrites[1].pTexelBufferView = nullptr;

        descriptorSetWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[2].pNext = nullptr;
        descriptorSetWrites[2].dstSet = meshDescriptorSets[meshIndex];
        descriptorSetWrites[2].dstBinding = 2;
        descriptorSetWrites[2].dstArrayElement = 0;
        descriptorSetWrites[2].descriptorCount = 1;
        descriptorSetWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetWrites[2].pImageInfo = &descriptorSetNormalImageInfo;
        descriptorSetWrites[2].pBufferInfo = nullptr;
        descriptorSetWrites[2].pTexelBufferView = nullptr;

        descriptorSetWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWrites[3].pNext = nullptr;
        descriptorSetWrites[3].dstSet = meshDescriptorSets[meshIndex];
        descriptorSetWrites[3].dstBinding = 3;
        descriptorSetWrites[3].dstArrayElement = 0;
        descriptorSetWrites[3].descriptorCount = 1;
        descriptorSetWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorSetWrites[3].pImageInfo = &descriptorSetSpecImageInfo;
        descriptorSetWrites[3].pBufferInfo = nullptr;
        descriptorSetWrites[3].pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(logicalDevices[0], 4, descriptorSetWrites, 0, nullptr);
    }
}

typedef ASSIMP_API const C_STRUCT aiScene* FNP_aiImportFile(const char*, unsigned int);

void VulkanEngine::loadMesh() {
    std::string err;

    HMODULE assimpModule = LoadLibrary("assimp-vc140-mt.dll");

    if(assimpModule == NULL)
        throw std::exception();

    FNP_aiImportFile *_aiImportFile = (FNP_aiImportFile*)GetProcAddress(assimpModule, "aiImportFile");

    cachedScene = (aiScene*)malloc(sizeof(aiScene));

    const aiScene *scene = _aiImportFile("..\\Resources\\nyra.obj", aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
    memcpy(cachedScene, scene, sizeof(aiScene));

    std::cout << "Number of Meshes reported from Assimp: " << cachedScene->mNumMeshes << std::endl;

    if (scene == NULL) {
        throw VulkanException("Couldn't load obj file: ");
    }
    else {
        std::cout << "OBj File Loaded successfully." << std::endl;
    }

    for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {

        uint32_t numVertices = cachedScene->mMeshes[meshIndex]->mNumVertices;
        uint32_t numFaces = cachedScene->mMeshes[meshIndex]->mNumFaces;
        uint32_t numIndices = numFaces * 3; // Triangulated

        for (int i = 0; i < numVertices; i++) {

            attribute tmpAttribute = {};
            memcpy(((byte*)&tmpAttribute) + offsetof(attribute, position), &((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mVertices))[i].x, 3 * sizeof(float));
            memcpy(((byte*)&tmpAttribute) + offsetof(attribute, normal), &((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mNormals))[i].x, 3 * sizeof(float));
            memcpy(((byte*)&tmpAttribute) + offsetof(attribute, uv), &((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mTextureCoords[0]))[i].x, 2 * sizeof(float));
            memcpy(((byte*)&tmpAttribute) + offsetof(attribute, tangent), &((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mTangents))[i].x, 3 * sizeof(float));
            memcpy(((byte*)&tmpAttribute) + offsetof(attribute, bitangent), &((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mBitangents))[i].x, 3 * sizeof(float));

            sortedAttributes[meshIndex].push_back(tmpAttribute);
        }

        for (int i = 0; i < numFaces; i++) {
            for (int j = 0; j < ((aiFace*)(cachedScene->mMeshes[meshIndex]->mFaces))[i].mNumIndices; j++) {
                uint32_t index = ((unsigned int*)(((aiFace*)(cachedScene->mMeshes[meshIndex]->mFaces))[i].mIndices))[j];
                sortedIndices[meshIndex].push_back(index);
            }
        }

        vertexBuffersSizes[meshIndex] = sortedAttributes[meshIndex].size() * sizeof(attribute);
        indexBuffersSizes[meshIndex] = sortedIndices[meshIndex].size() * 4;
    }
}

VkMemoryRequirements VulkanEngine::createBuffer(VkBuffer* buffer, VkDeviceSize size, VkBufferUsageFlags usageFlags) {
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.usage = usageFlags;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = size;

    VKASSERT_SUCCESS(vkCreateBuffer(logicalDevices[0], &bufferCreateInfo, nullptr, buffer));

    VkMemoryRequirements uniformBufferMemoryRequirements;

    vkGetBufferMemoryRequirements(logicalDevices[0], *buffer, &uniformBufferMemoryRequirements);

    return uniformBufferMemoryRequirements;
}

VkMemoryRequirements VulkanEngine::createTexture(VkImage* textureImage) {
    VkImageCreateInfo textureImageCreateInfo = {};

    textureImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    textureImageCreateInfo.pNext = nullptr;
    textureImageCreateInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    textureImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    textureImageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureImageCreateInfo.extent.width = 1024;
    textureImageCreateInfo.extent.height = 1024;
    textureImageCreateInfo.extent.depth = 1;
    textureImageCreateInfo.arrayLayers = 1;
    textureImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    textureImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    textureImageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    textureImageCreateInfo.mipLevels = 1;
    textureImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    textureImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    textureImageCreateInfo.queueFamilyIndexCount = 0;
    textureImageCreateInfo.pQueueFamilyIndices = nullptr;

    VKASSERT_SUCCESS(vkCreateImage(logicalDevices[0], &textureImageCreateInfo, nullptr, textureImage));

    VkMemoryRequirements textureImageMemoryRequirements;

    vkGetImageMemoryRequirements(logicalDevices[0], *textureImage, &textureImageMemoryRequirements);

    return textureImageMemoryRequirements;
}

void VulkanEngine::createTextureView(VkImageView* textureImageView, VkImage textureImage) {
    VkImageViewCreateInfo textureImageViewCreateInfo = {};

    textureImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    textureImageViewCreateInfo.pNext = NULL;
    textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    textureImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    textureImageViewCreateInfo.flags = 0;
    textureImageViewCreateInfo.image = textureImage;
    textureImageViewCreateInfo.subresourceRange = {};
    textureImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    textureImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    textureImageViewCreateInfo.subresourceRange.levelCount = 1;
    textureImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    textureImageViewCreateInfo.subresourceRange.layerCount = 1;

    VKASSERT_SUCCESS(vkCreateImageView(logicalDevices[0], &textureImageViewCreateInfo, nullptr, textureImageView));
}