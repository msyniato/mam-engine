
#pragma once

#include "common/common.hpp"
#include "jobsys/job_system.hpp" 

namespace mam {

	class GraphicsDevice;
	class VertexBuffer;
	class VertexArray;
	class IndexBuffer;
	
	/**
	 * @brief Represents a 3D mesh with vertices and indices.
	 */
	class Mesh {
		
		friend class MeshRegistry;
	
	public:
		
		std::shared_ptr<VertexArray> vertexArray();

		/// Get the number of vertices
		std::vector<Vertex>& vertices() { return vertices_; };

		/// Get the number of indices
		u32 indices();
		
		std::string getName() const;
	
		/**
		 * @brief Construct a mesh with a name only.
		 * @param name Mesh name
		 */
		Mesh(GraphicsDevice& gd, const std::string& name);

		Mesh(GraphicsDevice& gd, const std::string& name, 
				 std::vector<Vertex> vertices,
				 std::vector<unsigned int> indices);

    /// Destructor
    ~Mesh();

    /**
     * @brief Load mesh from an OBJ file.
     * @param path Path to OBJ file
     */
    void LoadOBJ(const char* path);

    /**
     * @brief Load mesh from OBJ file on CPU (no GPU upload).
     * @param path Path to OBJ file
     */
    void LoadOBJ_CPU(const char* path);

    /// Upload vertex and index data to GPU
    void UploadToGPU();

    /// Create a cube with 8 vertices
    void createCube8v();

    /// Create a cube with 24 vertices
    void createCube24v();

    /// Create a simple quad
    void createQuad();

    /**
     * @brief Create a sphere mesh.
     * @param num_heights Number of horizontal segments (default 20)
     * @param num_revs Number of vertical segments (default 20)
     */
    void createSphere(const int num_heights = 20.0f,
      const int num_revs = 20.0f);

    /**
     * @brief Create surfaces from parametric points.
     * @param points Array of points
     * @param num_heights Number of heights
     * @param num_revs Number of revolutions
     * @param max_height Maximum height
     */
    void CreateSurfaces(const float* points,
      const int num_heights,
      const int num_revs,
      const float max_height);

    /// Initialize internal buffers
    void initMesh();

		
  protected:
		
		GraphicsDevice& graphicsDevice_;
		
		std::vector<Vertex> vertices_;
		std::vector<unsigned int> indices_;
		std::shared_ptr<VertexArray> vertexArray_;
		std::shared_ptr<VertexBuffer> vertexBuffer_;
		std::shared_ptr<IndexBuffer> indexBuffer_;
		std::string name_;

	};
	
	// Mesh registry
	class MeshRegistry {
  public:
		
    enum class LoadState : u8 {
      Pending,   
      Loading,   
      Uploading, 
      Ready,     
      Failed     
    };
 

    MeshRegistry(GraphicsDevice& gd, JobSystem& jobSystem);
    ~MeshRegistry() = default;
 
    MeshRegistry(const MeshRegistry&)            = delete;
    MeshRegistry& operator=(const MeshRegistry&) = delete;
		
    using MeshReadyCallback = std::function<void(ID id, Mesh* mesh)>;
		
    ID loadOBJAsync(const std::string&  path,
                    const std::string&  name,
                    MeshReadyCallback   onReady = nullptr);
 

    ID loadOBJSync(const std::string& path, const std::string& name);
		
 
    ID createFromData(const std::string& name,
                      std::vector<Vertex>       vertices,
                      std::vector<unsigned int> indices);
 
    ID createCube(const std::string& name, bool highQuality = true);
    ID createQuad(const std::string& name);
    ID createSphere(const std::string& name,
                    int num_heights = 20, int num_revs = 20);
		
    void drainPendingUploads();
		
    Mesh* getMesh(ID id);
    const Mesh* getMesh(ID id) const;
		
    Mesh* getMeshByName(const std::string& name);
 
    LoadState getLoadState(ID id) const;
 
    bool isReady   (ID id) const { return getLoadState(id) == LoadState::Ready;  }
    bool hasFailed (ID id) const { return getLoadState(id) == LoadState::Failed; }

    void destroy(ID id);
		
    void destroyAll();
		
    std::size_t count()      const { return entries_.size(); }
    std::size_t countReady() const;
 
  private:
 
    struct Entry {
      std::unique_ptr<Mesh> mesh;
      JobHandle             jobHandle;
      std::atomic<LoadState> state{ LoadState::Pending };
      MeshReadyCallback     onReady;
      std::string           errorMsg;
 
      Entry() = default;
    	
      Entry(Entry&&) = default;
      Entry& operator=(Entry&&) = default;
    };
 
    ID allocateID();
		
    ID submitOBJLoad(const std::string&      path,
                     const std::string&      name,
                     bool                    async,
                     MeshReadyCallback       onReady);
 
    GraphicsDevice& graphicsDevice_;
    JobSystem&      jobSystem_;
 
    std::unordered_map<ID, std::unique_ptr<Entry>> entries_;
    mutable std::mutex                              mutex_;   
    ID                                              nextID_{ 0 };
  };
	
}