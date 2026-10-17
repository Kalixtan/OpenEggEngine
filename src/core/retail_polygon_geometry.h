#pragma once

#include <box2d/box2d.h>
#include <cmath>
#include <vector>

inline void SetRetailPolygonGeometry(b2PolygonShape* polygon,
                                    const std::vector<b2Vec2>& vertices) {
    const int count = (int)vertices.size();
    polygon->m_count = count;
    polygon->m_centroid.SetZero();
    if (count == 2) {
        polygon->m_centroid = 0.5f * (vertices[0] + vertices[1]);
    } else if (count > 2) {
        b2Vec2 weighted(0.0f, 0.0f);
        float area = 0.0f;
        for (int i = 0; i < count; ++i) {
            const b2Vec2& current = vertices[(size_t)i];
            const b2Vec2& next = vertices[(size_t)((i + 1) % count)];
            const float cross = b2Cross(current, next);
            area += cross * 0.5f;
            weighted += (cross * (1.0f / 6.0f)) * (current + next);
        }
        const float centroidScale = (b2_epsilon - area < 0.0f)
            ? 1.0f / area : 4194304.0f; // retail's degenerate-area fallback
        polygon->m_centroid = centroidScale * weighted;
    }

    for (int i = 0; i < count; ++i) {
        polygon->m_vertices[i] = vertices[(size_t)i];
        const b2Vec2& next = vertices[(size_t)((i + 1) % count)];
        b2Vec2 normal(next.y - vertices[(size_t)i].y,
                      vertices[(size_t)i].x - next.x);
        const float lengthSquared = normal.LengthSquared();
        if (b2_epsilon <= std::sqrt(lengthSquared)) {
            float inverseLength = 1.0f / std::sqrt(lengthSquared);
            // Retail uses three Newton refinements for reciprocal length.
            for (int refine = 0; refine < 3; ++refine)
                inverseLength = -(lengthSquared * inverseLength * inverseLength - 1.0f)
                                * 0.5f * inverseLength + inverseLength;
            normal *= inverseLength;
        }
        polygon->m_normals[i] = normal;
    }
}
