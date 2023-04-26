#include <person.pb.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>

using namespace std;

person::Person CreatePerson() {
    person::Person p;

    // Зададим имя
    p.set_name("Человек"s);
    *p.mutable_name() += " Рассеяный"sv;

    // Добавим пару номеров телефона
    p.add_phone_number("33-22-22-33-22");
    p.add_phone_number("+7-123-456-789");

    // Создадим объект адреса
    person::Address addr;
    addr.set_street("Бассейная"s);
    addr.set_building(1);

    // Установим адрес
    *p.mutable_address() = move(addr);

    return p;
}

void Serialize(const filesystem::path& path, const person::Person& object) {
    ofstream out_file(path, ios::binary);
    object.SerializeToOstream(&out_file);
}

optional<person::Person> DeserializePerson(const filesystem::path& path) {
    ifstream in_file(path, ios::binary);
    person::Person object;
    if (!object.ParseFromIstream(&in_file)) {
        return nullopt;
    }

    // тут нужен move, поскольку возвращается другой тип
    return {std::move(object)};
}

int main() {
    const filesystem::path path = "test_file.bin"sv;

    Serialize(path, CreatePerson());

    optional<person::Person> deserialized = DeserializePerson(path);

    if (!deserialized) {
        cout << "Не получилось прочитать сообщение"sv << endl;
    } else {
        cout << deserialized->name();
        if (deserialized->has_address()) {
            cout << " с улицы "sv << deserialized->address().street();
        }
        cout << endl;
    }
}