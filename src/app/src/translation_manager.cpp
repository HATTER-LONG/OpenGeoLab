/**
 * @file translation_manager.cpp
 * @brief TranslationManager implementation — runtime language switching
 */

#include "opengeolab/app/translation_manager.hpp"

#include <QCoreApplication>
#include <QQmlEngine>

TranslationManager* TranslationManager::create(QQmlEngine* engine, QJSEngine*) {
    return new TranslationManager(engine, engine);
}

QString TranslationManager::currentLanguage() const { return m_currentLanguage; }

void TranslationManager::switchLanguage(const QString& locale) {
    if(locale == m_currentLanguage) {
        return;
    }

    QCoreApplication::removeTranslator(&m_translator);

    if(locale != "en_US") {
        const auto translator_path = QString(":/translations/opengeolab_%1.qm").arg(locale);
        if(!m_translator.load(translator_path)) {
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

    if(m_engine != nullptr) {
        m_engine->retranslate();
    }

    Q_EMIT currentLanguageChanged();
}

TranslationManager::TranslationManager(QQmlEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {}
