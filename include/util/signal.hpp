/**
 * @file signal.hpp
 * @brief Lightweight signal-slot implementation for observer pattern
 *
 * Provides a type-safe, thread-aware signal-slot mechanism for decoupled
 * event-driven communication. Signals can connect to multiple slots and
 * emit events to all connected listeners.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace OpenGeoLab::Util {

/**
 * @brief Unique identifier for a signal-slot connection
 *
 * Used to disconnect a specific slot from a signal.
 */
using ConnectionId = uint64_t;

/// Invalid/null connection ID constant
constexpr ConnectionId INVALID_CONNECTION_ID = 0;

/**
 * @brief Generate a new unique connection ID
 * @return A new unique ConnectionId (thread-safe)
 */
[[nodiscard]] ConnectionId generateConnectionId();

/**
 * @brief RAII connection guard that auto-disconnects on destruction
 *
 * ScopedConnection ensures that a signal-slot connection is properly
 * cleaned up when the guard goes out of scope. This prevents dangling
 * callbacks to destroyed objects.
 */
class ScopedConnection {
public:
    /// Default constructor creates an empty guard
    ScopedConnection() = default;

    /**
     * @brief Construct with a disconnect function
     * @param disconnect_fn Function to call on destruction
     */
    explicit ScopedConnection(std::function<void()> disconnect_fn)
        : m_disconnectFn(std::move(disconnect_fn)) {}

    /// Move constructor
    ScopedConnection(ScopedConnection&& other) noexcept
        : m_disconnectFn(std::move(other.m_disconnectFn)) {
        other.m_disconnectFn = nullptr;
    }

    /// Move assignment
    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if(this != &other) {
            disconnect();
            m_disconnectFn = std::move(other.m_disconnectFn);
            other.m_disconnectFn = nullptr;
        }
        return *this;
    }

    /// Non-copyable
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    /// Destructor - automatically disconnects
    ~ScopedConnection() { disconnect(); }

    /**
     * @brief Manually disconnect
     */
    void disconnect() {
        if(m_disconnectFn) {
            m_disconnectFn();
            m_disconnectFn = nullptr;
        }
    }

    /**
     * @brief Release ownership without disconnecting
     */
    void release() { m_disconnectFn = nullptr; }

    /**
     * @brief Check if connection is active
     * @return true if a disconnect function is held
     */
    [[nodiscard]] bool isConnected() const { return m_disconnectFn != nullptr; }

private:
    std::function<void()> m_disconnectFn;
};

/**
 * @brief Thread-safe signal class for event broadcasting
 *
 * Signal implements the observer pattern with support for:
 * - Multiple connected slots
 * - Thread-safe connection/disconnection/emission
 * - RAII-based automatic disconnection via ScopedConnection
 *
 * @tparam Args Parameter types for the signal emission
 *
 * Usage example:
 * @code
 * Signal<int, const std::string&> mySignal;
 *
 * // Connect a slot
 * auto conn = mySignal.connect([](int val, const std::string& msg) {
 *     std::cout << val << ": " << msg << std::endl;
 * });
 *
 * // Emit the signal
 * mySignal.emit(42, "hello");
 *
 * // Connection auto-disconnects when 'conn' goes out of scope
 * @endcode
 */
template <typename... Args> class Signal {
public:
    /// Slot function type
    using SlotFn = std::function<void(Args...)>;

    Signal() = default;
    ~Signal() = default;

    /// Non-copyable
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// Movable
    Signal(Signal&&) noexcept = default;
    Signal& operator=(Signal&&) noexcept = default;

    /**
     * @brief Connect a slot to this signal
     * @param slot Callable to invoke on emit
     * @return ScopedConnection for automatic disconnection
     */
    [[nodiscard]] ScopedConnection connect(SlotFn slot) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const ConnectionId id = generateConnectionId();
        m_slots.emplace_back(id, std::move(slot));

        // Capture 'this' and 'id' for the disconnect lambda
        return ScopedConnection([this, id]() { this->disconnect(id); });
    }

    /**
     * @brief Connect a slot and return only the connection ID
     * @param slot Callable to invoke on emit
     * @return Connection ID for manual disconnection
     * @note Caller is responsible for calling disconnect()
     */
    [[nodiscard]] ConnectionId connectManual(SlotFn slot) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const ConnectionId id = generateConnectionId();
        m_slots.emplace_back(id, std::move(slot));
        return id;
    }

    /**
     * @brief Disconnect a slot by connection ID
     * @param id Connection ID returned by connect()
     * @return true if a slot was disconnected
     */
    bool disconnect(ConnectionId id) {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = std::find_if(m_slots.begin(), m_slots.end(),
                                     [id](const SlotEntry& entry) { return entry.m_id == id; });
        if(it != m_slots.end()) {
            m_slots.erase(it);
            return true;
        }
        return false;
    }

    /**
     * @brief Disconnect all connected slots
     */
    void disconnectAll() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_slots.clear();
    }

    /**
     * @brief Emit the signal to all connected slots
     * @param args Arguments to pass to each slot
     * @note Slots are invoked synchronously in connection order
     */
    void emitSignal(Args... args) {
        // Copy slots to allow disconnection during emission
        std::vector<SlotEntry> slots_copy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            slots_copy = m_slots;
        }

        for(const auto& entry : slots_copy) {
            if(entry.m_slot) {
                entry.m_slot(args...);
            }
        }
    }

    /**
     * @brief Operator() alias for emitSignal
     * @param args Arguments to pass to each slot
     */
    void operator()(Args... args) { emitSignal(std::forward<Args>(args)...); }

    /**
     * @brief Get the number of connected slots
     * @return Slot count
     */
    [[nodiscard]] size_t slotCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_slots.size();
    }

    /**
     * @brief Check if any slots are connected
     * @return true if at least one slot is connected
     */
    [[nodiscard]] bool hasSlots() const { return slotCount() > 0; }

private:
    struct SlotEntry {
        ConnectionId m_id{INVALID_CONNECTION_ID};
        SlotFn m_slot;

        SlotEntry(ConnectionId id, SlotFn slot) : m_id(id), m_slot(std::move(slot)) {}
    };

    mutable std::mutex m_mutex;
    std::vector<SlotEntry> m_slots;
};

} // namespace OpenGeoLab::Util
