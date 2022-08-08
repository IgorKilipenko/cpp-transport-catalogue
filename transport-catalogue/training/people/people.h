#pragma once
#include <string>

namespace trainig::sprint10::people {

    class Person;

    // Наблюдатель за состоянием человека.
    class PersonObserver {
    public:
        // Этот метод вызывается, когда меняется состояние удовлетворённости человека
        virtual void OnSatisfactionChanged(Person& /*person*/, int /*old_value*/, int /*new_value*/) {
            // Реализация метода базового класса ничего не делает
        }

    protected:
        // Класс PersonObserver не предназначен для удаления напрямую
        ~PersonObserver() = default;
    };

    /*
        Человек.
        При изменении уровня удовлетворённости уведомляет
        связанного с ним наблюдателя
    */
    class Person {
    public:
        Person(const std::string& name, int age) : name_{name}, age_{age} {
            // Напишите реализацию самостоятельно
        }

        virtual ~Person() = default;

        int GetSatisfaction() const {
            return satisfaction_;
        }

        const std::string& GetName() const {
            return name_;
        }

        // «Привязывает» наблюдателя к человеку. Привязанный наблюдатель
        // уведомляется об изменении уровня удовлетворённости человека
        // Новый наблюдатель заменяет собой ранее привязанного
        // Если передать nullptr в качестве наблюдателя, это эквивалентно отсутствию наблюдателя
        void SetObserver(PersonObserver* observer) {
            // Напишите реализацию самостоятельно
            observer_ = observer;
        }

        int GetAge() const {
            return age_;
        }

        // Увеличивает на 1 количество походов на танцы
        // Увеличивает удовлетворённость на 1
        virtual void Dance() {
            // Напишите тело метода самостоятельно
            Dance(1);
        }

        int GetDanceCount() const {
            // Заглушка. Напишите реализацию самостоятельно
            return dance_count_;
        }

        // Прожить день. Реализация в базовом классе ничего не делает
        virtual void LiveADay() {
            // Подклассы могут переопределить этот метод
        }

    private:
        std::string name_;
        int age_;
        int satisfaction_ = 100;
        PersonObserver* observer_ = nullptr;
        int dance_count_ = 0;

    protected:
        void AddSatisfaction(int value) {
            if (satisfaction_ == satisfaction_ + value) {
                return;
            }
            int old_satisfaction = satisfaction_;
            satisfaction_ += value;
            if (observer_) {
                observer_->OnSatisfactionChanged(*this, old_satisfaction, satisfaction_);
            }
        }
        void Dance(int satisfaction_count, int dance_count = 1) {
            // Напишите тело метода самостоятельно
            AddSatisfaction(satisfaction_count);
            dance_count_ += dance_count;
        }
    };

    // Рабочий.
    // День рабочего проходит за работой
    class Worker : public Person {
    public:
        Worker(const std::string& name, int age) : Person(name, age), dance_satisfaction_step_{(age > 30 && age < 40) ? 2 : 1} {
            // Напишите недостающий код
        }

        // Рабочий старше 30 лет и младше 40 за танец получает 2 единицы удовлетворённости вместо 1
        // День рабочего проходит за работой

        void Dance() override {
            // Напишите тело метода самостоятельно
            Person::Dance(dance_satisfaction_step_);
        }

        void LiveADay() override {
            // Подклассы могут переопределить этот метод
            Work();
        }

        /// Увеличивает счётчик сделанной работы на 1, уменьшает удовлетворённость на 5
        void Work() {
            // Напишите тело метода самостоятельно
            const static int satisfaction_work_step = 5;
            ++work_count_;
            AddSatisfaction(-satisfaction_work_step);
        }

        // Возвращает значение счётчика сделанной работы
        int GetWorkDone() const {
            // Заглушка. Напишите реализацию самостоятельно
            return work_count_;
        }

    private:
        int work_count_ = 0;
        int dance_satisfaction_step_ = 0;
    };

    // Студент.
    // День студента проходит за учёбой
    class Student : public Person {
    public:
        Student(const std::string& name, int age) : Person(name, age) {
            // Напишите недостающий код
        }

        void LiveADay() override {
            // Подклассы могут переопределить этот метод
            Study();
        }

        // День студента проходит за учёбой

        // Учёба увеличивает уровень знаний на 1, уменьшает уровень удовлетворённости на 3
        void Study() {
            // Напишите реализацию самостоятельно
            const static int satisfaction_study_step = 3;

            ++study_count_;
            AddSatisfaction(-satisfaction_study_step);
        }

        // Возвращает уровень знаний
        int GetKnowledgeLevel() const {
            // Заглушка, напишите реализацию самостоятельно
            return study_count_;
        }

    private:
        int study_count_ = 0;
    };
}