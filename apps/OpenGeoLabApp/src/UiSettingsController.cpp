#include <ogl/app/UiSettingsController.hpp>

#include <QCoreApplication>
#include <QQmlEngine>

namespace OGL::App {

UiSettingsController::UiSettingsController(QQmlEngine& engine, QObject* parent)
    : QObject(parent), m_engine(&engine) {}

const QString& UiSettingsController::currentLanguage() const { return m_currentLanguage; }

bool UiSettingsController::toggleLanguage() {
    return setCurrentLanguage(m_currentLanguage == QStringLiteral("zh_CN")
                                  ? QStringLiteral("en_US")
                                  : QStringLiteral("zh_CN"));
}

bool UiSettingsController::setCurrentLanguage(const QString& languageCode) {
    if(m_engine == nullptr) {
        return false;
    }

    const QString normalized_language =
        languageCode == QStringLiteral("zh_CN") ? QStringLiteral("zh_CN") : QStringLiteral("en_US");
    if(normalized_language == m_currentLanguage) {
        return true;
    }

    QCoreApplication::removeTranslator(&m_translator);
    if(normalized_language == QStringLiteral("zh_CN")) {
        if(!m_translator.load(QStringLiteral(":/i18n/opengeolab_zh_CN.qm"))) {
            return false;
        }

        QCoreApplication::installTranslator(&m_translator);
    }

    m_currentLanguage = normalized_language;
    m_engine->retranslate();
    emit currentLanguageChanged();
    return true;
}

} // namespace OGL::App
