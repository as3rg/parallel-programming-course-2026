#pragma once
// Этап 1: планировщик, который ведёт настоящие потоки строго по одному
// (передача хода через семафоры). Этап 2: обнаружение deadlock в главном цикле
// и корректное завершение застрявших потоков. Внутреннее устройство (поля,
// семафоры, вспомогательные функции для shared и sched_mutex) придумываете
// сами.
#include "sstream"
#include <functional>
#include <list>
#include <memory>
#include <semaphore>
#include <string>
#include <vector>

#include "strategy.hpp"

namespace sched {

struct sched_mutex;

struct Thread;

class Scheduler {
public:
  Scheduler();
  ~Scheduler();
  // Зарегистрировать поток. Потоки получают буквы A, B, C… в порядке вызова
  // spawn.
  void spawn(std::function<void()> body);
  // Запустить потоки и вести их по одному: на каждой паузе спросить
  // strategy.pick(enabled, last).
  void run(Strategy &strategy);
  // Трасса прогона: буквы потоков, выбранных во всех решениях подряд, например
  // "AAAABB".
  std::string trace_string() const;

  // Если true, каждая операция с общими данными добавляет строку в log.
  // Выставляет runner.
  bool logging = false;
  // Лог прогона: строки "<поток>  <операция>", например "A  lock mu", "A
  // balance -> 100".
  std::vector<std::string> log;
  // Пустая строка, если deadlock не было; иначе "held={a:A, b:B}" (этап 2).
  std::string deadlock;

  std::list<sched_mutex *> locks;

private:
  char name_ = 'A';
  std::ostringstream trace;
  std::vector<std::unique_ptr<Thread>> threads;
  std::binary_semaphore turn{1};
  bool dirty = false;

  void recover_deadlock();

  friend void mark_dirty();
  friend void await_impl(std::function<bool()> cond);
};
void mark_dirty();
void await_impl(std::function<bool()> cond = nullptr);
Thread *this_thread();

// window() из тестовых программ: пауза перед следующей операцией с общими
// данными обязательна.
void point();
// Ждать, пока cond не станет true. До этого поток не попадает в enabled.
void await(std::function<bool()> cond);

void log(std::string_view log);
} // namespace sched
