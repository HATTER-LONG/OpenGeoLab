/**
 * @file reader_test.cpp
 * @brief Unit tests for file readers
 */

#include <catch2/catch_test_macros.hpp>

#include "io/brep_reader.hpp"
#include "io/reader.hpp"
#include "io/step_reader.hpp"


using namespace OpenGeoLab::IO;

TEST_CASE("BrepReader interface", "[io][reader]") {
    BrepReader reader;

    SECTION("Supported extensions") {
        auto extensions = reader.supportedExtensions();
        CHECK(extensions.size() >= 2);

        bool hasBRep = false;
        bool hasBRP = false;
        for(const auto& ext : extensions) {
            if(ext == ".brep")
                hasBRep = true;
            if(ext == ".brp")
                hasBRP = true;
        }
        CHECK(hasBRep);
        CHECK(hasBRP);
    }

    SECTION("Can read BREP files") {
        CHECK(reader.canRead("model.brep"));
        CHECK(reader.canRead("model.BREP"));
        CHECK(reader.canRead("model.brp"));
        CHECK(reader.canRead("/path/to/model.brep"));
    }

    SECTION("Cannot read non-BREP files") {
        CHECK_FALSE(reader.canRead("model.step"));
        CHECK_FALSE(reader.canRead("model.stp"));
        CHECK_FALSE(reader.canRead("model.txt"));
    }

    SECTION("Description is not empty") { CHECK_FALSE(reader.description().empty()); }

    SECTION("Read non-existent file fails") {
        auto result = reader.readFile("non_existent_file.brep");
        CHECK_FALSE(result.success);
        CHECK_FALSE(result.errorMessage.empty());
        CHECK(result.part == nullptr);
    }
}

TEST_CASE("StepReader interface", "[io][reader]") {
    StepReader reader;

    SECTION("Supported extensions") {
        auto extensions = reader.supportedExtensions();
        CHECK(extensions.size() >= 2);

        bool hasStep = false;
        bool hasStp = false;
        for(const auto& ext : extensions) {
            if(ext == ".step")
                hasStep = true;
            if(ext == ".stp")
                hasStp = true;
        }
        CHECK(hasStep);
        CHECK(hasStp);
    }

    SECTION("Can read STEP files") {
        CHECK(reader.canRead("model.step"));
        CHECK(reader.canRead("model.STEP"));
        CHECK(reader.canRead("model.stp"));
        CHECK(reader.canRead("model.STP"));
        CHECK(reader.canRead("/path/to/model.step"));
    }

    SECTION("Cannot read non-STEP files") {
        CHECK_FALSE(reader.canRead("model.brep"));
        CHECK_FALSE(reader.canRead("model.txt"));
        CHECK_FALSE(reader.canRead("model.obj"));
    }

    SECTION("Description is not empty") { CHECK_FALSE(reader.description().empty()); }

    SECTION("Read non-existent file fails") {
        auto result = reader.readFile("non_existent_file.step");
        CHECK_FALSE(result.success);
        CHECK_FALSE(result.errorMessage.empty());
        CHECK(result.part == nullptr);
    }
}

TEST_CASE("ReadResult helpers", "[io][reader]") {
    SECTION("Success result") {
        auto part = std::make_shared<OpenGeoLab::Geometry::PartEntity>();
        auto result = ReadResult::Success(part);

        CHECK(result.success);
        CHECK(result.errorMessage.empty());
        CHECK(result.part != nullptr);
    }

    SECTION("Failure result") {
        auto result = ReadResult::Failure("Test error message");

        CHECK_FALSE(result.success);
        CHECK(result.errorMessage == "Test error message");
        CHECK(result.part == nullptr);
    }
}
