
#include "render/vulkan/vk_vertex_array.hpp"
#include "render/vulkan/vk_buffer.hpp"
#include "render/vulkan/vk_device.hpp"
#include "vulkan/vulkan.hpp"

namespace mam {

	void VKVertexArray::bind() const {
		vertexBuffers_->bind();
		indexBuffer_->bind();
	}

	void VKVertexArray::unBind() const {
		// No-op in Vulkan backend. Vertex input and instancing are configured in VKPipeline.
	}

	void VKVertexArray::release() const {
		vertexBuffers_->release();
		indexBuffer_->release();
	}

	void VKVertexArray::addVertexBuffer(VertexBuffer* vertexBuffer,
		unsigned int index, int count, UniformType type, bool normalized, int stride, const void* offset) {
		vertexBuffers_ = vertexBuffer;
	}

	void VKVertexArray::setIndexBuffer(IndexBuffer* indexBuffer) {
		indexBuffer_ = indexBuffer;
	}

	void VKVertexArray::addInstanceBuffer(VertexBuffer*, unsigned int,
		int, UniformType, int, const void*, unsigned int) {
		// No-op in Vulkan backend. Vertex input and instancing are configured in VKPipeline.
	}

	void VKVertexArray::addInstanceBufferMat4(VertexBuffer*,
		unsigned int, unsigned int) {
		// No-op in Vulkan backend. Vertex input and instancing are configured in VKPipeline.
	}

}