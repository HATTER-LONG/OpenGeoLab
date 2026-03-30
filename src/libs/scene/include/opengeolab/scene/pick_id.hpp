/**
 * @file pick_id.hpp
 * @brief PickId 64-bit encoding and decoding for GPU color picking
 *
 * Encoding layout (MSB to LSB):
 *   bits [63..40]: shapeId  (24 bits, max 16'777'215)
 *   bits [39..8]:  localId  (32 bits)
 *   bits [7..0]:   EntityType (8 bits)
 */

#pragma once

#include <opengeolab/core/entity_tag.hpp>

#include <cassert>
#include <cstdint>

namespace OpenGeoLab::Scene {

/** @brief 64-bit GPU pick identifier encoding and decoding. */
struct PickId {
    /** @brief Encode shape, entity type, and local index into 64 bits. */
    static constexpr uint64_t encode(uint32_t shape_id, Core::EntityType type, uint32_t local_id) {
        assert(shape_id <= 0x00FF'FFFF && "shapeId exceeds 24-bit range");
        return (static_cast<uint64_t>(shape_id) << 40) | (static_cast<uint64_t>(local_id) << 8)
               | static_cast<uint64_t>(static_cast<uint8_t>(type));
    }

    /** @brief Extract EntityType from an encoded PickId. */
    static constexpr Core::EntityType decodeType(uint64_t encoded) {
        return static_cast<Core::EntityType>(encoded & 0xFF);
    }

    /** @brief Extract localId from an encoded PickId. */
    static constexpr uint32_t decodeLocalId(uint64_t encoded) {
        return static_cast<uint32_t>((encoded >> 8) & 0xFFFF'FFFF);
    }

    /** @brief Extract shapeId from an encoded PickId. */
    static constexpr uint32_t decodeShapeId(uint64_t encoded) {
        return static_cast<uint32_t>((encoded >> 40) & 0x00FF'FFFF);
    }

    /** @brief Check if the encoded value represents a valid pick target. */
    static constexpr bool isValid(uint64_t encoded) {
        return encoded != 0;
    }
};

} // namespace OpenGeoLab::Scene
