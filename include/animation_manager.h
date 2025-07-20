#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <functional>

// Структура для хранения состояния одной анимации
struct Animation {
    bool active = false;
    unsigned long startTime;
    unsigned long duration;
    float startValue;
    float endValue;
    // Callback-функция, которая будет вызываться на каждом кадре
    // Она принимает текущее значение анимируемого свойства
    std::function<void(float currentValue, bool isFinished)> onUpdate;
};

class AnimationManager {
public:
    AnimationManager();

    // Запускает новую анимацию
    void startAnimation(unsigned long duration, float startValue, float endValue, std::function<void(float, bool)> onUpdate);

    // Должен вызываться в каждом цикле loop() для обновления всех анимаций
    void update();

private:
    // Простая функция сглаживания для более приятного движения
    float easeInOutQuad(float t);

    // Пул объектов анимации для предотвращения динамического выделения памяти
    std::vector<Animation> animations;
    static const int MAX_ANIMATIONS = 10; // Максимальное количество одновременных анимаций
};

#endif // ANIMATION_MANAGER_H
