/**
 * @file translation_manager.hpp
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
     * @return Newly created singleton instance owned by the QML engine.
     */
    static TranslationManager* create(QQmlEngine* engine, QJSEngine*);

    /**
     * @brief Return the currently active locale code.
     * @return Locale identifier exposed to QML, such as "en_US".
     */
    [[nodiscard]] QString currentLanguage() const;

    /**
     * @brief Switch the active runtime language and refresh QML translations.
     * @param locale Locale identifier to activate.
     */
    Q_INVOKABLE void switchLanguage(const QString& locale);

Q_SIGNALS:
    void currentLanguageChanged();

private:
    explicit TranslationManager(QQmlEngine* engine, QObject* parent = nullptr);

    QQmlEngine* m_engine = nullptr;
    QTranslator m_translator;
    QString m_currentLanguage{"en_US"};
};
