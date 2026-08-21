#include "render/api/river.hpp"
#include <algorithm>
#include <limits>
#include <cstdlib>

namespace mam {

  namespace {
    inline int idx(int x, int z, int w) { return z * w + x; }

    inline float octile(glm::ivec2 a, glm::ivec2 b) {
      int dx = std::abs(a.x - b.x);
      int dz = std::abs(a.y - b.y);
      int dmin = std::min(dx, dz);
      int dmax = std::max(dx, dz);
      const float SQRT2 = 1.41421356f;
      return float(dmax - dmin) + SQRT2 * float(dmin);
    }

    void appendLine(std::vector<glm::ivec2>& out, glm::ivec2 a, glm::ivec2 b) {
      int dx = std::abs(b.x - a.x);
      int dz = std::abs(b.y - a.y);
      int steps = std::max(dx, dz);
      if (steps == 0) { out.push_back(b); return; }
      for (int s = 1; s <= steps; ++s) {
        float t = float(s) / steps;
        out.push_back({ int(std::lround(a.x + (b.x - a.x) * t)),
                        int(std::lround(a.y + (b.y - a.y) * t)) });
      }
    }
  }

  std::vector<glm::ivec2> computeRiverPath(
    const std::vector<float>& field, int w, int h,
    glm::ivec2 source, glm::ivec2 sink,
    const RiverParams& params)
  {
    std::vector<glm::ivec2> path;
    if (field.empty() || w <= 0 || h <= 0) return path;

    auto inBounds = [&](glm::ivec2 c) {
      return c.x >= 0 && c.x < w && c.y >= 0 && c.y < h;
      };
    if (!inBounds(source) || !inBounds(sink)) return path;

    const size_t N = static_cast<size_t>(w) * h;
    const float  INF = std::numeric_limits<float>::max();

    std::vector<float> gScore(N, INF);
    std::vector<int>   cameFrom(N, -1);
    std::vector<char>  closed(N, 0);

    struct Node { float f; int i; };
    struct Cmp { bool operator()(const Node& a, const Node& b) const { return a.f > b.f; } };
    std::priority_queue<Node, std::vector<Node>, Cmp> open;

    const int si = idx(source.x, source.y, w);
    const int ti = idx(sink.x, sink.y, w);

    gScore[si] = 0.0f;
    open.push({ params.heuristicWeight * octile(source, sink), si });

    const int OX[8] = { 1, -1,  0,  0,  1,  1, -1, -1 };
    const int OZ[8] = { 0,  0,  1, -1,  1, -1,  1, -1 };

    while (!open.empty()) {
      Node cur = open.top();
      open.pop();

      if (closed[cur.i]) continue;
      closed[cur.i] = 1;

      if (cur.i == ti) break;

      int   cx = cur.i % w;
      int   cz = cur.i / w;
      float curH = field[cur.i];
      float curG = gScore[cur.i];

      for (int n = 0; n < 8; ++n) {
        int nx = cx + OX[n];
        int nz = cz + OZ[n];
        if (nx < 0 || nx >= w || nz < 0 || nz >= h) continue;

        int ni = idx(nx, nz, w);
        if (closed[ni]) continue;

        float base = (OX[n] != 0 && OZ[n] != 0) ? 1.41421356f : 1.0f;

        float dh = field[ni] - curH;
        float stepCost = base;
        if (dh > 0.0f) stepCost += params.uphillPenalty * dh;

        float tentativeG = curG + stepCost;
        if (tentativeG < gScore[ni]) {
          gScore[ni] = tentativeG;
          cameFrom[ni] = cur.i;
          float fScore = tentativeG +
            params.heuristicWeight * octile({ nx, nz }, sink);
          open.push({ fScore, ni });
        }
      }
    }

    if (ti != si && cameFrom[ti] == -1) return path; 

    for (int c = ti; c != -1; c = cameFrom[c])
      path.push_back({ c % w, c / w });
    std::reverse(path.begin(), path.end());
    return path;
  }

  RiverResult buildRiver(
    std::vector<float>& field, int w, int h,
    glm::ivec2 source, glm::ivec2 sink,
    const RiverParams& params,
    const RiverCarveParams& carve,
    float cellSize)
  {
    RiverResult result;

    int stride = std::max(1, carve.pathStride);
    int cw = (w + stride - 1) / stride;
    int ch = (h + stride - 1) / stride;
    std::vector<float> coarse(static_cast<size_t>(cw) * ch);
    for (int z = 0; z < ch; ++z)
      for (int x = 0; x < cw; ++x) {
        int sx = std::min(x * stride, w - 1);
        int sz = std::min(z * stride, h - 1);
        coarse[static_cast<size_t>(z) * cw + x] = field[static_cast<size_t>(sz) * w + sx];
      }

    glm::ivec2 cs{ source.x / stride, source.y / stride };
    glm::ivec2 ct{ sink.x / stride, sink.y / stride };

    auto cpath = computeRiverPath(coarse, cw, ch, cs, ct, params);
    if (cpath.empty()) return result;

    std::vector<glm::ivec2>& path = result.path;
    path.push_back({ cpath[0].x * stride, cpath[0].y * stride });
    for (size_t i = 1; i < cpath.size(); ++i)
      appendLine(path, { cpath[i - 1].x * stride, cpath[i - 1].y * stride },
        { cpath[i].x * stride, cpath[i].y * stride });
    for (auto& p : path) {
      p.x = std::clamp(p.x, 0, w - 1);
      p.y = std::clamp(p.y, 0, h - 1);
    }

    std::vector<float> orig = field;
    result.mask.assign(static_cast<size_t>(w) * h, 0.0f);

    std::vector<float>& bed = result.bed;
    bed.resize(path.size());
    float running = orig[static_cast<size_t>(path[0].y) * w + path[0].x] - carve.depth;
    for (size_t i = 0; i < path.size(); ++i) {
      float target = orig[static_cast<size_t>(path[i].y) * w + path[i].x] - carve.depth;
      running = std::min(running, target);
      bed[i] = running;
    }

    float radius = (carve.width * 0.5f) / cellSize;
    int   R = std::max(1, int(std::ceil(radius)));
    for (size_t i = 0; i < path.size(); ++i) {
      glm::ivec2 c = path[i];
      float bedH = bed[i];
      for (int dz = -R; dz <= R; ++dz)
        for (int dx = -R; dx <= R; ++dx) {
          int nx = c.x + dx, nz = c.y + dz;
          if (nx < 0 || nx >= w || nz < 0 || nz >= h) continue;
          float dist = std::sqrt(float(dx * dx + dz * dz));
          if (dist > radius) continue;
          float k = dist / radius;
          float s = k * k * (3.0f - 2.0f * k);
          size_t id = static_cast<size_t>(nz) * w + nx;

          float riverness = 1.0f - s;
          result.mask[id] = std::max(result.mask[id], riverness);

          float bank = orig[id];
          float t = bedH * (1.0f - s) + bank * s;
          field[id] = std::min(field[id], t);
        }
    }
    return result;
  }

  void buildWaterMesh(const std::vector<glm::ivec2>& path,
    const std::vector<float>& bed,
    float cellSize,
    const WaterMeshParams& wp,
    std::vector<Vertex>& outVerts,
    std::vector<unsigned int>& outIndices)
  {
    outVerts.clear();
    outIndices.clear();
    const size_t n = path.size();
    if (n < 2 || bed.size() != n) return;

    const float halfW = wp.waterWidth * 0.5f;
    const int   K = 3;

    for (size_t i = 0; i < n; ++i) {
      glm::vec2 cur{ path[i].x * cellSize, path[i].y * cellSize };

      size_t ia = (i > size_t(K)) ? i - K : 0;
      size_t ib = (i + K < n) ? i + K : n - 1;
      glm::vec2 a{ path[ia].x * cellSize, path[ia].y * cellSize };
      glm::vec2 b{ path[ib].x * cellSize, path[ib].y * cellSize };
      glm::vec2 dir = b - a;
      if (glm::dot(dir, dir) < 1e-6f) dir = glm::vec2(1.0f, 0.0f);
      dir = glm::normalize(dir);

      glm::vec2 perp{ -dir.y, dir.x };
      float y = bed[i] + wp.waterRaise;

      glm::vec2 L = cur - perp * halfW;
      glm::vec2 R = cur + perp * halfW;
      float vCoord = float(i);

      Vertex vl;
      vl.position = glm::vec3(L.x, y, L.y);
      vl.normal = glm::vec3(0.0f, 1.0f, 0.0f);
      vl.texCoord = glm::vec2(0.0f, vCoord);
      vl.tangent = glm::vec4(dir.x, 0.0f, dir.y, 1.0f);
      outVerts.push_back(vl);

      Vertex vr = vl;
      vr.position = glm::vec3(R.x, y, R.y);
      vr.texCoord = glm::vec2(1.0f, vCoord);
      outVerts.push_back(vr);
    }

    for (size_t i = 0; i + 1 < n; ++i) {
      unsigned int L0 = static_cast<unsigned int>(2 * i);
      unsigned int R0 = L0 + 1, L1 = L0 + 2, R1 = L0 + 3;

      outIndices.push_back(L0); outIndices.push_back(R0); outIndices.push_back(R1);
      outIndices.push_back(L0); outIndices.push_back(R1); outIndices.push_back(L1);
    }
  }

  bool River::build(std::vector<float>& field, int w, int h, glm::ivec2 source, glm::ivec2 sink, float cellSize)
  {
    RiverResult r = buildRiver(field, w, h, source, sink, params_, carve_, cellSize);
    path_ = std::move(r.path);
    bed_  = std::move(r.bed);
    mask_ = std::move(r.mask);
    return !path_.empty();
  }

}
