#include <iostream>
#include <map>
#include <string>
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