#pragma once
#include <string>
#include <iostream>
#include <stdexcept>

#include "../vector.h"

namespace tests {
    // "Магическое" число, используемое для отслеживания живости объекта
    inline const uint32_t DEFAULT_COOKIE = 0xdeadbeef;

    struct TestObj {
        TestObj() = default;
        TestObj(const TestObj& other) = default;
        TestObj& operator=(const TestObj& other) = default;
        TestObj(TestObj&& other) = default;
        TestObj& operator=(TestObj&& other) = default;
        ~TestObj() {
            cookie = 0;
        }
        [[nodiscard]] bool IsAlive() const noexcept {
            return cookie == DEFAULT_COOKIE;
        }
        uint32_t cookie = DEFAULT_COOKIE;
    };

    struct Obj {
        Obj() {
            if (default_construction_throw_countdown > 0) {
                if (--default_construction_throw_countdown == 0) {
                    throw std::runtime_error("Oops");
                }
            }
            ++num_default_constructed;
        }

        explicit Obj(int id)
            : id(id)  //
        {
            ++num_constructed_with_id;
        }

        Obj(int id, std::string name)
            : id(id),
              name(std::move(name))  //
        {
            ++num_constructed_with_id_and_name;
        }

        Obj(const Obj& other)
            : id(other.id)  //
        {
            if (other.throw_on_copy) {
                throw std::runtime_error("Oops");
            }
            ++num_copied;
        }

        Obj(Obj&& other) noexcept
            : id(other.id)  //
        {
            ++num_moved;
        }

        Obj& operator=(const Obj& other) = default;
        Obj& operator=(Obj&& other) = default;

        ~Obj() {
            ++num_destroyed;
            id = 0;
        }

        static int GetAliveObjectCount() {
            return num_default_constructed + num_copied + num_moved + num_constructed_with_id + num_constructed_with_id_and_name - num_destroyed;
        }

        static void ResetCounters() {
            default_construction_throw_countdown = 0;
            num_default_constructed = 0;
            num_copied = 0;
            num_moved = 0;
            num_destroyed = 0;
            num_constructed_with_id = 0;
            num_constructed_with_id_and_name = 0;
        }

        bool throw_on_copy = false;
        int id = 0;
        std::string name;

        static inline int default_construction_throw_countdown = 0;
        static inline int num_default_constructed = 0;
        static inline int num_constructed_with_id = 0;
        static inline int num_constructed_with_id_and_name = 0;
        static inline int num_copied = 0;
        static inline int num_moved = 0;
        static inline int num_destroyed = 0;
    };

    void Test1();
    void Test2();
    void Test3();
    void Test4();
    void Test5();

    void run();
}
