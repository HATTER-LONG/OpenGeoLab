/**
 * @file label_manager.cpp
 * @brief LabelManager — thread-safe 3D label storage and visibility state
 */
#include <opengeolab/scene/label_manager.hpp>

#include <algorithm>
#include <mutex>
#include <utility>

namespace OpenGeoLab::Scene {

LabelManager::LabelManager() = default;
LabelManager::~LabelManager() = default;

void LabelManager::addLabel(Label3D label) {
    if(!label.entity.isValid()) {
        return;
    }

    {
        std::unique_lock lock(m_mutex);
        const auto it =
            std::find_if(m_labels.begin(), m_labels.end(), [&](const Label3D& existing_label) {
                return existing_label.entity == label.entity;
            });
        if(it != m_labels.end()) {
            *it = std::move(label);
        } else {
            m_labels.push_back(std::move(label));
        }
        ++m_version;
    }

    labelsChanged.emit();
}

void LabelManager::removeByEntity(const Core::EntityRef& entity) {
    {
        std::unique_lock lock(m_mutex);
        const auto it = std::remove_if(m_labels.begin(), m_labels.end(), [&](const Label3D& label) {
            return label.entity == entity;
        });
        if(it == m_labels.end()) {
            return;
        }

        m_labels.erase(it, m_labels.end());
        ++m_version;
    }

    labelsChanged.emit();
}

std::size_t LabelManager::removeWhere(const std::function<bool(const Label3D&)>& predicate) {
    std::size_t removed = 0;
    {
        std::unique_lock lock(m_mutex);
        const auto previous_size = m_labels.size();
        std::erase_if(m_labels, predicate);
        removed = previous_size - m_labels.size();
        if(removed == 0) {
            return 0;
        }
        ++m_version;
    }
    labelsChanged.emit();
    return removed;
}

void LabelManager::clearLabels() {
    {
        std::unique_lock lock(m_mutex);
        if(m_labels.empty()) {
            return;
        }

        m_labels.clear();
        ++m_version;
    }

    labelsChanged.emit();
}

std::vector<Label3D> LabelManager::labels() const {
    std::shared_lock lock(m_mutex);
    return m_labels;
}

uint64_t LabelManager::version() const noexcept {
    return m_version.load(std::memory_order_acquire);
}

void LabelManager::setVisible(bool visible) {
    if(m_visible.load(std::memory_order_acquire) != visible) {
        m_visible.store(visible, std::memory_order_release);
        visibleChanged.emit();
    }
}

bool LabelManager::isVisible() const noexcept { return m_visible.load(std::memory_order_acquire); }

void LabelManager::setAutoLabel(bool enabled) {
    if(m_autoLabel.load(std::memory_order_acquire) != enabled) {
        m_autoLabel.store(enabled, std::memory_order_release);
        autoLabelChanged.emit();
    }
}

bool LabelManager::autoLabel() const noexcept {
    return m_autoLabel.load(std::memory_order_acquire);
}

} // namespace OpenGeoLab::Scene
