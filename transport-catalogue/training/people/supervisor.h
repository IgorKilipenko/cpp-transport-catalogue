#pragma once

#include "people.h"

namespace trainig::sprint10::people {
    /*
        Надзиратель за уровнем удовлетворённости.
        Способен наблюдать за состоянием человека
        Если уровень удовлетворённости человека опустится ниже минимального уровня, Надзиратель
        побуждает человека танцевать до тех пор, пока уровень уровень удовлетворённости
        не станет больше или равен максимальному значению
    */
    class SatisfactionSupervisor : public PersonObserver {
    public:
        // Конструктор принимает значение нижнего и верхнего уровня удовлетворённости
        SatisfactionSupervisor(int min_satisfaction, int max_satisfaction)
            : PersonObserver(), min_satisfaction_{min_satisfaction}, max_satisfaction_{max_satisfaction} {
            // Напишите реализацию самостоятельно
        }

        // Напишите реализацию самостоятельно
        void OnSatisfactionChanged(Person& person, int old_value, int new_value) override {
            if (new_value > old_value || new_value >= min_satisfaction_) {
                return;
            }

            while (person.GetSatisfaction() < max_satisfaction_) {
                person.Dance();
            }
        }

    private:
        const int min_satisfaction_;
        const int max_satisfaction_;
    };
}