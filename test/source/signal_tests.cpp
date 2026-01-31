/**
 * @file signal_tests.cpp
 * @brief Unit tests for Signal/Slot implementation
 */

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <util/signal.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

using namespace OpenGeoLab::Util;

TEST_CASE("Signal - Basic connection and emission", "[signal]") {
    Signal<int> sig;
    int received_value = 0;

    auto conn = sig.connect([&](int val) { received_value = val; });

    sig.emit(42);
    REQUIRE(received_value == 42);

    sig.emit(100);
    REQUIRE(received_value == 100);
}

TEST_CASE("Signal - Multiple slots", "[signal]") {
    Signal<int> sig;
    std::vector<int> results;

    auto conn1 = sig.connect([&](int val) { results.push_back(val * 1); });
    auto conn2 = sig.connect([&](int val) { results.push_back(val * 2); });
    auto conn3 = sig.connect([&](int val) { results.push_back(val * 3); });

    sig.emit(10);

    REQUIRE(results.size() == 3);
    REQUIRE(results[0] == 10);
    REQUIRE(results[1] == 20);
    REQUIRE(results[2] == 30);
}

TEST_CASE("Signal - ScopedConnection auto-disconnect", "[signal]") {
    Signal<int> sig;
    int call_count = 0;

    {
        auto conn = sig.connect([&](int) { call_count++; });
        sig.emit(1);
        REQUIRE(call_count == 1);
    } // conn goes out of scope, should disconnect

    sig.emit(2);
    REQUIRE(call_count == 1); // Should not have been called again
}

TEST_CASE("Signal - Manual disconnect", "[signal]") {
    Signal<int> sig;
    int call_count = 0;

    auto conn = sig.connect([&](int) { call_count++; });

    sig.emit(1);
    REQUIRE(call_count == 1);

    conn.disconnect();

    sig.emit(2);
    REQUIRE(call_count == 1); // Should not have been called after disconnect
}

TEST_CASE("Signal - Manual ConnectionId disconnect", "[signal]") {
    Signal<int> sig;
    int call_count = 0;

    ConnectionId id = sig.connectManual([&](int) { call_count++; });

    sig.emit(1);
    REQUIRE(call_count == 1);

    bool disconnected = sig.disconnect(id);
    REQUIRE(disconnected);

    sig.emit(2);
    REQUIRE(call_count == 1);

    // Disconnecting again should return false
    disconnected = sig.disconnect(id);
    REQUIRE_FALSE(disconnected);
}

TEST_CASE("Signal - DisconnectAll", "[signal]") {
    Signal<int> sig;
    int count1 = 0, count2 = 0;

    auto conn1 = sig.connect([&](int) { count1++; });
    auto conn2 = sig.connect([&](int) { count2++; });

    sig.emit(1);
    REQUIRE(count1 == 1);
    REQUIRE(count2 == 1);

    sig.disconnectAll();

    sig.emit(2);
    REQUIRE(count1 == 1);
    REQUIRE(count2 == 1);
}

TEST_CASE("Signal - Multiple arguments", "[signal]") {
    Signal<int, const std::string&, double> sig;
    int int_val = 0;
    std::string str_val;
    double dbl_val = 0.0;

    auto conn = sig.connect([&](int i, const std::string& s, double d) {
        int_val = i;
        str_val = s;
        dbl_val = d;
    });

    sig.emit(42, "hello", 3.14);

    REQUIRE(int_val == 42);
    REQUIRE(str_val == "hello");
    REQUIRE(dbl_val == Catch::Approx(3.14));
}

TEST_CASE("Signal - No arguments", "[signal]") {
    Signal<> sig;
    int call_count = 0;

    auto conn = sig.connect([&]() { call_count++; });

    sig.emit();
    sig.emit();
    sig.emit();

    REQUIRE(call_count == 3);
}

TEST_CASE("Signal - Operator() alias", "[signal]") {
    Signal<int> sig;
    int received = 0;

    auto conn = sig.connect([&](int val) { received = val; });

    sig(99);
    REQUIRE(received == 99);
}

TEST_CASE("Signal - slotCount and hasSlots", "[signal]") {
    Signal<int> sig;

    REQUIRE(sig.slotCount() == 0);
    REQUIRE_FALSE(sig.hasSlots());

    auto conn1 = sig.connect([](int) {});
    REQUIRE(sig.slotCount() == 1);
    REQUIRE(sig.hasSlots());

    auto conn2 = sig.connect([](int) {});
    REQUIRE(sig.slotCount() == 2);

    conn1.disconnect();
    REQUIRE(sig.slotCount() == 1);

    conn2.disconnect();
    REQUIRE(sig.slotCount() == 0);
    REQUIRE_FALSE(sig.hasSlots());
}

TEST_CASE("Signal - ScopedConnection move semantics", "[signal]") {
    Signal<int> sig;
    int call_count = 0;

    ScopedConnection conn1;
    {
        ScopedConnection conn2 = sig.connect([&](int) { call_count++; });

        sig.emit(1);
        REQUIRE(call_count == 1);

        conn1 = std::move(conn2);
        REQUIRE_FALSE(conn2.isConnected()); // NOLINT (intentional use-after-move test)
        REQUIRE(conn1.isConnected());
    }

    sig.emit(2);
    REQUIRE(call_count == 2); // Connection still active via conn1
}

TEST_CASE("Signal - Connection release", "[signal]") {
    Signal<int> sig;
    int call_count = 0;

    auto conn = sig.connect([&](int) { call_count++; });
    conn.release(); // Release ownership

    sig.emit(1);
    REQUIRE(call_count == 1); // Connection still active because we released (not disconnected)

    // Now we have a leaked connection - this is expected behavior for release()
    // In real usage, you'd store the ConnectionId and disconnect manually
}

TEST_CASE("Signal - Thread safety", "[signal]") {
    Signal<int> sig;
    std::atomic<int> total{0};

    // Connect 10 slots
    std::vector<ScopedConnection> connections;
    for(int i = 0; i < 10; ++i) {
        connections.push_back(sig.connect([&](int val) { total.fetch_add(val); }));
    }

    // Emit from multiple threads
    std::vector<std::thread> threads;
    for(int t = 0; t < 4; ++t) {
        threads.emplace_back([&]() {
            for(int i = 0; i < 100; ++i) {
                sig.emit(1);
            }
        });
    }

    for(auto& t : threads) {
        t.join();
    }

    // 4 threads * 100 emits * 10 slots * 1 value = 4000
    REQUIRE(total.load() == 4000);
}

TEST_CASE("Signal - Disconnect during emission", "[signal]") {
    Signal<int> sig;
    int call_count = 0;
    ScopedConnection conn;

    conn = sig.connect([&](int) {
        call_count++;
        conn.disconnect(); // Disconnect self during callback
    });

    // This should not crash or cause issues
    sig.emit(1);
    REQUIRE(call_count == 1);

    sig.emit(2);
    REQUIRE(call_count == 1); // Not called again
}

TEST_CASE("ConnectionId generation", "[signal]") {
    ConnectionId id1 = generateConnectionId();
    ConnectionId id2 = generateConnectionId();
    ConnectionId id3 = generateConnectionId();

    REQUIRE(id1 != INVALID_CONNECTION_ID);
    REQUIRE(id2 != INVALID_CONNECTION_ID);
    REQUIRE(id3 != INVALID_CONNECTION_ID);
    REQUIRE(id1 != id2);
    REQUIRE(id2 != id3);
    REQUIRE(id1 < id2);
    REQUIRE(id2 < id3);
}
