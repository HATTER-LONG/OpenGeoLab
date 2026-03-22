/**
 * @file translation_manager.h
 * @brief Declares the runtime QML singleton used to switch application language.
 */

#pragma once

#include <QObject>
#include <QString>
#include <QTranslator>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

/**
 * @brief Runtime language switcher exposed as a QML singleton.
 */
class TranslationManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString currentLanguage READ currentLanguage WRITE switchLanguage NOTIFY
                   currentLanguageChanged)

public:
    /**
     * @brief Create the singleton instance for the owning QML engine.
     * @param engine QML engine that owns the singleton and refreshes translated bindings.
     * @note This factory is invoked automatically by the QML engine for the singleton type.
     * @return Newly created singleton instance owned by the QML engine.
     */
    static auto create(QQmlEngine* engine, QJSEngine*) -> TranslationManager*;

    /**
     * @brief Return the currently active locale code.
     * @return Locale identifier exposed to QML, such as "en_US".
     */
    [[nodiscard]] auto currentLanguage() const -> QString;

    /**
     * @brief Switch the active runtime language and refresh QML translations.
     * @param locale Locale identifier to activate.
     */
    Q_INVOKABLE void switchLanguage(const QString& locale);

signals:
    void currentLanguageChanged();

private:
    explicit TranslationManager(QQmlEngine* engine, QObject* parent = nullptr);

    QQmlEngine* m_engine = nullptr;
    QTranslator m_translator;
    QString m_currentLanguage{"en_US"};
};
