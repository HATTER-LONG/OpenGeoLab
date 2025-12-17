/**
 * @file hdf5_usage_example.cpp
 * @brief HDF5 usage examples (HighFive)
 */

#include <catch2/catch_test_macros.hpp>
#include <highfive/highfive.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace {
std::string getTempFilePath(const std::string& filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

TEST_CASE("HDF5 - Create and open file") {
    const std::string file_path = getTempFilePath("test_create.h5");

    {
        HighFive::File file(file_path, HighFive::File::Truncate);
        CHECK(file.isValid());
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);
        CHECK(file.isValid());
    }

    std::filesystem::remove(file_path);
}

TEST_CASE("HDF5 - Write and read 1D vector") {
    const std::string file_path = getTempFilePath("test_vector.h5");

    std::vector<double> write_data = {1.0, 2.0, 3.0, 4.0, 5.0};

    {
        HighFive::File file(file_path, HighFive::File::Truncate);
        file.createDataSet("my_dataset", write_data);
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);
        auto dataset = file.getDataSet("my_dataset");

        std::vector<double> read_data;
        dataset.read(read_data);

        REQUIRE(read_data.size() == write_data.size());
        for(size_t i = 0; i < write_data.size(); ++i) {
            CHECK(read_data[i] == write_data[i]);
        }
    }

    std::filesystem::remove(file_path);
}

TEST_CASE("HDF5 - Write and read 2D matrix") {
    const std::string file_path = getTempFilePath("test_matrix.h5");

    std::vector<std::vector<int>> write_data = {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};

    {
        HighFive::File file(file_path, HighFive::File::Truncate);
        file.createDataSet("matrix", write_data);
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);
        auto dataset = file.getDataSet("matrix");

        auto dims = dataset.getDimensions();
        REQUIRE(dims.size() == 2);
        CHECK(dims[0] == 3);
        CHECK(dims[1] == 4);

        std::vector<std::vector<int>> read_data;
        dataset.read(read_data);

        REQUIRE(read_data.size() == write_data.size());
        for(size_t i = 0; i < write_data.size(); ++i) {
            REQUIRE(read_data[i].size() == write_data[i].size());
            for(size_t j = 0; j < write_data[i].size(); ++j) {
                CHECK(read_data[i][j] == write_data[i][j]);
            }
        }
    }

    std::filesystem::remove(file_path);
}

TEST_CASE("HDF5 - Create and use groups") {
    const std::string file_path = getTempFilePath("test_groups.h5");

    {
        HighFive::File file(file_path, HighFive::File::Truncate);

        auto group1 = file.createGroup("simulation");
        auto group2 = group1.createGroup("results");

        std::vector<float> data1 = {1.0f, 2.0f, 3.0f};
        std::vector<float> data2 = {4.0f, 5.0f, 6.0f};

        group1.createDataSet("parameters", data1);
        group2.createDataSet("output", data2);
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);

        CHECK(file.exist("simulation"));
        CHECK(file.exist("simulation/results"));
        CHECK(file.exist("simulation/parameters"));
        CHECK(file.exist("simulation/results/output"));

        auto dataset = file.getDataSet("simulation/results/output");
        std::vector<float> read_data;
        dataset.read(read_data);

        CHECK(read_data.size() == 3);
        CHECK(read_data[0] == 4.0f);
    }

    std::filesystem::remove(file_path);
}

TEST_CASE("HDF5 - Create and read attributes") {
    const std::string file_path = getTempFilePath("test_attributes.h5");

    {
        HighFive::File file(file_path, HighFive::File::Truncate);

        std::vector<double> data = {1.0, 2.0, 3.0};
        auto dataset = file.createDataSet("data_with_attrs", data);

        dataset.createAttribute("description", std::string("Sample data"));
        dataset.createAttribute("version", 1);
        dataset.createAttribute("scale_factor", 0.5);

        file.createAttribute("created_by", std::string("OpenGeoLab"));
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);

        std::string created_by;
        file.getAttribute("created_by").read(created_by);
        CHECK(created_by == "OpenGeoLab");

        auto dataset = file.getDataSet("data_with_attrs");

        std::string description;
        dataset.getAttribute("description").read(description);
        CHECK(description == "Sample data");

        int version;
        dataset.getAttribute("version").read(version);
        CHECK(version == 1);

        double scale_factor;
        dataset.getAttribute("scale_factor").read(scale_factor);
        CHECK(scale_factor == 0.5);
    }

    std::filesystem::remove(file_path);
}

TEST_CASE("HDF5 - Chunked and compressed dataset") {
    const std::string file_path = getTempFilePath("test_compressed.h5");

    std::vector<double> large_data(10000);
    for(size_t i = 0; i < large_data.size(); ++i) {
        large_data[i] = static_cast<double>(i) * 0.1;
    }

    {
        HighFive::File file(file_path, HighFive::File::Truncate);

        HighFive::DataSetCreateProps props;
        props.add(HighFive::Chunking({1000}));
        props.add(HighFive::Deflate(6));

        file.createDataSet("compressed_data", large_data, props);
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);
        auto dataset = file.getDataSet("compressed_data");

        std::vector<double> read_data;
        dataset.read(read_data);

        REQUIRE(read_data.size() == large_data.size());
        CHECK(read_data[0] == large_data[0]);
        CHECK(read_data[9999] == large_data[9999]);
    }

    std::filesystem::remove(file_path);
}

TEST_CASE("HDF5 - Partial read with selection") {
    const std::string file_path = getTempFilePath("test_selection.h5");

    std::vector<std::vector<int>> matrix(10, std::vector<int>(10));
    for(int i = 0; i < 10; ++i) {
        for(int j = 0; j < 10; ++j) {
            matrix[i][j] = i * 10 + j;
        }
    }

    {
        HighFive::File file(file_path, HighFive::File::Truncate);
        file.createDataSet("large_matrix", matrix);
    }

    {
        HighFive::File file(file_path, HighFive::File::ReadOnly);
        auto dataset = file.getDataSet("large_matrix");

        std::vector<std::vector<int>> subset;
        dataset.select({2, 3}, {3, 4}).read(subset);

        REQUIRE(subset.size() == 3);
        REQUIRE(subset[0].size() == 4);

        CHECK(subset[0][0] == 23);
        CHECK(subset[2][3] == 46);
    }

    std::filesystem::remove(file_path);
}

} // namespace
