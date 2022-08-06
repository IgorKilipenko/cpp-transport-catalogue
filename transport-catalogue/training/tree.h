#pragma once
#include <cassert>
#include <iostream>
#include <memory>

template <typename T>
struct TreeNode;

template <typename T>
using TreeNodePtr = std::unique_ptr<TreeNode<T>>;

template <typename T>
struct TreeNode {
    // Используйте TreeNodePtr<T> вместо сырых указателей
    // Примите умные указатели по rvalue-ссылке
    TreeNode(T val, TreeNodePtr<T>&& left, TreeNodePtr<T>&& right) : value(std::move(val)), left(std::move(left)), right(std::move(right)) {}

    T value;
    TreeNodePtr<T> left;   // Замените TreeNode* на TreeNodePtr<T>
    TreeNodePtr<T> right;  // Замените TreeNode* на TreeNodePtr<T>
    TreeNode* parent = nullptr;
};

template <typename T>
bool CheckTreeProperty(const TreeNode<T>* node, const T* min, const T* max) {
    if (!node) {
        return true;
    }
    if (((max) && node->value > *max) || ((min) && node->value < *min)) {
        return false;
    }
    return CheckTreeProperty<T>(node->left.get(), min, &node->value) && CheckTreeProperty<T>(node->right.get(), &node->value, max);
}

template <typename T>
bool CheckTreeProperty(const TreeNode<T>* node) {
    return CheckTreeProperty<T>(node, nullptr, nullptr);
}

template <typename T>
TreeNode<T>* begin(TreeNode<T>* node) noexcept {
    if (!node) {
        return nullptr;
    }
    TreeNode<T>* result = node;
    while (result->left) {
        result = result->left.get();
    }
    return result;
}

template <typename T>
TreeNode<T>* next(TreeNode<T>* node) noexcept {
    if (!node) {
        return nullptr;
    }
    if (node->right) {
        return begin(node->right.get());
    }
    TreeNode<T>* result = node;
    while (result && result->parent) {
        if (result->parent->left.get() == result) {
            result = result->parent;
            break;
        }
        if (result->parent->right.get() == result) {
            result = result->parent->parent ? result->parent : nullptr;
        }
    }
    return result;
}

// функция создаёт новый узел с заданным значением и потомками
inline TreeNodePtr<int> N(int val, TreeNodePtr<int>&& left = {}, TreeNodePtr<int>&& right = {}) {
    auto res =  std::make_unique<TreeNode<int>>(std::move(val), std::move(left), std::move(right)); //new TreeNode<int>{val, nullptr, left, right};
    if (res->left) {
        res->left->parent = res.get();
    }
    if (res->right) {
        res->right->parent = res.get();
    }

    return res;
}
