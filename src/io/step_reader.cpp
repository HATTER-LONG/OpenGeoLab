/**
 * @file step_reader.cpp
 * @brief Implementation of STEP file reader
 */

#include "step_reader.hpp"
#include "util/logger.hpp"

#include <BRep_Builder.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPControl_Reader.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDataStd_Name.hxx>
#include <TDocStd_Document.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>


#include <algorithm>
#include <filesystem>

namespace OpenGeoLab::IO {

ReadResult StepReader::readFile(const std::string& filePath,
                                ReadProgressCallback progressCallback) {
    LOG_INFO("Reading STEP file: {}", filePath);

    // Verify file exists
    if(!std::filesystem::exists(filePath)) {
        LOG_ERROR("STEP file not found: {}", filePath);
        return ReadResult::Failure("File not found: " + filePath);
    }

    if(progressCallback && !progressCallback(0.1, "Initializing STEP reader...")) {
        return ReadResult::Failure("Operation cancelled");
    }

    try {
        // Use XCAF reader for better support of assemblies and colors
        STEPCAFControl_Reader xcafReader;
        xcafReader.SetColorMode(true);
        xcafReader.SetNameMode(true);
        xcafReader.SetLayerMode(true);

        if(progressCallback && !progressCallback(0.2, "Reading STEP file...")) {
            return ReadResult::Failure("Operation cancelled");
        }

        // Read the file
        IFSelect_ReturnStatus status = xcafReader.ReadFile(filePath.c_str());
        if(status != IFSelect_RetDone) {
            LOG_ERROR("Failed to read STEP file: {} (status: {})", filePath,
                      static_cast<int>(status));
            return ReadResult::Failure("Failed to read STEP file");
        }

        if(progressCallback && !progressCallback(0.5, "Transferring geometry...")) {
            return ReadResult::Failure("Operation cancelled");
        }

        // Create an XCAF document to store the geometry
        Handle(TDocStd_Document) doc;
        Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
        app->NewDocument("MDTV-XCAF", doc);

        // Transfer shapes to document
        if(!xcafReader.Transfer(doc)) {
            LOG_ERROR("Failed to transfer STEP data to document");
            return ReadResult::Failure("Failed to transfer STEP data");
        }

        if(progressCallback && !progressCallback(0.7, "Processing shapes...")) {
            return ReadResult::Failure("Operation cancelled");
        }

        // Get the shape tool to access shapes
        Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());

        // Get all free shapes (top-level shapes not referenced by others)
        TDF_LabelSequence freeShapes;
        shapeTool->GetFreeShapes(freeShapes);

        if(freeShapes.Length() == 0) {
            LOG_WARN("No shapes found in STEP file: {}", filePath);
            return ReadResult::Failure("No geometry found in file");
        }

        // Combine all shapes into a compound if multiple top-level shapes
        TopoDS_Shape resultShape;
        if(freeShapes.Length() == 1) {
            resultShape = shapeTool->GetShape(freeShapes.Value(1));
        } else {
            BRep_Builder builder;
            TopoDS_Compound compound;
            builder.MakeCompound(compound);

            for(Standard_Integer i = 1; i <= freeShapes.Length(); ++i) {
                TopoDS_Shape shape = shapeTool->GetShape(freeShapes.Value(i));
                if(!shape.IsNull()) {
                    builder.Add(compound, shape);
                }
            }
            resultShape = compound;
        }

        if(resultShape.IsNull()) {
            LOG_ERROR("Result shape is null after reading STEP file");
            return ReadResult::Failure("Failed to extract geometry");
        }

        if(progressCallback && !progressCallback(0.9, "Creating part entity...")) {
            return ReadResult::Failure("Operation cancelled");
        }

        // Create the part entity
        auto part = std::make_shared<Geometry::PartEntity>(resultShape);

        // Extract filename for part name
        std::filesystem::path path(filePath);
        part->setName(path.stem().string());
        part->setFilePath(filePath);

        // Build the entity hierarchy
        part->buildHierarchy();

        if(progressCallback) {
            progressCallback(1.0, "STEP file loaded successfully");
        }

        LOG_INFO("Successfully loaded STEP file: {} ({} solids)", filePath, part->solids().size());

        return ReadResult::Success(part);

    } catch(const Standard_Failure& e) {
        std::string error = e.GetMessageString() ? e.GetMessageString() : "Unknown OCC error";
        LOG_ERROR("OCC exception reading STEP file: {}", error);
        return ReadResult::Failure("OCC error: " + error);
    } catch(const std::exception& e) {
        LOG_ERROR("Exception reading STEP file: {}", e.what());
        return ReadResult::Failure(std::string("Error: ") + e.what());
    }
}

bool StepReader::canRead(const std::string& filePath) const {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".step" || ext == ".stp" || ext == ".p21";
}

std::vector<std::string> StepReader::supportedExtensions() const {
    return {".step", ".stp", ".p21"};
}

std::string StepReader::description() const { return "STEP (ISO 10303-21) file reader"; }

} // namespace OpenGeoLab::IO