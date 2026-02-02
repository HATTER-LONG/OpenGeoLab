/**
 * @file signal.cpp
 * @brief Implementation of signal-slot connection ID generator
 */

#include "util/signal.hpp"

#include <atomic>

namespace OpenGeoLab::Util {

namespace {
/// Atomic counter for generating unique connection IDs
std::atomic<ConnectionId> g_next_connection_id{1};
} // namespace

ConnectionId generateConnectionId() { return g_next_connection_id.fetch_add(1); }

} // namespace OpenGeoLab::Util
