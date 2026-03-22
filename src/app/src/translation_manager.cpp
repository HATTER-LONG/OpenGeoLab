#include "opengeolab/app/translation_manager.h"

#include <QCoreApplication>
#include <QQmlEngine>

auto TranslationManager::create(QQmlEngine* engine, QJSEngine*) -> TranslationManager* {
    return new TranslationManager(engine, engine);
}

auto TranslationManager::currentLanguage() const -> QString { return m_currentLanguage; }

void TranslationManager::switchLanguage(const QString& locale) {
    if(locale == m_currentLanguage) {
        return;
    }

    QCoreApplication::removeTranslator(&m_translator);

    if(locale != "en_US") {
        // Keep English as the built-in fallback and only load external translators for overrides.
        const auto translator_path = QString(":/translations/opengeolab_%1.qm").arg(locale);
        if(!m_translator.load(translator_path)) {
            // Restore previous translator when the requested locale is unavailable.
            if(m_currentLanguage != "en_US") {
                const auto restore_path =
                    QString(":/translations/opengeolab_%1.qm").arg(m_currentLanguage);
                if(m_translator.load(restore_path)) {
                    QCoreApplication::installTranslator(&m_translator);
                }
            }
            return;
        }
        QCoreApplication::installTranslator(&m_translator);
    }

    m_currentLanguage = locale;

    // Retranslating the engine updates qsTr-bound QML text without recreating the scene.
    if(m_engine != nullptr) {
        m_engine->retranslate();
    }

    emit currentLanguageChanged();
}

TranslationManager::TranslationManager(QQmlEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {}
