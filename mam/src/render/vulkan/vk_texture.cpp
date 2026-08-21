#include "render/vulkan/vk_device.hpp"
#include "render/vulkan/vk_context.hpp"
#include "render/vulkan/vk_texture.hpp"

namespace mam {

#pragma region Helpers
	static bool isDepthFormat(Format f) {
		return f == Format::DEPTH32F || f == Format::DEPTH24STENCIL8;
	}

	static vk::ImageAspectFlags aspectForFormat(Format f) {
		if (f == Format::DEPTH24STENCIL8)
			return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;

		if (f == Format::DEPTH32F)
			return vk::ImageAspectFlagBits::eDepth;

		return vk::ImageAspectFlagBits::eColor;
	}

	vk::Format VKTexture::toVKFormat(Format f) {
		switch (f) {
		case Format::RGBA8:           return vk::Format::eR8G8B8A8Unorm;
		case Format::RGBA16F:         return vk::Format::eR16G16B16A16Sfloat;
		case Format::RGBA32F:         return vk::Format::eR32G32B32A32Sfloat;
		case Format::RG32F:           return vk::Format::eR32G32Sfloat;
		case Format::DEPTH32F:        return vk::Format::eD32Sfloat;
		case Format::DEPTH24STENCIL8: return vk::Format::eD24UnormS8Uint;
		}
		return vk::Format::eR8G8B8A8Srgb;
	}

	vk::Filter VKTexture::toVKFilter(Filter f) {
		switch (f) {
		case Filter::NEAREST:
		case Filter::NEAREST_MIPMAP_NEAREST:
		case Filter::NEAREST_MIPMAP_LINEAR:  return vk::Filter::eNearest;
		default:                             return vk::Filter::eLinear;
		}
	}

	vk::SamplerMipmapMode VKTexture::toVKMipmapMode(Filter f) {
		switch (f) {
		case Filter::NEAREST_MIPMAP_NEAREST:
		case Filter::LINEAR_MIPMAP_NEAREST:  return vk::SamplerMipmapMode::eNearest;
		default:                             return vk::SamplerMipmapMode::eLinear;
		}
	}

	vk::SamplerAddressMode VKTexture::toVKWrap(Wrap c) {
		switch (c) {
		case Wrap::REPEAT:          return vk::SamplerAddressMode::eRepeat;
		case Wrap::MIRRORED_REPEAT: return vk::SamplerAddressMode::eMirroredRepeat;
		case Wrap::CLAMP_TO_EDGE:   return vk::SamplerAddressMode::eClampToEdge;
		}
		return vk::SamplerAddressMode::eRepeat;
	}

	bool VKTexture::wantsMipmaps(Filter f) {
		return f == Filter::LINEAR_MIPMAP_LINEAR ||
			f == Filter::LINEAR_MIPMAP_NEAREST ||
			f == Filter::NEAREST_MIPMAP_LINEAR ||
			f == Filter::NEAREST_MIPMAP_NEAREST;
	}

	u32 VKTexture::calcMipLevels(u32 w, u32 h) {
		u32 m = std::max(w, h);
		u32 levels = 1;
		while (m > 1) { m >>= 1; ++levels; }
		return levels;
	}

#pragma endregion

	VKTexture::VKTexture(VKDevice* device) {
		vk_device_ = device;
	}

	VKTexture::VKTexture(VKDevice* device, const std::string& filepath, bool relative) {
		vk_device_ = device;
		currentLayout_ = vk::ImageLayout::eUndefined;
		setts_.minFilter = Filter::LINEAR_MIPMAP_LINEAR;
		setts_.magFilter = Filter::LINEAR;
		setts_.wrap = Wrap::CLAMP_TO_EDGE;
		resetTexture(filepath, relative);
	}

	VKTexture::VKTexture(VKDevice* device, u32 width, u32 height, const TextureSettings& settings) {
		vk_device_ = device;
		currentLayout_ = vk::ImageLayout::eUndefined;
		width_ = width;
		height_ = height;
		path_ = "Texture from memory";
		setts_ = settings;

		vk::Format vkFmt = toVKFormat(setts_.format);
		bool depth = isDepthFormat(setts_.format);

		mipLevels_ = 1;

		vk::ImageUsageFlags usage{};

		if (depth) {
			usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
		}else {
			usage = vk::ImageUsageFlagBits::eColorAttachment |
				vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
		}

		createImage(width_, height_, mipLevels_, vkFmt,
			vk::ImageTiling::eOptimal,
			usage,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		createImageView(vkFmt, aspectForFormat(setts_.format), mipLevels_);
		createSampler(mipLevels_);
	}

	VKTexture::~VKTexture() {
		destroyResources();
	}

	void VKTexture::destroyResources() {
		if (sampler_) {
			vk_device_->device->destroySampler(sampler_);
			sampler_ = nullptr;
		}
		if (textureImageView_) {
			vk_device_->device->destroyImageView(textureImageView_);
			textureImageView_ = nullptr;
		}
		if (textureImage_) {
			vk_device_->device->destroyImage(textureImage_);
			textureImage_ = nullptr;
		}
		if (textureImageMemory_) {
			vk_device_->device->freeMemory(textureImageMemory_);
			textureImageMemory_ = nullptr;
		}
		currentLayout_ = vk::ImageLayout::eUndefined;
	}

	void VKTexture::setData(const void* data, u32 sizeBytes) {
		vk::Buffer       stagingBuffer;
		vk::DeviceMemory stagingMemory;
		vk_device_->createBuffer(sizeBytes,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent,
			stagingBuffer, stagingMemory);

		void* mapped = vk_device_->device->mapMemory(stagingMemory, 0, sizeBytes);
		std::memcpy(mapped, data, sizeBytes);
		vk_device_->device->unmapMemory(stagingMemory);

		transitionImageLayout(currentLayout_, vk::ImageLayout::eTransferDstOptimal);
		copyBufferToImage(stagingBuffer, width_, height_);
		transitionImageLayout(vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal);

		vk_device_->device->destroyBuffer(stagingBuffer);
		vk_device_->device->freeMemory(stagingMemory);
	}

	void VKTexture::setData(const void* data, u32 sizeBytes, u32, u32) {
		setData(data, sizeBytes);
	}

	void VKTexture::resetTexture(const std::string& filepath, bool relative) {
		destroyResources();

		const std::string resolvedPath = relative ? ("assets/" + filepath) : filepath;
		path_ = filepath;

		stbi_set_flip_vertically_on_load(1);
		int w = 0, h = 0, channels = 0;
		stbi_uc* pixels = stbi_load(resolvedPath.c_str(), &w, &h, &channels, STBI_rgb_alpha);

		if (!pixels || w <= 0 || h <= 0) {
			throw std::runtime_error("VKTexture: failed to load image: " + resolvedPath);
		}

		width_ = static_cast<u32>(w);
		height_ = static_cast<u32>(h);
		mipLevels_ = wantsMipmaps(setts_.minFilter) ? calcMipLevels(width_, height_) : 1;

		vk::DeviceSize imageSize = static_cast<vk::DeviceSize>(w * h * 4);
		vk::Format     vkFmt = toVKFormat(setts_.format);

		vk::Buffer       stagingBuffer;
		vk::DeviceMemory stagingMemory;
		vk_device_->createBuffer(imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			vk::MemoryPropertyFlagBits::eHostVisible |
			vk::MemoryPropertyFlagBits::eHostCoherent,
			stagingBuffer, stagingMemory);

		void* mapped = vk_device_->device->mapMemory(stagingMemory, 0, imageSize);
		std::memcpy(mapped, pixels, static_cast<size_t>(imageSize));
		vk_device_->device->unmapMemory(stagingMemory);
		stbi_image_free(pixels);

		createImage(width_, height_, mipLevels_, vkFmt,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferDst |
			vk::ImageUsageFlagBits::eTransferSrc |
			vk::ImageUsageFlagBits::eSampled,
			vk::MemoryPropertyFlagBits::eDeviceLocal);

		transitionImageLayout(vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal, mipLevels_);
		copyBufferToImage(stagingBuffer, width_, height_);

		if (mipLevels_ > 1) {
			generateMipmaps();
		}else {
			transitionImageLayout(vk::ImageLayout::eTransferDstOptimal,
				vk::ImageLayout::eShaderReadOnlyOptimal, 1);
		}

		vk_device_->device->destroyBuffer(stagingBuffer);
		vk_device_->device->freeMemory(stagingMemory);

		createImageView(vkFmt, vk::ImageAspectFlagBits::eColor, mipLevels_);
		createSampler(mipLevels_);
	}

	void VKTexture::generateMipmaps() {
		vk::FormatProperties fmtProps =
			vk_device_->physicalDevice.getFormatProperties(toVKFormat(setts_.format));

		if (!(fmtProps.optimalTilingFeatures &
			vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
			throw std::runtime_error("VKTexture: texture format does not support linear blitting");
		}

		vk::CommandBuffer cmd = vk_device_->beginSingleTimeCommands();

		vk::ImageMemoryBarrier barrier{};
		barrier.image = textureImage_;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipW = static_cast<int32_t>(width_);
		int32_t mipH = static_cast<int32_t>(height_);

		for (u32 i = 1; i < mipLevels_; ++i) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer, {},
				nullptr, nullptr, barrier);

			int32_t nextW = std::max(mipW / 2, 1);
			int32_t nextH = std::max(mipH / 2, 1);

			vk::ImageBlit blit{};
			blit.srcOffsets[0] = vk::Offset3D{ 0, 0, 0 };
			blit.srcOffsets[1] = vk::Offset3D{ mipW, mipH, 1 };
			blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
			blit.dstOffsets[1] = vk::Offset3D{ nextW, nextH, 1 };
			blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			cmd.blitImage(textureImage_, vk::ImageLayout::eTransferSrcOptimal,
				textureImage_, vk::ImageLayout::eTransferDstOptimal,
				blit, vk::Filter::eLinear);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eFragmentShader, {},
				nullptr, nullptr, barrier);

			mipW = nextW;
			mipH = nextH;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels_ - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
		cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
			vk::PipelineStageFlagBits::eFragmentShader, {},
			nullptr, nullptr, barrier);

		vk_device_->endSingleTimeCommands(cmd);
		currentLayout_ = vk::ImageLayout::eShaderReadOnlyOptimal;
	}

	void VKTexture::setMinFilter(Filter f) { setts_.minFilter = f; rebuildSampler(); }
	void VKTexture::setMagFilter(Filter f) { setts_.magFilter = f; rebuildSampler(); }
	void VKTexture::setWrapS(Wrap c) { setts_.wrap = c; rebuildSampler(); }
	void VKTexture::setWrapT(Wrap c) { setts_.wrap = c; rebuildSampler(); }
	void VKTexture::setWrapR(Wrap c) { setts_.wrap = c; rebuildSampler(); }
	void VKTexture::setWrap(Wrap c) { setts_.wrap = c; rebuildSampler(); }

	void VKTexture::rebuildSampler() {
		if (!textureImage_) return;
		if (sampler_) {
			vk_device_->device->destroySampler(sampler_);
			sampler_ = nullptr;
		}
		createSampler(mipLevels_);
	}

	void VKTexture::bind(u32 slot) const {
		// Vulkan textures are bound through VKPipeline material descriptor sets.
	}

	void VKTexture::unBind(u32 slot) const {

	}

	vk::DescriptorImageInfo VKTexture::descriptorInfo() const {
		return vk::DescriptorImageInfo{
				sampler_,
				textureImageView_,
				vk::ImageLayout::eShaderReadOnlyOptimal
		};
	}

	void VKTexture::createImage(uint32_t width, uint32_t height, u32 mipLevels, vk::Format format,
		vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties) {

		vk::ImageCreateInfo info{};
		info.imageType = vk::ImageType::e2D;
		info.extent = vk::Extent3D{ width, height, 1 };
		info.mipLevels = mipLevels;
		info.arrayLayers = 1;
		info.format = format;
		info.tiling = tiling;
		info.initialLayout = vk::ImageLayout::eUndefined;
		info.usage = usage;
		info.sharingMode = vk::SharingMode::eExclusive;
		info.samples = vk::SampleCountFlagBits::e1;

		textureImage_ = vk_device_->device->createImage(info);

		vk::MemoryRequirements memReqs =
			vk_device_->device->getImageMemoryRequirements(textureImage_);

		vk::MemoryAllocateInfo allocInfo{};
		allocInfo.allocationSize = memReqs.size;
		allocInfo.memoryTypeIndex = vk_device_->findMemoryType(
			memReqs.memoryTypeBits, properties);

		textureImageMemory_ = vk_device_->device->allocateMemory(allocInfo);
		vk_device_->device->bindImageMemory(textureImage_, textureImageMemory_, 0);

		currentLayout_ = vk::ImageLayout::eUndefined;
	}

	void VKTexture::createImageView(vk::Format format,
		vk::ImageAspectFlags aspectFlags, u32 mipLevels) {
		vk::ImageViewCreateInfo info{};
		info.image = textureImage_;
		info.viewType = vk::ImageViewType::e2D;
		info.format = format;
		info.subresourceRange.aspectMask = aspectFlags;
		info.subresourceRange.baseMipLevel = 0;
		info.subresourceRange.levelCount = mipLevels;
		info.subresourceRange.baseArrayLayer = 0;
		info.subresourceRange.layerCount = 1;

		textureImageView_ = vk_device_->device->createImageView(info);
	}

	void VKTexture::createSampler(u32 mipLevels) {
		vk::PhysicalDeviceProperties props =
			vk_device_->physicalDevice.getProperties();

		vk::SamplerCreateInfo info{};
		info.magFilter = toVKFilter(setts_.magFilter);
		info.minFilter = toVKFilter(setts_.minFilter);
		info.addressModeU = toVKWrap(setts_.wrap);
		info.addressModeV = toVKWrap(setts_.wrap);
		info.addressModeW = toVKWrap(setts_.wrap);

		info.anisotropyEnable = VK_FALSE;
		info.maxAnisotropy = 1.0f;

		info.borderColor = vk::BorderColor::eIntOpaqueBlack;
		info.unnormalizedCoordinates = VK_FALSE;
		info.compareEnable = VK_FALSE;
		info.compareOp = vk::CompareOp::eAlways;
		info.mipmapMode = toVKMipmapMode(setts_.minFilter);
		info.mipLodBias = 0.0f;
		info.minLod = 0.0f;
		info.maxLod = static_cast<float>(mipLevels);

		sampler_ = vk_device_->device->createSampler(info);
	}

	void VKTexture::transitionImageLayout(vk::ImageLayout oldLayout,
		vk::ImageLayout newLayout, u32 mipLevels) {
		vk::CommandBuffer cmd = vk_device_->beginSingleTimeCommands();

		vk::ImageMemoryBarrier barrier{};
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = textureImage_;
		barrier.subresourceRange.aspectMask = aspectForFormat(setts_.format);
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		vk::PipelineStageFlags srcStage, dstStage;

		if (oldLayout == vk::ImageLayout::eUndefined &&
			newLayout == vk::ImageLayout::eTransferDstOptimal) {
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
			srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
			dstStage = vk::PipelineStageFlagBits::eTransfer;

		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
			newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			srcStage = vk::PipelineStageFlagBits::eTransfer;
			dstStage = vk::PipelineStageFlagBits::eFragmentShader;

		}
		else if (oldLayout == vk::ImageLayout::eUndefined &&
			newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			barrier.srcAccessMask = {};
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			srcStage = vk::PipelineStageFlagBits::eTopOfPipe;
			dstStage = vk::PipelineStageFlagBits::eFragmentShader;

		}
		else {
			throw std::invalid_argument("VKTexture: unsupported layout transition");
		}

		cmd.pipelineBarrier(srcStage, dstStage, {},
			nullptr, nullptr, barrier);

		vk_device_->endSingleTimeCommands(cmd);
		currentLayout_ = newLayout;
	}

	void VKTexture::copyBufferToImage(vk::Buffer buffer, u32 width, u32 height) {
		vk::CommandBuffer cmd = vk_device_->beginSingleTimeCommands();

		vk::BufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = vk::Offset3D{ 0, 0, 0 };
		region.imageExtent = vk::Extent3D{ width, height, 1 };

		cmd.copyBufferToImage(buffer, textureImage_,
			vk::ImageLayout::eTransferDstOptimal, region);

		vk_device_->endSingleTimeCommands(cmd);
	}


}