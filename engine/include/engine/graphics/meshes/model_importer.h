// Created by Niffoxic (Harsh Dubey)
#ifndef CURSEOFTHESEA_MODEL_IMPORTER_H
#define CURSEOFTHESEA_MODEL_IMPORTER_H

#include <string_view>

#include "engine/graphics/meshes/imported_model.h"

namespace cots::graphics::meshes
{
    //~ swappable parse policy
    class model_importer
    {
    public:
        virtual ~model_importer() = default;

        //~ source path to mesh
        [[nodiscard]] virtual bool import_model(std::string_view source_path,
                                                imported_model& out) = 0;
    };
} // namespace cots::graphics::meshes

#endif //CURSEOFTHESEA_MODEL_IMPORTER_H
