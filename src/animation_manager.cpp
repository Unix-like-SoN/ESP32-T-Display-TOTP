#include "animation_manager.h"
#include <cmath>

AnimationManager::AnimationManager() {
    // Инициализируем наш пул анимаций заранее
    animations.resize(MAX_ANIMATIONS);
}

// Формула сглаживания (медленный старт, ускорение, медленное завершение)
float AnimationManager::easeInOutQuad(float t) {
    return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;
}

void AnimationManager::startAnimation(unsigned long duration, float startValue, float endValue, std::function<void(float, bool)> onUpdate) {
    // Находим первый свободный слот для анимации
    for (auto& anim : animations) {
        if (!anim.active) {
            anim.active = true;
            anim.startTime = millis();
            anim.duration = duration;
            anim.startValue = startValue;
            anim.endValue = endValue;
            anim.onUpdate = onUpdate;
            return; // Запускаем и выходим
        }
    }
    // Если свободных слотов нет, ничего не делаем
}

void AnimationManager::update() {
    unsigned long currentTime = millis();

    for (auto& anim : animations) {
        if (!anim.active) {
            continue;
        }

        unsigned long elapsedTime = currentTime - anim.startTime;

        if (elapsedTime >= anim.duration) {
            // Анимация завершена
            anim.onUpdate(anim.endValue, true); // Вызываем коллбэк с конечным значением
            anim.active = false; // Освобождаем слот
        } else {
            // Анимация в процессе
            float progress = (float)elapsedTime / (float)anim.duration;
            float easedProgress = easeInOutQuad(progress);
            float currentValue = anim.startValue + (anim.endValue - anim.startValue) * easedProgress;
            anim.onUpdate(currentValue, false); // Вызываем коллбэк с промежуточным значением
        }
    }
}
