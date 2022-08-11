#pragma once

#include <cstddef>
#include <istream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

namespace json::detail {
    template <bool Condition>
    using EnableIf = typename std::enable_if_t<Condition, bool>;

    template <typename FromType, typename ToType>
    using EnableIfConvertible = std::enable_if_t<std::is_convertible_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename FromType, typename ToType>
    using IsConvertible = std::is_convertible<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using IsSame = std::is_same<std::decay_t<FromType>, ToType>;

    template <typename FromType, typename ToType>
    using EnableIfSame = std::enable_if_t<std::is_same_v<std::decay_t<FromType>, ToType>, bool>;

    template <typename BaseType, typename DerivedType>
    using EnableIfBaseOf = std::enable_if_t<std::is_base_of_v<BaseType, std::decay_t<DerivedType>>, bool>;
}

namespace json {
    class Node;
    using Array = std::vector<Node>;
    using Dict = std::map<std::string, Node>;
    // template <typename ...T>
    // using NodeValueVarianrs = ...T;
    using NodeValueType = std::variant<std::nullptr_t, std::monostate, std::string, int, double, bool, json::Array, json::Dict>;

    class Node : private NodeValueType {
    public:
        using ValueType = NodeValueType;

        bool IsNull() const {
            return IsType<std::monostate>() || IsType<std::nullptr_t>();
        }

        bool IsBool() const {
            return IsType<bool>();
        }

        bool IsInt() const {
            return IsType<int>();
        }

        bool IsDouble() const {
            return IsType<double>() || IsType<int>();
        }

        bool IsPureDouble() const {
            return IsType<double>();
        }

        bool IsString() const {
            return IsType<std::string>();
        }

        bool IsArray() const {
            return IsType<json::Array>();
        }

        bool IsMap() const {
            return IsType<json::Dict>();
        }

        template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        bool IsType() const {
            return std::holds_alternative<T>(*this);
        }

        // template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto& GetValue() const {
            using namespace std::string_literals;
            if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
                try {
                    return std::get<T>(*this);
                } catch (std::bad_variant_access const& ex) {
                    throw std::logic_error(ex.what() + ": Node don't contained this type alternative."s);
                }
            } else {
                return static_cast<const Node::ValueType&>(*this);
            }
        }

        /*const Node::ValueType& GetValue() const {
            return GetValue<void>();
        }*/

        // template <typename T, detail::EnableIfConvertible<T, NodeValueType> = true>
        template <typename T = void, detail::EnableIf<detail::IsConvertible<T, NodeValueType>::value || detail::IsSame<T, void>::value> = true>
        const auto* GetValuePtr() const noexcept {
            if constexpr (detail::IsConvertible<T, NodeValueType>::value == true) {
                return std::get_if<T>(this);
            } else {
                return static_cast<const Node::ValueType*>(this);
            }
        }

        const Node::ValueType* GetValuePtr() const {
            return GetValuePtr<void>();
        }

        bool AsBool() const {
            return GetValue<bool>();
        }

        int AsInt() const {
            return GetValue<int>();
        }

        double AsDouble() const {
            const auto* ptr = GetValuePtr<double>();
            ptr = ptr == nullptr ? reinterpret_cast<const double*>(GetValuePtr<int>()) : ptr;
            if (ptr == nullptr) {
                throw std::logic_error("Node don't contained [double/integer] value alternative.");
            }
            return *ptr;
        }

        const std::string& AsString() const {
            return GetValue<std::string>();
        }

        const json::Array& AsArray() const {
            return GetValue<json::Array>();
        }

        const json::Dict& AsMap() const {
            return GetValue<json::Dict>();
        }

        //! const ValueType& GetValue() const;

        bool operator==(const Node& rhs) const {
            return this == &rhs || *GetValuePtr() == *rhs.GetValuePtr();
        }

        //! bool operator!=(const Node& rhs) const;
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);

}