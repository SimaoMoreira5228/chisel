#ifndef MINITEST_H
#define MINITEST_H

#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace Test {
    int failures = 0;
    std::vector<std::function<void()>> tests;

    struct TestRegistrar {
        TestRegistrar(std::function<void()> func) { tests.push_back(func); }
    };

    template <typename T, typename U>
    void AssertEqual(const T &expected, const U &actual, const std::string &expr, const char *file, int line) {
        if(expected != actual) {
            std::cerr << file << ":" << line << ": Assertion failed: " << expr << " (expected: " << expected
                      << ", actual: " << actual << ")\n";
            failures++;
        }
    }

    void AssertTrue(bool condition, const std::string &expr, const char *file, int line) {
        if(!condition) {
            std::cerr << file << ":" << line << ": Assertion failed: " << expr << "\n";
            failures++;
        }
    }

    int RunAllTests() {
        int total = 0;
        failures = 0;
        std::cout << "Running " << tests.size() << " tests...\n";
        for(const auto &test : tests) {
            try {
                test();
                total++;
                std::cout << " ✅" << std::endl;
            } catch(const std::exception &e) {
                std::cerr << "\nTest crashed: " << e.what() << "\n";
                failures++;
                std::cout << " ❌" << std::endl;
            }
        }
        std::cout << "\nTests complete: " << (total - failures) << "/" << total << " passed.\n";
        return failures > 0 ? 1 : 0;
    }
} // namespace Test

#ifdef ENABLE_TESTS
#define TEST(name)                                                                                                          \
    void test_##name();                                                                                                     \
    static Test::TestRegistrar registrar_##name(&test_##name);                                                              \
    void test_##name()

#define ASSERT_TRUE(condition) Test::AssertTrue(condition, #condition, __FILE__, __LINE__)
#define ASSERT_EQ(expected, actual) Test::AssertEqual(expected, actual, #expected " == " #actual, __FILE__, __LINE__)
#else
#define TEST(name) static void test_##name()
#define ASSERT_TRUE(condition)
#define ASSERT_EQ(expected, actual)
#endif

#endif