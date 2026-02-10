#pragma once
#include <compare>
#include <concepts>
#include <functional>
#include <type_traits>

namespace OpenGeoLab::Util {

/**
 * ============================================================
 * CoreUidIdentity (Lightweight Reference)
 * ============================================================
 * Holds (uid, type) only. Defined first so that CoreIdentity
 * can declare an implicit conversion operator to it.
 */
template <typename UidT, typename TypeT, UidT InvalidUid, TypeT InvalidType>
    requires std::is_enum_v<TypeT> && std::totally_ordered<UidT>
struct CoreUidIdentity {
    UidT m_uid{InvalidUid};
    TypeT m_type{InvalidType};

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return m_uid != InvalidUid && m_type != InvalidType;
    }

    [[nodiscard]] constexpr bool operator==(const CoreUidIdentity& other) const noexcept {
        return m_uid == other.m_uid && m_type == other.m_type;
    }

    [[nodiscard]] constexpr bool operator!=(const CoreUidIdentity& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr auto operator<=>(const CoreUidIdentity& other) const noexcept {
        using Underlying = std::underlying_type_t<TypeT>;

        if(auto cmp = static_cast<Underlying>(m_type) <=> static_cast<Underlying>(other.m_type);
           cmp != 0) {
            return cmp;
        }

        if(auto cmp = m_uid <=> other.m_uid; cmp != 0) {
            return cmp;
        }

        return std::strong_ordering::equal;
    }
};

template <typename Key>
    requires requires(const Key& k) {
        { k.m_uid };
        { k.m_type };
    }
struct CoreUidIdentityHash {
    static constexpr void hashCombine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    [[nodiscard]] size_t operator()(const Key& key) const noexcept {
        size_t seed = 0;

        hashCombine(seed, std::hash<decltype(key.m_uid)>{}(key.m_uid));

        using U = std::underlying_type_t<decltype(key.m_type)>;

        hashCombine(seed, std::hash<U>{}(static_cast<U>(key.m_type)));

        return seed;
    }
};

/**
 * ============================================================
 * CoreIdentity (Full Key: id + uid + type)
 * ============================================================
 */
template <typename IdT,
          typename UidT,
          typename TypeT,
          IdT InvalidId,
          UidT InvalidUid,
          TypeT InvalidType>
    requires std::is_enum_v<TypeT> && std::totally_ordered<IdT> && std::totally_ordered<UidT>
struct CoreIdentity {
    IdT m_id{InvalidId};
    UidT m_uid{InvalidUid};
    TypeT m_type{InvalidType};

    [[nodiscard]] constexpr bool isValid() const noexcept {
        return m_id != InvalidId && m_uid != InvalidUid && m_type != InvalidType;
    }

    [[nodiscard]] constexpr bool operator==(const CoreIdentity& other) const noexcept {
        return m_id == other.m_id && m_uid == other.m_uid && m_type == other.m_type;
    }
    [[nodiscard]] constexpr bool operator!=(const CoreIdentity& other) const noexcept {
        return !(*this == other);
    }

    [[nodiscard]] constexpr auto operator<=>(const CoreIdentity& other) const noexcept {
        using Underlying = std::underlying_type_t<TypeT>;
        if(auto cmp = static_cast<Underlying>(m_type) <=> static_cast<Underlying>(other.m_type);
           cmp != 0) {
            return cmp;
        }
        if(auto cmp = m_id <=> other.m_id; cmp != 0) {
            return cmp;
        }
        if(auto cmp = m_uid <=> other.m_uid; cmp != 0) {
            return cmp;
        }
        return std::strong_ordering::equal;
    }

    /**
     * @brief Implicit conversion from CoreIdentity (Key) to CoreUidIdentity (Ref).
     *
     * Drops the m_id field, keeping only (uid, type). This allows EntityKey
     * to be passed wherever an EntityRef is expected.
     */
    [[nodiscard]] constexpr
    operator CoreUidIdentity<UidT, TypeT, InvalidUid, InvalidType>() const noexcept {
        return {m_uid, m_type};
    }
};

template <typename Key>
    requires requires(const Key& k) {
        { k.m_id };
        { k.m_uid };
        { k.m_type };
    }
struct CoreIdentityHash {
    static constexpr void hashCombine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    [[nodiscard]] size_t operator()(const Key& key) const noexcept {
        size_t seed = 0;
        hashCombine(seed, std::hash<decltype(key.m_id)>{}(key.m_id));
        hashCombine(seed, std::hash<decltype(key.m_uid)>{}(key.m_uid));
        using U = std::underlying_type_t<decltype(key.m_type)>;
        hashCombine(seed, std::hash<U>{}(static_cast<U>(key.m_type)));
        return seed;
    }
};

} // namespace OpenGeoLab::Util
