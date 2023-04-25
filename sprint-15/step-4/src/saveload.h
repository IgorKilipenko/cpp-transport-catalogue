#include <cstddef>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Serialization

template <typename T>
void Serialize(T val, std::ostream& out);

void Serialize(const std::string& str, std::ostream& out);

template <typename T>
void Serialize(const std::vector<T>& val, std::ostream& out);

template <typename T1, typename T2>
void Serialize(const std::map<T1, T2>& val, std::ostream& out);

// Deserialization

template <typename T>
void Deserialize(std::istream& in, T& val);

void Deserialize(std::istream& in, std::string& str);

template <typename T>
void Deserialize(std::istream& in, std::vector<T>& val);

template <typename T1, typename T2>
void Deserialize(std::istream& in, std::map<T1, T2>& val);

// Поскольку функции шаблонные, их реализации будем писать прямо в h-файле.
// Не забудьте объявить нешаблонные функции как inline.

template <typename T>
void Serialize(T val, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&val), sizeof(val));
}

inline void Serialize(const std::string& str, std::ostream& out) {
    const size_t len = str.length();
    Serialize(len, out);
    for (const auto ch : str) {
        out.write(reinterpret_cast<const char*>(&ch), sizeof(ch));
    }
}

template <typename T>
void Serialize(const std::vector<T>& val, std::ostream& out) {
    Serialize(val.size(), out);
    for (const auto& item : val) {
        Serialize(item, out);
    }
}

template <typename T1, typename T2>
void Serialize(const std::map<T1, T2>& val, std::ostream& out) {
    Serialize(val.size(), out);
    for (const auto& [key, val] : val) {
        Serialize(key, out);
        Serialize(val, out);
    }
}

template <typename T>
void Deserialize(std::istream& in, T& val) {
    in.read(reinterpret_cast<char*>(&val), sizeof(val));
}

inline void Deserialize(std::istream& in, std::string& str) {
    size_t len;
    Deserialize(in, len);
    str.resize(len);
    for (char& ch : str) {
        Deserialize(in, ch);
    }
}

template <typename T>
void Deserialize(std::istream& in, std::vector<T>& val) {
    size_t len;
    Deserialize(in, len);
    val.resize(len);
    for (auto& item : val) {
        Deserialize(in, item);
    }
}

template <typename T1, typename T2>
void Deserialize(std::istream& in, std::map<T1, T2>& val) {
    size_t len;
    Deserialize(in, len);
    for (int i = 0; i < len; ++i) {
        T1 first;
        T2 second;
        Deserialize(in, first);
        Deserialize(in, second);
        val.emplace(first, second);
    }
}