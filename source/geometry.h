// geometry.h - 几何体数据定义
// 将顶点数据与渲染逻辑分离
#pragma once

#include <vector>

// 几何体数据基类
struct GeometryData {
    virtual ~GeometryData() = default;

    // 获取顶点数据
    virtual const float* vertices() const = 0;
    virtual int vertexCount() const = 0;

    // 获取索引数据(如果有)
    virtual const unsigned int* indices() const { return nullptr; }
    virtual int indexCount() const { return 0; }
};

// Squircle 几何数据 - 简单的四边形
class SquircleData : public GeometryData {
public:
    SquircleData() {
        // 四边形的四个顶点 (x, y)
        m_vertices = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
    }

    const float* vertices() const override { return m_vertices.data(); }
    int vertexCount() const override { return 4; }

private:
    std::vector<float> m_vertices;
};

// 立方体几何数据
class CubeData : public GeometryData {
public:
    CubeData() {
        // 每个顶点包含: 位置(x,y,z) + 法向量(nx,ny,nz) + 颜色(r,g,b)
        // 立方体的 8 个顶点,每个面使用不同的法向量
        // clang-format off
        m_vertices = {
            // 前面 (z = 0.5) - 法向量 (0, 0, 1)
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 0.0f,  // 左下
             0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 0.0f,  // 右下
             0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, 1.0f,  // 右上
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 0.0f,  // 左上

            // 后面 (z = -0.5) - 法向量 (0, 0, -1)
             0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  1.0f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f,  0.5f, 0.5f, 0.5f,

            // 顶面 (y = 0.5) - 法向量 (0, 1, 0)
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 0.5f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f,  0.5f, 1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  0.0f, 0.5f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.5f,

            // 底面 (y = -0.5) - 法向量 (0, -1, 0)
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.5f, 0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f, 0.0f,  0.0f, 0.5f, 0.5f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  1.0f, 0.0f, 0.5f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f, 0.0f,  0.5f, 1.0f, 0.5f,

            // 右面 (x = 0.5) - 法向量 (1, 0, 0)
             0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.5f, 0.5f,
             0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  0.5f, 1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.5f,
             0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  0.5f, 0.5f, 1.0f,

            // 左面 (x = -0.5) - 法向量 (-1, 0, 0)
            -0.5f, -0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.8f, 0.8f, 0.8f,
            -0.5f,  0.5f,  0.5f,  -1.0f, 0.0f, 0.0f,  0.6f, 0.6f, 0.6f,
            -0.5f,  0.5f, -0.5f,  -1.0f, 0.0f, 0.0f,  0.9f, 0.9f, 0.9f,
        };

        // 索引数据 - 每个面两个三角形
        m_indices = {
            0,  1,  2,   0,  2,  3,   // 前面
            4,  5,  6,   4,  6,  7,   // 后面
            8,  9,  10,  8,  10, 11,  // 顶面
            12, 13, 14,  12, 14, 15,  // 底面
            16, 17, 18,  16, 18, 19,  // 右面
            20, 21, 22,  20, 22, 23   // 左面
        };
        // clang-format on
    }

    const float* vertices() const override { return m_vertices.data(); }
    int vertexCount() const override { return 24; } // 6面 * 4顶点

    const unsigned int* indices() const override { return m_indices.data(); }
    int indexCount() const override { return 36; } // 6面 * 2三角形 * 3顶点

private:
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
};
