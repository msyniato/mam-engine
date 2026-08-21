#pragma once 

#include <cstdint>
#include <memory>
#include <bitset>
#include <vector>
#include <string>
#include <string_view>
#include <queue>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <optional>
#include <future> 
#include <random>
#include <variant>
#include <Windows.h>
#undef CreateWindow 
#include <XInput.h>
#pragma comment(lib, "XInput.lib") 
#include <GL/glew.h> 
#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h> 
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h> 
#include <stb_image.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <chrono>

// Basic fixed-size integer typedefs
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

// Helper macro to check for OpenGL errors
#define GL_CHECK() { GLenum e = glGetError(); if (e != GL_NO_ERROR) printf("GL error: 0x%x at %s:%d\n", e, __FILE__, __LINE__); }

namespace mam {

	/// Simple vertex struct for mesh data
	struct Vertex {
		glm::vec3 position; ///< Vertex position
		glm::vec3 normal;   ///< Vertex normal
		glm::vec2 texCoord; ///< Texture coordinate
		glm::vec4 tangent; 
	};
	
	constexpr std::size_t kMaxComponentTypes = 256;
	constexpr std::size_t kMaxEntities			 = 1024;
	constexpr std::size_t kMaxSystems				 = 64;
	constexpr std::size_t kMaxScenes				 = 64;
	constexpr std::size_t kMaxWorlds				 = 64;
	constexpr std::size_t kMaxGPULights			 = 1024;
	

	// ECS entity and component types
	using Signature	= std::bitset<kMaxComponentTypes>; ///< Bitset representing an entity's component signature
	using Hash			= u64;
	using Type			= u16;
	using ID				= u32; // TO-DO make separate ids for clarity

	using ComponentType = std::uint8_t;
	const ComponentType kMaxComponents = 32; ///< Maximum number of component types
	
	constexpr ID kInvalidID	= std::numeric_limits<ID>::max();
	
	// TO-DO move to a different file and make sure to remove everything that is not part of the name itself
	template <typename T>
	constexpr std::string_view typeName()
	{
#if defined(__clang__)
		constexpr std::string_view p = __PRETTY_FUNCTION__;
		constexpr std::string_view prefix = "T = ";
		constexpr std::string_view suffix = "]";
#elif defined(__GNUC__)
		constexpr std::string_view p = __PRETTY_FUNCTION__;
		constexpr std::string_view prefix = "T = ";
		constexpr std::string_view suffix = "]";
#elif defined(_MSC_VER)
		constexpr std::string_view p = __FUNCSIG__;
		constexpr std::string_view prefix = "typeName<";
		constexpr std::string_view suffix = ">(void)";
#else
#   error Unsupported compiler
#endif

		const auto start = p.find(prefix) + prefix.size();
		const auto end = p.rfind(suffix);
		return p.substr(start, end - start);
	}

	template <typename T>
	struct TypeName {
		static constexpr std::string_view get() {
			return typeName<T>();
		}
	};
	
	// Keyboard key codes
	using KeyCode = uint16_t;
	enum : KeyCode
	{
		// Printable keys
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */

		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */

		Semicolon = 59, /* ; */
		Equal = 61, /* = */

		A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72,
		I = 73, J = 74, K = 75, L = 76, M = 77, N = 78, O = 79, P = 80,
		Q = 81, R = 82, S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
		Y = 89, Z = 90,

		LeftBracket = 91,  /* [ */
		Backslash = 92,  /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,  /* ` */

		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */

		/* Function keys */
		EscapeBar = 256, Enter = 257, Tab = 258, Backspace = 259,
		Insert = 260, Delete = 261, Right = 262, Left = 263,
		Down = 264, Up = 265, PageUp = 266, PageDown = 267,
		Home = 268, End = 269, CapsLock = 280, ScrollLock = 281,
		NumLock = 282, PrintScreen = 283, Pause = 284,
		F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
		F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
		F13 = 302, F14 = 303, F15 = 304, F16 = 305, F17 = 306, F18 = 307,
		F19 = 308, F20 = 309, F21 = 310, F22 = 311, F23 = 312, F24 = 313,
		F25 = 314,

		/* Keypad */
		KP0 = 320, KP1 = 321, KP2 = 322, KP3 = 323, KP4 = 324,
		KP5 = 325, KP6 = 326, KP7 = 327, KP8 = 328, KP9 = 329,
		KPDecimal = 330, KPDivide = 331, KPMultiply = 332, KPSubtract = 333,
		KPAdd = 334, KPEnter = 335, KPEqual = 336,

		LeftShift = 340, LeftControl = 341, LeftAlt = 342, LeftSuper = 343,
		RightShift = 344, RightControl = 345, RightAlt = 346, RightSuper = 347,
		Menu = 348
	};

	// Mouse button codes
	using MouseCode = u16;
	enum : MouseCode
	{
		Button0 = 0, Button1 = 1, Button2 = 2, Button3 = 3,
		Button4 = 4, Button5 = 5, Button6 = 6, Button7 = 7,

		ButtonLast = Button7,
		ButtonLeft = Button0,
		ButtonRight = Button1,
		ButtonMiddle = Button2
	};

	/// Types of vertex/constant buffer data
	enum class ShaderStage
	{
		Fragment = 0,
		Vertex,
		// ...
		Invalid
	};
	
	/**
	 * @brief Supported light types.
	 */
	enum class LightType : s32
	{
		DirectionalLight = 0, /// Directional light source.
		PointLight = 1,				/// Omnidirectional point light.
		SpotLight = 2				/// Spotlight with cutoff angles.
	};
	
	struct GPULight {
		glm::vec3 position;     float constant_att;   // 16
		glm::vec3 ambient;      float linear_att;     // 16
		glm::vec3 diffuse;      float quadratic_att;  // 16
		glm::vec3 specular;     float cut_off;        // 16
		glm::vec3 direction;    float outer_cut_off;  // 16
		glm::vec3 pad;					LightType type;       // 16
	};                                              
	
	struct LightList {
		std::vector<GPULight> lights;
	};
	
	enum UniformType {
		Byte1 = 0,
		Byte2,
		Byte3,
		Byte4,

		UByte1,
		UByte2,
		UByte3,
		UByte4,

		Short1,
		Short2,
		Short3,
		Short4,

		UShort1,
		UShort2,
		UShort3,
		UShort4,

		Int1,
		Int2,
		Int3,
		Int4,

		UInt1,
		UInt2,
		UInt3,
		UInt4,

		Float1,
		Float2,
		Float3,
		Float4,

		Double1,
		Double2,
		Double3,
		Double4,

		Mat2,
		Mat3,
		Mat4,

		Bool,
		Invalid
	};

	/// Returns the byte size of a given UniformType
	static uint32_t UniformTypeSize(UniformType type)
	{
		switch (type)
		{
			// Byte types
		case UniformType::Byte1:  return 1;
		case UniformType::UByte1: return 1;
		case UniformType::Byte2:  return 2;
		case UniformType::UByte2: return 2;
		case UniformType::Byte3:  return 3;
		case UniformType::UByte3: return 3;
		case UniformType::Byte4:  return 4;
		case UniformType::UByte4: return 4;

			// Short types
		case UniformType::Short1:  return 2;
		case UniformType::UShort1: return 2;
		case UniformType::Short2:  return 2 * 2;
		case UniformType::UShort2: return 2 * 2;
		case UniformType::Short3:  return 2 * 3;
		case UniformType::UShort3: return 2 * 3;
		case UniformType::Short4:  return 2 * 4;
		case UniformType::UShort4: return 2 * 4;

			// Float types
		case UniformType::Float1: return 4;
		case UniformType::Float2: return 4 * 2;
		case UniformType::Float3: return 4 * 3;
		case UniformType::Float4: return 4 * 4;

			// Double types
		case UniformType::Double1: return 8;
		case UniformType::Double2: return 8 * 2;
		case UniformType::Double3: return 8 * 3;
		case UniformType::Double4: return 8 * 4;

			// Int/UInt types
		case UniformType::UInt1:  return 4;
		case UniformType::UInt2:  return 4 * 2;
		case UniformType::UInt3:  return 4 * 3;
		case UniformType::UInt4:  return 4 * 4;
		case UniformType::Int1:   return 4;
		case UniformType::Int2:   return 4 * 2;
		case UniformType::Int3:   return 4 * 3;
		case UniformType::Int4:   return 4 * 4;

			// Matrix types
		case UniformType::Mat2: return 2 * 2 * 4;
		case UniformType::Mat3: return 3 * 3 * 4;
		case UniformType::Mat4: return 4 * 4 * 4;

			// Boolean
		case UniformType::Bool:   return 1;
		}

		return 0;
	}

	/// Texture and render formats
	enum class Format {
		F_None = 0,
		RGBA8 = 1,
		RGBA16F = 2,
		RGBA32F = 3,
		RG32F = 4,
		RGBA = 5,
		DEPTH32F = 6,
		DEPTH24STENCIL8 = 7,
		Depth = DEPTH24STENCIL8 ///< Default depth format
	};

	/// Texture filtering modes
	enum class Filter : u16 {
		NEAREST = 0,
		LINEAR,
		NEAREST_MIPMAP_NEAREST,
		LINEAR_MIPMAP_NEAREST,
		NEAREST_MIPMAP_LINEAR,
		LINEAR_MIPMAP_LINEAR
	};

	/// Texture wrapping modes
	enum class Wrap : u16 {
		REPEAT = 0,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE
	};

	/// Primitive/topology modes for drawing
	enum class TopologyEnum {
		PointList = 0,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan,
		LineListWithAdjacency,
		LineStripWithAdjacency,
		TriangleListWithAdjacency,
		TriangleStripWithAdjacency,
		PatchList
	};

	/// Polygon fill modes
	enum class PolyModeEnum
	{
		Fill,
		Line,
		Point,
		FillRectangleNV
	};

	/// Face culling modes
	enum class CullModeEnum
	{
		None,
		Front,
		Back,
		FrontAndBack
	};

	/// Front face winding order
	enum class FrontFaceEnum
	{
		eCounterClockwise,
		eClockwise
	};

	/// Logical operations (for blending or framebuffers)
	enum class LogicOpEnum
	{
		Clear, And, AndReverse, Copy, AndInverted, NoOp, Xor, Or, Nor,
		Equivalent, Invert, OrReverse, CopyInverted, OrInverted, Nand, Set
	};

	/*vk::PrimitiveTopology DrawModeEnumVulkan(Topology mode) {
		
		switch (mode)
		{
		case mam::Topology::PointList:
			return vk::PrimitiveTopology::ePointList; break;
		case mam::Topology::LineList:
			return vk::PrimitiveTopology::eLineList; break;
		case mam::Topology::LineStrip:
			return vk::PrimitiveTopology::eLineStrip; break;
		case mam::Topology::TriangleList:
			return vk::PrimitiveTopology::eTriangleList; break;
		case mam::Topology::TriangleStrip:
			return vk::PrimitiveTopology::eTriangleStrip; break;
		case mam::Topology::TriangleFan:
			return vk::PrimitiveTopology::eTriangleFan; break;
		case mam::Topology::LineListWithAdjacency:
			return vk::PrimitiveTopology::eLineListWithAdjacency; break;
		case mam::Topology::LineStripWithAdjacency:
			return vk::PrimitiveTopology::eLineStripWithAdjacency; break;
		case mam::Topology::TriangleListWithAdjacency:
			return vk::PrimitiveTopology::eTriangleListWithAdjacency; break;
		case mam::Topology::TriangleStripWithAdjacency:
			return vk::PrimitiveTopology::eTriangleStripWithAdjacency; break;
		case mam::Topology::PatchList:
			return vk::PrimitiveTopology::ePatchList; break;
		default:
			return vk::PrimitiveTopology::eTriangleList; break;
		}
	}*/
	

}