/**
 * @file selection_service.hpp
 * @brief QML singleton bridging SelectionState to QML
 *
 * Connects to SelectionState signals (emitted from any thread) and
 * re-emits them as QML-safe signals via QMetaObject::invokeMethod.
 * Provides Q_INVOKABLE methods for pick mode control and selection management.
 */

#pragma once

#include <QObject>
#include <QVariantList>

#include <kangaroo/util/signal.hpp>

#include <vector>

namespace OpenGeoLab::Scene {
class SelectionState;
class LabelManager;
} // namespace OpenGeoLab::Scene

namespace OpenGeoLab::App {

/**
 * @brief QML singleton bridging SelectionState to QML.
 *
 * Registered via qmlRegisterSingletonInstance in main.cpp.
 * Call setSelectionState() after construction to wire signals.
 */
class SelectionService : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool pickEnabled READ pickEnabled WRITE setPickEnabled NOTIFY pickEnabledChanged)
    Q_PROPERTY(int pickMask READ pickMask WRITE setPickMask NOTIFY pickMaskChanged)
    Q_PROPERTY(QVariantList selections READ selections NOTIFY selectionChanged)
    Q_PROPERTY(bool labelsVisible READ labelsVisible NOTIFY labelsVisibleChanged)
    Q_PROPERTY(bool autoLabel READ autoLabel WRITE setAutoLabel NOTIFY autoLabelChanged)

public:
    explicit SelectionService(QObject* parent = nullptr);
    ~SelectionService() override;

    /**
     * @brief Wire this service to a SelectionState instance.
     * @param state SelectionState owned by SceneGraph. Must outlive this service.
     */
    void setSelectionState(Scene::SelectionState* state);

    /// Set the LabelManager for auto-label support. Must outlive this service.
    void setLabelManager(Scene::LabelManager* manager);

    [[nodiscard]] bool pickEnabled() const;
    void setPickEnabled(bool enabled);

    [[nodiscard]] int pickMask() const;
    void setPickMask(int mask);

    /** @brief Return current selections as QVariantList of QVariantMaps. */
    [[nodiscard]] QVariantList selections() const;

    /** @brief Activate pick mode with the given mask bitmask. */
    Q_INVOKABLE void activatePickMode(int mask);

    /** @brief Deactivate pick mode (disables picking). */
    Q_INVOKABLE void deactivatePickMode();

    /** @brief Clear all selections. */
    Q_INVOKABLE void clearSelection();

    /** @brief Remove a specific entity from selection. */
    Q_INVOKABLE void removeSelection(int shape_id, int entity_type, int local_id);

    /** @brief Add a label for the given entity. */
    Q_INVOKABLE void addLabelForSelection(int shapeId, int entityType, int localId);

    /** @brief Remove a label for the given entity. */
    Q_INVOKABLE void removeLabelForSelection(int shapeId, int entityType, int localId);

    /** @brief Show or hide all labels. */
    Q_INVOKABLE void setLabelsVisible(bool visible);

    [[nodiscard]] bool labelsVisible() const;
    [[nodiscard]] bool autoLabel() const;
    void setAutoLabel(bool enabled);

Q_SIGNALS:
    void entitySelected(int shapeId, int entityType, int localId);
    void entityDeselected(int shapeId, int entityType, int localId);
    void selectionCleared();
    void hoverChanged(int shapeId, int entityType, int localId);
    void pickEnabledChanged();
    void pickMaskChanged();
    void selectionChanged();
    void labelsVisibleChanged();
    void autoLabelChanged();

private:
    void connectSignals();
    void disconnectSignals();

    Scene::SelectionState* m_state{nullptr};
    Scene::LabelManager* m_labelManager{nullptr};
    std::vector<Kangaroo::Util::ScopedConnection> m_connections;
    bool m_labelsVisible{true};
    bool m_autoLabel{true};
};

} // namespace OpenGeoLab::App
