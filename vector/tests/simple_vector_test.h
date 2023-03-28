#pragma once
#include <cassert>
#include <cstddef>
#include <iostream>
#include <numeric>

#include "../simple_vector/simple_vector.h"

namespace tests::vector {
    class SimpleVectorTests {
    private:
        class X {
        public:
            X() : X(5) {}
            X(size_t num) : x_(num) {}
            X(const X& other) = delete;
            X& operator=(const X& other) = delete;
            X(X&& other) {
                x_ = std::exchange(other.x_, 0);
            }
            X& operator=(X&& other) {
                x_ = std::exchange(other.x_, 0);
                return *this;
            }
            size_t GetX() const {
                return x_;
            }

        private:
            size_t x_;
        };

    public:
        SimpleVector<int> GenerateVector(size_t size) const;

        void TestTemporaryObjConstructor(size_t size = 1000000);

        void TestTemporaryObjOperator(size_t size = 1000000);

        void TestNamedMoveConstructor(size_t size = 1000000);

        void TestNamedMoveOperator(size_t size = 1000000);

        void TestNoncopiableMoveConstructor(size_t size = 5);

        void TestNoncopiablePushBack(size_t size = 5);

        void TestNoncopiableInsert(size_t size = 5);

        void TestNoncopiableErase(size_t size = 3);

        int Run();
    };
}