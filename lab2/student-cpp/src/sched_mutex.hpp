#pragma once
// Этап 2: лок, о котором знает планировщик. Поток, ждущий занятый лок, не
// попадает в enabled; обычный std::mutex здесь нельзя — поток уснёт с ходом в
// руках и всё зависнет. Лог: "lock <имя>", "unlock <имя>". Планировщик должен
// знать все локи, чтобы печатать held={a:A, b:B}.
#include <list>
#include <string>

#include "scheduler.hpp"

namespace sched {

class sched_mutex {
public:
  // Создаётся в setup() тестовой программы; зарегистрируйте лок в планировщике
  // s.
  sched_mutex(Scheduler &s, std::string name) : s(s), name_(std::move(name)) {
    it = s.locks.insert(s.locks.end(), this);
  }
  sched_mutex(const sched_mutex &) = delete;

  ~sched_mutex() { s.locks.erase(it); }

  // Пауза; если лок занят — ждать, не участвуя в выборе; затем стать
  // владельцем.
  void lock() {
    mark_dirty();
    await_impl([this] { return !owner_; });
    log("lock " + name_);
    owner_ = this_thread();
  }
  // Пауза; освободить лок, ждавшие потоки снова могут быть выбраны.
  void unlock() {
    mark_dirty();
    await_impl();
    owner_ = nullptr;
    log("unlock " + name_);
  }

  std::string_view name() const noexcept { return name_; }

  Thread *owner() const noexcept { return owner_; }

private:
  Scheduler &s;
  const std::string name_;
  Thread *owner_ = nullptr;
  std::list<sched_mutex *>::iterator it;
};

// RAII: lock сейчас, unlock на закрывающей скобке. Выдан готовым.
class sched_guard {
public:
  explicit sched_guard(sched_mutex &m) : m_(m) { m_.lock(); }
  ~sched_guard() { m_.unlock(); }
  sched_guard(const sched_guard &) = delete;

private:
  sched_mutex &m_;
};

} // namespace sched
