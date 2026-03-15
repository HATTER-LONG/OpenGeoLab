/**
 * @file UiSettingsController.hpp
 * @brief Provides application-level UI settings actions such as runtime language switching.
 */

#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QTranslator>

class QQmlEngine;

namespace OGL::App {

/**
 * @brief App-local controller for UI settings that need to be triggered from QML.
 */
class UiSettingsController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentLanguage READ currentLanguage NOTIFY currentLanguageChanged FINAL)

public:
    explicit UiSettingsController(QQmlEngine& engine, QObject* parent = nullptr);

    [[nodiscard]] const QString& currentLanguage() const;

    Q_INVOKABLE bool toggleLanguage();

signals:
    void currentLanguageChanged();

private:
    bool setCurrentLanguage(const QString& languageCode);

    QPointer<QQmlEngine> m_engine;
    QTranslator m_translator;
    QString m_currentLanguage = QStringLiteral("en_US");
};

} // namespace OGL::App
