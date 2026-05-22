// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_GLTF_IMPORTER_H
#define CURSEOFTHESEA_GLTF_IMPORTER_H

#include <string>
#include "engine/graphics/meshes/model_importer.h"

namespace cots::graphics::meshes
{
    //~ gltf parser fastgltf
    //~ and also converts to left handed
    class gltf_importer final : public model_importer
    {
    public:
         gltf_importer() = default;
        ~gltf_importer() override = default;

        gltf_importer           (const gltf_importer&) = delete;
        gltf_importer& operator=(const gltf_importer&) = delete;

        [[nodiscard]] bool import_model(std::string_view source_path,
                                        imported_model& out) override;
    };
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_GLTF_IMPORTER_H
