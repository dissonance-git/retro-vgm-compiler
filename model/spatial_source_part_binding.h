#pragma once

#include "musical_execution_graph.h"
#include "spatial_source.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace vgmtooling::model {

inline const attribute* find_spatial_part_binding_attribute(
    const node& part,
    const char* key) noexcept {
    for (const auto& item : part.attributes) {
        if (item.key == key)
            return &item;
    }
    return nullptr;
}

// Spatial/source identity and persistent musical-part identity are different
// domains. This adapter binds them only after a persistent part has already been
// materialized in the execution graph. It inherits that part's epistemic bound
// instead of treating a physical channel/source id as musical identity.
inline spatial_source_evidence bind_spatial_source_to_persistent_part(
    spatial_source_evidence source,
    const node& part) {
    if (part.id == 0 || part.kind != node_kind::part)
        throw std::invalid_argument("spatial source part binding requires a materialized part node");

    const attribute* scope_item = find_spatial_part_binding_attribute(part, "identity_scope");
    if (scope_item == nullptr)
        throw std::invalid_argument("spatial source part binding requires persistent-part identity scope");
    const auto* scope = std::get_if<std::string>(&scope_item->value);
    if (scope == nullptr || *scope != "persistent_musical_part")
        throw std::invalid_argument("spatial source cannot bind to a non-persistent musical part");
    if (!std::isfinite(scope_item->confidence) ||
        scope_item->confidence < 0.0 || scope_item->confidence > 1.0) {
        throw std::invalid_argument("persistent-part binding confidence must be finite in [0, 1]");
    }

    source.persistent_part_present = true;
    source.persistent_part_id = part.id;
    source.persistent_part_confidence = static_cast<float>(scope_item->confidence);
    return source;
}

} // namespace vgmtooling::model
