#include "people.h"

#include <stdexcept>

namespace trainig::sprint10::people {
    using namespace std;

    const string& Person::GetName() const {
        // Заглушка, реализуйте метод самостоятельно
        return name_;
    }

    int Person::GetAge() const {
        // Заглушка, реализуйте метод самостоятельно
        return age_;
    }

    Gender Person::GetGender() const {
        // Заглушка, реализуйте метод самостоятельно
        return gender_;
    }

    Programmer::Programmer(const string& name, int age, Gender gender) : Person(name, age, gender), languages_{} {
        // Напишите тело конструктора
    }

    void Programmer::AddProgrammingLanguage(ProgrammingLanguage language) {
        // Заглушка, реализуйте метод самостоятельно
        languages_.emplace(std::move(language));
    }

    bool Programmer::CanProgram(ProgrammingLanguage language) const {
        // Заглушка, реализуйте метод самостоятельно
        return languages_.count(std::move(language));
    }

    Worker::Worker(const string& name, int age, Gender gender) : Person(name, age, gender), specialities_{} {
        // Заглушка, реализуйте конструктор самостоятельно
    }

    void Worker::AddSpeciality(WorkerSpeciality speciality) {
        specialities_.emplace(std::move(speciality));
    }

    bool Worker::HasSpeciality(WorkerSpeciality speciality) const {
        // Заглушка, реализуйте метод самостоятельно
        return specialities_.count(std::move(speciality));
    }
}