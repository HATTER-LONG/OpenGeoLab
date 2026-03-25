/**
 * @file request_service_test.cpp
 * @brief Tests direct geometry routing in RequestService.
 */

#include <opengeolab/app/progress_tracker.hpp>
#include <opengeolab/app/request_service.hpp>
#include <opengeolab/geometry/geometry_module.hpp>
#include <opengeolab/python/embedded_python_runtime.hpp>
#include <opengeolab/scene/scene_graph.hpp>

#include <doctest/doctest.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

#include <filesystem>
#include <memory>

namespace {

QCoreApplication& testApplication() {
    static int argc = 1;
    static char app_name[] = "opengeolab_app_request_service_test";
    static char* argv[] = {app_name, nullptr};
    static QCoreApplication app(argc, argv);
    return app;
}

OpenGeoLab::Python::EmbeddedPythonRuntime& sharedRuntime() {
    static auto runtime = []() {
        const auto project_root = std::filesystem::path(OPENGEOLAB_PROJECT_SOURCE_DIR);
        const auto runtime_root = project_root / "src" / "app" / "resource" / "python";
        const auto plugin_root = project_root / "plugins";
        return std::make_unique<OpenGeoLab::Python::EmbeddedPythonRuntime>(
            project_root, runtime_root, plugin_root);
    }();
    return *runtime;
}

} // namespace

TEST_CASE("RequestService executes geometry requests on the main thread through GeometryModule") {
    testApplication();

    OpenGeoLab::App::ProgressTracker progress_tracker;
    OpenGeoLab::App::RequestService service(sharedRuntime(), progress_tracker);
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Geometry::GeometryModule geometry_module(graph);

    service.setGeometryModule(&geometry_module);

    QString response_request_id;
    QString response_json;
    QString error_message;
    QObject::connect(&service, &OpenGeoLab::App::RequestService::responseReady, &service,
                     [&](const QString& request_id, const QString& json) {
                         response_request_id = request_id;
                         response_json = json;
                     });
    QObject::connect(&service, &OpenGeoLab::App::RequestService::errorOccurred, &service,
                     [&](const QString&, const QString& error) { error_message = error; });

    const QString request_json = QStringLiteral(
        R"({"module":"geometry","action":"create_box","center":[0,0,0],"size":[1,2,3]})");

    const QString request_id = service.executeOnMainThread(request_json);

    CHECK(error_message.isEmpty());
    CHECK(response_request_id == request_id);
    REQUIRE_FALSE(response_json.isEmpty());

    const auto response = QJsonDocument::fromJson(response_json.toUtf8()).object();
    CHECK(response.value(QStringLiteral("ok")).toBool(false));
    CHECK(graph.root().children.size() == 1);
}

TEST_CASE("RequestService submitAsync routes geometry requests directly to GeometryModule") {
    testApplication();

    OpenGeoLab::App::ProgressTracker progress_tracker;
    OpenGeoLab::App::RequestService service(sharedRuntime(), progress_tracker);
    OpenGeoLab::Scene::SceneGraph graph;
    OpenGeoLab::Geometry::GeometryModule geometry_module(graph);

    service.setGeometryModule(&geometry_module);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    QString response_request_id;
    QString response_json;
    QString error_message;

    QObject::connect(&service, &OpenGeoLab::App::RequestService::responseReady, &service,
                     [&](const QString& request_id, const QString& json) {
                         response_request_id = request_id;
                         response_json = json;
                         loop.quit();
                     });
    QObject::connect(&service, &OpenGeoLab::App::RequestService::errorOccurred, &service,
                     [&](const QString&, const QString& error) {
                         error_message = error;
                         loop.quit();
                     });
    QObject::connect(&timeout, &QTimer::timeout, &service, [&]() {
        error_message = QStringLiteral("Timed out");
        loop.quit();
    });

    const QString request_json = QStringLiteral(
        R"({"module":"geometry","action":"create_sphere","center":[0,0,0],"radius":2.5})");

    const QString request_id = service.submitAsync(request_json);
    timeout.start(5000);
    loop.exec();

    CHECK(error_message.isEmpty());
    CHECK(response_request_id == request_id);
    REQUIRE_FALSE(response_json.isEmpty());

    const auto response = QJsonDocument::fromJson(response_json.toUtf8()).object();
    CHECK(response.value(QStringLiteral("ok")).toBool(false));
    CHECK_FALSE(service.isBusy());
    CHECK(graph.root().children.size() == 1);
}
