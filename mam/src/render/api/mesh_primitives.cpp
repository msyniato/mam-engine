#include "render/api/mesh.hpp"
#include "render/api/graphics_context.hpp"
#include "render/api/vertex_array.hpp"
#include "render/api/buffer.hpp"
#include "render/api/graphics_device.hpp"

#include "render/vulkan/vk_device.hpp"

#include "jobsys/dispatcher.hpp"
#include "mesh.cpp"

namespace mam
{

  void Mesh::createCube8v()
  {

    glm::vec3 pos[8] = {
      {+1.0f, +1.0f, +1.0f},
      {+1.0f, -1.0f, +1.0f},
      {-1.0f, -1.0f, +1.0f},
      {-1.0f, +1.0f, +1.0f},

      {+1.0f, +1.0f, -1.0f},
      {+1.0f, -1.0f, -1.0f},
      {-1.0f, -1.0f, -1.0f},
      {-1.0f, +1.0f, -1.0f},
    };

    glm::vec3 normals[8] = {
        {+0.577f, +0.577f, +0.577f}, // 0
        {+0.577f, -0.577f, +0.577f}, // 1
        {-0.577f, -0.577f, +0.577f}, // 2
        {-0.577f, +0.577f, +0.577f}, // 3

        {+0.577f, +0.577f, -0.577f}, // 4
        {+0.577f, -0.577f, -0.577f}, // 5
        {-0.577f, -0.577f, -0.577f}, // 6
        {-0.577f, +0.577f, -0.577f}, // 7
    };

    glm::vec2 texCoord[8] = {
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f}};

    for (int i = 0; i < 8; i++)
    {
      Vertex vertex;
      vertex.position = pos[i];
      vertex.normal = normals[i];
      vertex.texCoord = texCoord[i];
      vertices_.push_back(vertex);
    }

    unsigned int indices[36] = {
        2, 1, 0, 0, 3, 2,
        5, 6, 4, 6, 7, 4,
        5, 4, 1, 4, 0, 1,
        6, 2, 7, 2, 3, 7,
        5, 1, 2, 2, 6, 5,
        0, 4, 7, 7, 3, 0};

    for (int i = 0; i < 36; i++)
    {
      indices_.push_back(indices[i]);
    }

    initMesh();
  }

  void Mesh::createCube24v() {

    glm::vec3 pos[24] = {
         {+1.0f, +1.0f, +1.0f}, // 0
         {+1.0f, -1.0f, +1.0f}, // 1
         {-1.0f, -1.0f, +1.0f}, // 2
         {-1.0f, +1.0f, +1.0f}, // 3

         {+1.0f, +1.0f, -1.0f}, // 4
         {+1.0f, -1.0f, -1.0f}, // 5
         {-1.0f, -1.0f, -1.0f}, // 6
         {-1.0f, +1.0f, -1.0f}, // 7

         {+1.0f, +1.0f, +1.0f}, // 8  (0)
         {+1.0f, -1.0f, +1.0f}, // 9  (1)
         {+1.0f, -1.0f, -1.0f}, // 10 (5)
         {+1.0f, +1.0f, -1.0f}, // 11 (4)

         {-1.0f, -1.0f, +1.0f}, // 12 (2)
         {-1.0f, +1.0f, +1.0f}, // 13 (3)
         {-1.0f, +1.0f, -1.0f}, // 14 (7)
         {-1.0f, -1.0f, -1.0f}, // 15 (6)

         {+1.0f, +1.0f, +1.0f}, // 16 (0)
         {-1.0f, +1.0f, +1.0f}, // 17 (3)
         {-1.0f, +1.0f, -1.0f}, // 18 (7)
         {+1.0f, +1.0f, -1.0f}, // 19 (4)

         {+1.0f, -1.0f, +1.0f}, // 20 (1)
         {-1.0f, -1.0f, +1.0f}, // 21 (2)
         {-1.0f, -1.0f, -1.0f}, // 22 (6)
         {+1.0f, -1.0f, -1.0f}, // 23 (5)
    };

    glm::vec3 normals[24] = {
        {+0.0f, +0.0f, +1.0f}, // 0
        {+0.0f, +0.0f, +1.0f}, // 1
        {+0.0f, +0.0f, +1.0f}, // 2
        {+0.0f, +0.0f, +1.0f}, // 3

        {+0.0f, +0.0f, -1.0f}, // 4
        {+0.0f, +0.0f, -1.0f}, // 5
        {+0.0f, +0.0f, -1.0f}, // 6
        {+0.0f, +0.0f, -1.0f}, // 7

        {+1.0f, +0.0f, +0.0f}, // 8  (0)
        {+1.0f, +0.0f, +0.0f}, // 9  (1)
        {+1.0f, +0.0f, +0.0f}, // 10 (5)
        {+1.0f, +0.0f, +0.0f}, // 11 (4)

        {-1.0f, +0.0f, +0.0f}, // 12 (2)
        {-1.0f, +0.0f, +0.0f}, // 13 (3)
        {-1.0f, +0.0f, +0.0f}, // 14 (7)
        {-1.0f, +0.0f, +0.0f}, // 15 (6)

        {+0.0f, +1.0f, +0.0f}, // 16 (0)
        {+0.0f, +1.0f, +0.0f}, // 17 (3)
        {+0.0f, +1.0f, +0.0f}, // 18 (7)
        {+0.0f, +1.0f, +0.0f}, // 19 (4)

        {+0.0f, -1.0f, +0.0f}, // 20 (1)
        {+0.0f, -1.0f, +0.0f}, // 21 (2)
        {+0.0f, -1.0f, +0.0f}, // 22 (6)
        {+0.0f, -1.0f, +0.0f}, // 23 (5)
    };

    glm::vec2 texCoord[24] = {
        // Front face
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},

        // Back face
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},

        // Right face
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},

        // Left face
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},

        // Top face
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},

        // Bottom face
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},
    };

    unsigned int indices[] = {
        2,
        1,
        0,
        0,
        3,
        2,
        4,
        5,
        6,
        6,
        7,
        4,
        10,
        11,
        9,
        11,
        8,
        9,
        15,
        12,
        14,
        12,
        13,
        14,
        16,
        19,
        18,
        18,
        17,
        16,
        23,
        20,
        21,
        21,
        22,
        23,
    };

    for (int i = 0; i < 24; i++)
    {
      Vertex vertex;
      vertex.position = pos[i];
      vertex.normal = normals[i];
      vertex.texCoord = texCoord[i];
      vertices_.push_back(vertex);
    }

    for (int i = 0; i < 36; i++)
    {
      indices_.push_back(indices[i]);
    }

    initMesh();
  }

  void Mesh::createQuad() {

    glm::vec3 pos[4] = {
        {-1.0f, -1.0f, 0.0f},   //bottom left
        { 1.0f, -1.0f, 0.0f},   //bottom right
        { 1.0f,  1.0f, 0.0f},   //top right
        {-1.0f,  1.0f, 0.0f},   //top left
    };

    glm::vec3 normals[4] = {
        {0.0f, 0.0f, 1.0f}, // bottom left
        {0.0f, 0.0f, 1.0f}, // bottom right
        {0.0f, 0.0f, 1.0f}, // top right
        {0.0f, 0.0f, 1.0f}, // top left,
    };

    glm::vec2 texCoord[4] = {
        {0.0f, 0.0f}, // bottom left
        {1.0f, 0.0f}, // bottom right
        {1.0f, 1.0f}, // top right
        {0.0f, 1.0f}, // top left,
    };

    unsigned int indices[6] = {
        0,
        1,
        3,
        1,
        2,
        3,
    };

    for (int i = 0; i < 4; i++){
      Vertex vertex;
      vertex.position = pos[i];
      vertex.normal = normals[i];
      vertex.texCoord = texCoord[i];
      vertices_.push_back(vertex);
    }

    for (int i = 0; i < 6; i++) {
      indices_.push_back(indices[i]);
    }

    initMesh();
  }

  void Mesh::createSphere(const int num_heights,
                          const int num_revs) {

    if (num_heights == 0) return;
    if (num_revs == 0) return;

    float alpha = 3.14159265359f / num_heights;
    float om = 6.2831853071f / num_revs;

    for (int i = 0; i <= num_heights; i++)
    {
      for (int j = 0; j <= num_revs; j++)
      {

        float radio = cosf((alpha * i) - 1.57f);

        Vertex el;
        el.position = {
          cosf(om * j) * radio * 1.0f ,
          sinf((alpha * i) - 1.57f) * 1.0f,
          sinf(om * j) * radio * 1.0f
        };

        el.normal = {
            el.position.x / radio,
            el.position.y,
            el.position.z / radio};

        el.texCoord = {
            (1.0f / num_revs) * j,
            (1.0f / num_heights) * i};

        vertices_.push_back(el);
      }
    }

    for (int i = 0; i <= num_heights; i++)
    {
      for (int j = 0; j <= num_revs; j++)
      {

        // primer
        int up_left = i * (num_revs + 1) + j;                // 0
        int down_left = (i + 1) * (num_revs + 1) + j;        // 1
        int down_right = (i + 1) * (num_revs + 1) + (j + 1); // 2
        // segundo
        int up_left_2 = i * (num_revs + 1) + j;                // 0
        int down_right_2 = (i + 1) * (num_revs + 1) + (j + 1); // 1
        int up_right = i * (num_revs + 1) + (j + 1);           // 2

        indices_.push_back(up_left);
        indices_.push_back(down_left);
        indices_.push_back(down_right);

        indices_.push_back(up_left_2);
        indices_.push_back(down_right_2);
        indices_.push_back(up_right);
      }
    }

    initMesh();
  }

  void Mesh::CreateSurfaces(const float *points,
                            const int num_heights,
                            const int num_revs,
                            const float max_height)
  {

    if (points == nullptr) return;
    if (num_heights == 0) return;
    if (num_revs == 0) return;
    if (max_height == 0) return;

    float alpha = 3.14159265359f / num_heights;
    float om = 3.14159265359f * 2.0f / num_revs;

    for (int i = 0; i <= num_heights; i++)
    {
      for (int j = 0; j <= num_revs; j++)
      {

        Vertex el;
        el.position = {
            cosf(om * j) * points[i * 2],
            points[i * 2 + 1],
            sinf(om * j) * points[i * 2]};

        glm::vec3 norm = {0.0f, 0.0f, 0.0f};
        el.normal = {norm.x, norm.y, norm.z};
        el.texCoord = {((1.0f / num_revs) * j),
                       (1.0f / num_heights) * i};

        vertices_.push_back(el);
      }
    }

    // normals
    for (int i = 0; i <= num_heights; i++)
    {
      for (int j = 0; j <= num_revs; j++)
      {
        int index = i * (num_revs + 1) + j;
        int next_rev = (j + 1) % num_revs;
        int next_index = i * (num_revs + 1) + next_rev;

        if (i != (num_heights - 1))
        {
          int next_height = i + 1;
          int next_height_index = next_height * (num_revs + 1) + j;

          if (next_height >= 0 && next_height <= num_heights)
          {
            glm::vec3 current_pos = {
                vertices_[index].position.x,
                vertices_[index].position.y,
                vertices_[index].position.z};
            glm::vec3 next_pos = {
                vertices_[next_index].position.x,
                vertices_[next_index].position.y,
                vertices_[next_index].position.z};
            glm::vec3 vert_pos = {
                vertices_[next_height_index].position.x,
                vertices_[next_height_index].position.y,
                vertices_[next_height_index].position.z};

            glm::vec3 normal = normalPoint(current_pos, next_pos, vert_pos);
            vertices_[index].normal = {normal.x, normal.y, normal.z};
          }
          else
          {
            int prev_rev = (j - 1 + num_revs + 1) % (num_revs + 1);
            int prev_index = i * (num_revs + 1) + prev_rev;

            if (prev_index != index)
            {
              glm::vec3 current_pos = {
                  vertices_[index].position.x,
                  vertices_[index].position.y,
                  vertices_[index].position.z};
              glm::vec3 next_pos = {
                  vertices_[next_index].position.x,
                  vertices_[next_index].position.y,
                  vertices_[next_index].position.z};
              glm::vec3 prev_pos = {
                  vertices_[prev_index].position.x,
                  vertices_[prev_index].position.y,
                  vertices_[prev_index].position.z};

              glm::vec3 normal = normalPoint(current_pos, next_pos, prev_pos);
              vertices_[index].normal = {normal.x, normal.y, normal.z};
            }
            else
            {
              vertices_[index].normal = {0.0f, 1.0f, 0.0f};
            }
          }
        }
        else
        {
          vertices_[index].normal = {1.0f, 1.0f, 0.0f};
        }
      }
    }

    indices_.resize(num_revs * num_heights * 6);
    for (int i = 0; i < num_heights; i++)
    {
      for (int j = 0; j < num_revs; j++)
      {
        int index = (j + i * num_revs) * 6;
        // first triangle
        indices_[index + 0] = (i * (num_revs + 1)) + j;
        indices_[index + 1] = ((i + 1) * (num_revs + 1)) + j;
        indices_[index + 2] = ((i + 1) * (num_revs + 1)) + (j + 1);
        // second triangle
        indices_[index + 3] = (i * (num_revs + 1)) + j;
        indices_[index + 4] = ((i + 1) * (num_revs + 1)) + (j + 1);
        indices_[index + 5] = (i * (num_revs + 1)) + (j + 1);
      }
    }

    initMesh();
  }

} // namespace mam
