#pragma once

#include <cstdint>

// Dictionary-only tap/hold classification. A tap is committed on release;
// holding moves rows, with no extra word step when the button is released.
class DictionaryButtonNavigation {
 public:
  enum class Event { None, Word, Row };
  void reset(bool alreadyPressed = false) {
    pressed_ = held_ = false;
    ignoreUntilRelease_ = alreadyPressed;
  }

  Event update(bool down, uint32_t now) {
    if (ignoreUntilRelease_) {
      if (!down) ignoreUntilRelease_ = false;
      return Event::None;
    }
    if (down) {
      if (!pressed_) {
        pressed_ = true;
        startedAt_ = repeatedAt_ = now;
      }
      if (now - startedAt_ >= HOLD_MS && (!held_ || now - repeatedAt_ >= REPEAT_MS)) {
        held_ = true;
        repeatedAt_ = now;
        return Event::Row;
      }
      return Event::None;
    }
    if (!pressed_) return Event::None;
    // A slow loop may first observe the threshold on release: still a hold.
    const Event event = held_ ? Event::None : now - startedAt_ >= HOLD_MS ? Event::Row : Event::Word;
    reset();
    return event;
  }

 private:
  static constexpr uint32_t HOLD_MS = 500;
  static constexpr uint32_t REPEAT_MS = 350;
  bool pressed_ = false;
  bool held_ = false;
  bool ignoreUntilRelease_ = false;
  uint32_t startedAt_ = 0;
  uint32_t repeatedAt_ = 0;
};
