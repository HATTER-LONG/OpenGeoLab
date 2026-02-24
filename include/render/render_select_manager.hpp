#pragma once

#include "render/render_types.hpp"
#include "util/signal.hpp"

#include <cstdint>
#include <kangaroo/util/noncopyable.hpp>

namespace OpenGeoLab::Render {

struct PickResult {
    uint64_t m_uid;          ///< Type-scoped unique identifier, Note: MeshNode only has node id
    RenderEntityType m_type; ///< Entity type for selection filtering

    bool operator==(const PickResult& other) const {
        return m_uid == other.m_uid && m_type == other.m_type;
    }
};

struct PickResultHash {
    static constexpr void hashCombine(size_t& seed, size_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }
    std::size_t operator()(const PickResult& result) const noexcept {
        std::size_t seed = 0;
        hashCombine(seed, result.m_uid);
        hashCombine(seed, static_cast<std::underlying_type_t<RenderEntityType>>(result.m_type));
        return seed;
    }
};

using PickResultSet = std::unordered_set<PickResult, PickResultHash>;

enum class SelectionChangeAction : uint8_t { Added = 0, Removed = 1, Cleared = 2 };

class RenderSelectManager : public Kangaroo::Util::NonCopyMoveable {
private:
    RenderSelectManager() = default;

public:
    static RenderSelectManager& instance();
    ~RenderSelectManager() = default;

    void setPickEnabled(bool enabled);
    bool isPickEnabled() const;

    void setPickTypes(RenderEntityTypeMask types);

    RenderEntityTypeMask getPickTypes() const;

    bool isTypePickable(RenderEntityType type) const;

    RenderEntityTypeMask normalizePickTypes(RenderEntityTypeMask types);

    bool addSelection(uint64_t entity_uid, RenderEntityType type);

    bool removeSelection(uint64_t entity_uid, RenderEntityType type);

    void clearSelection();

    std::vector<PickResult> selections() const;

    Util::ScopedConnection subscribePickEnabledChanged(std::function<void(bool)> callback) {
        return m_pickEnabledChanged.connect(std::move(callback));
    }

    Util::ScopedConnection
    subscribePickSettingsChanged(std::function<void(RenderEntityTypeMask)> callback) {
        return m_pickSettingsChanged.connect(std::move(callback));
    }

    Util::ScopedConnection
    subscribeSelectionChanged(std::function<void(PickResult, SelectionChangeAction)> callback) {
        return m_selectionChanged.connect(std::move(callback));
    }

private:
    mutable std::mutex m_mutex;
    bool m_pickEnabled{false};
    RenderEntityTypeMask m_pickTypes{RenderEntityTypeMask::None};

    PickResultSet m_currentSelections;

    Util::Signal<RenderEntityTypeMask> m_pickSettingsChanged;
    Util::Signal<bool> m_pickEnabledChanged;
    Util::Signal<PickResult, SelectionChangeAction> m_selectionChanged;
};
} // namespace OpenGeoLab::Render