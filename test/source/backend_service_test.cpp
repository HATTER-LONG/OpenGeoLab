/**
 * @file backend_service_test.cpp
 * @brief Unit tests for BackendService JSON conversion utilities
 *
 * Tests the JSON/QVariantMap conversion functionality used by BackendService
 * for communication between QML and C++ service layer.
 */

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>


namespace {

/**
 * @brief Convert QVariantMap to nlohmann::json (standalone implementation for testing)
 * @param map Qt variant map from QML
 * @return JSON object representation
 */
nlohmann::json variantMapToJson(const QVariantMap& map) {
    nlohmann::json result;
    for(auto it = map.begin(); it != map.end(); ++it) {
        const QString& key = it.key();
        const QVariant& value = it.value();

        switch(value.typeId()) {
        case QMetaType::Bool:
            result[key.toStdString()] = value.toBool();
            break;
        case QMetaType::Int:
            result[key.toStdString()] = value.toInt();
            break;
        case QMetaType::Double:
            result[key.toStdString()] = value.toDouble();
            break;
        case QMetaType::QString:
            result[key.toStdString()] = value.toString().toStdString();
            break;
        case QMetaType::QVariantList: {
            nlohmann::json arr = nlohmann::json::array();
            const QVariantList list = value.toList();
            for(const QVariant& item : list) {
                if(item.typeId() == QMetaType::QVariantMap) {
                    arr.push_back(variantMapToJson(item.toMap()));
                } else if(item.typeId() == QMetaType::Double) {
                    arr.push_back(item.toDouble());
                } else if(item.typeId() == QMetaType::Int) {
                    arr.push_back(item.toInt());
                } else if(item.typeId() == QMetaType::QString) {
                    arr.push_back(item.toString().toStdString());
                } else if(item.typeId() == QMetaType::Bool) {
                    arr.push_back(item.toBool());
                }
            }
            result[key.toStdString()] = arr;
            break;
        }
        case QMetaType::QVariantMap:
            result[key.toStdString()] = variantMapToJson(value.toMap());
            break;
        default:
            result[key.toStdString()] = value.toString().toStdString();
            break;
        }
    }
    return result;
}

/**
 * @brief Convert nlohmann::json to QVariantMap (standalone implementation for testing)
 * @param json JSON object from service layer
 * @return Qt variant map for QML
 */
QVariantMap jsonToVariantMap(const nlohmann::json& json) {
    QVariantMap result;
    if(!json.is_object()) {
        return result;
    }

    for(auto it = json.begin(); it != json.end(); ++it) {
        const QString key = QString::fromStdString(it.key());
        const nlohmann::json& value = it.value();

        if(value.is_boolean()) {
            result[key] = value.get<bool>();
        } else if(value.is_number_integer()) {
            result[key] = value.get<int>();
        } else if(value.is_number_float()) {
            result[key] = value.get<double>();
        } else if(value.is_string()) {
            result[key] = QString::fromStdString(value.get<std::string>());
        } else if(value.is_array()) {
            QVariantList list;
            for(const auto& item : value) {
                if(item.is_object()) {
                    list.append(jsonToVariantMap(item));
                } else if(item.is_boolean()) {
                    list.append(item.get<bool>());
                } else if(item.is_number_integer()) {
                    list.append(item.get<int>());
                } else if(item.is_number_float()) {
                    list.append(item.get<double>());
                } else if(item.is_string()) {
                    list.append(QString::fromStdString(item.get<std::string>()));
                }
            }
            result[key] = list;
        } else if(value.is_object()) {
            result[key] = jsonToVariantMap(value);
        }
    }
    return result;
}

// ============================================================================
// Test Cases
// ============================================================================

TEST_CASE("QVariantMap to JSON - primitive types", "[backend_service]") {
    QVariantMap input;
    input["boolValue"] = true;
    input["intValue"] = 42;
    input["doubleValue"] = 3.14159;
    input["stringValue"] = QString("Hello World");

    nlohmann::json result = variantMapToJson(input);

    CHECK(result["boolValue"].get<bool>() == true);
    CHECK(result["intValue"].get<int>() == 42);
    CHECK(result["doubleValue"].get<double>() == 3.14159);
    CHECK(result["stringValue"].get<std::string>() == "Hello World");
}

TEST_CASE("QVariantMap to JSON - nested objects", "[backend_service]") {
    QVariantMap nested;
    nested["x"] = 1.0;
    nested["y"] = 2.0;
    nested["z"] = 3.0;

    QVariantMap input;
    input["name"] = QString("Point");
    input["coordinates"] = nested;

    nlohmann::json result = variantMapToJson(input);

    CHECK(result["name"].get<std::string>() == "Point");
    CHECK(result["coordinates"]["x"].get<double>() == 1.0);
    CHECK(result["coordinates"]["y"].get<double>() == 2.0);
    CHECK(result["coordinates"]["z"].get<double>() == 3.0);
}

TEST_CASE("QVariantMap to JSON - arrays", "[backend_service]") {
    QVariantList values;
    values.append(1);
    values.append(2);
    values.append(3);

    QVariantMap input;
    input["values"] = values;

    nlohmann::json result = variantMapToJson(input);

    CHECK(result["values"].is_array());
    CHECK(result["values"].size() == 3);
    CHECK(result["values"][0].get<int>() == 1);
    CHECK(result["values"][1].get<int>() == 2);
    CHECK(result["values"][2].get<int>() == 3);
}

TEST_CASE("JSON to QVariantMap - primitive types", "[backend_service]") {
    nlohmann::json input;
    input["boolValue"] = true;
    input["intValue"] = 42;
    input["doubleValue"] = 3.14159;
    input["stringValue"] = "Hello World";

    QVariantMap result = jsonToVariantMap(input);

    CHECK(result["boolValue"].toBool() == true);
    CHECK(result["intValue"].toInt() == 42);
    CHECK(result["doubleValue"].toDouble() == 3.14159);
    CHECK(result["stringValue"].toString() == "Hello World");
}

TEST_CASE("JSON to QVariantMap - nested objects", "[backend_service]") {
    nlohmann::json input;
    input["name"] = "Point";
    input["coordinates"] = {{"x", 1.0}, {"y", 2.0}, {"z", 3.0}};

    QVariantMap result = jsonToVariantMap(input);

    CHECK(result["name"].toString() == "Point");
    QVariantMap coords = result["coordinates"].toMap();
    CHECK(coords["x"].toDouble() == 1.0);
    CHECK(coords["y"].toDouble() == 2.0);
    CHECK(coords["z"].toDouble() == 3.0);
}

TEST_CASE("JSON to QVariantMap - arrays", "[backend_service]") {
    nlohmann::json input;
    input["values"] = {1, 2, 3};

    QVariantMap result = jsonToVariantMap(input);

    QVariantList values = result["values"].toList();
    CHECK(values.size() == 3);
    CHECK(values[0].toInt() == 1);
    CHECK(values[1].toInt() == 2);
    CHECK(values[2].toInt() == 3);
}

TEST_CASE("Round-trip conversion - complex object", "[backend_service]") {
    // Create a complex QVariantMap
    QVariantMap params;
    params["name"] = QString("Box");
    params["originX"] = 0.0;
    params["originY"] = 0.0;
    params["originZ"] = 0.0;
    params["width"] = 10.0;
    params["height"] = 20.0;
    params["depth"] = 5.0;
    params["keepOriginal"] = true;

    // Convert to JSON and back
    nlohmann::json json = variantMapToJson(params);
    QVariantMap result = jsonToVariantMap(json);

    // Verify round-trip preserves values
    CHECK(result["name"].toString() == "Box");
    CHECK(result["originX"].toDouble() == 0.0);
    CHECK(result["width"].toDouble() == 10.0);
    CHECK(result["height"].toDouble() == 20.0);
    CHECK(result["depth"].toDouble() == 5.0);
    CHECK(result["keepOriginal"].toBool() == true);
}

TEST_CASE("Empty input handling", "[backend_service]") {
    SECTION("Empty QVariantMap") {
        QVariantMap empty;
        nlohmann::json result = variantMapToJson(empty);
        // Empty QVariantMap converts to empty JSON object
        CHECK(result.empty());
    }

    SECTION("Empty JSON object") {
        nlohmann::json empty = nlohmann::json::object();
        QVariantMap result = jsonToVariantMap(empty);
        CHECK(result.isEmpty());
    }

    SECTION("JSON array (should return empty map)") {
        nlohmann::json arr = nlohmann::json::array();
        QVariantMap result = jsonToVariantMap(arr);
        CHECK(result.isEmpty());
    }
}

} // namespace
