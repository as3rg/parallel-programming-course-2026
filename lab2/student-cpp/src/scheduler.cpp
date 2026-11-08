// Этап 1–2: реализация Scheduler, point() и await().
// Сейчас все методы бросают исключение «TODO»: заполните их по заданию этапов 1
// и 2. Деструктор должен дождаться (join) всех запущенных потоков, в том числе
// после deadlock.
#include "scheduler.hpp"
#include "sched_mutex.hpp"

#include <cassert>
#include <thread>

namespace sched {

struct Terminate {};

struct Thread {
  char name;
  bool alive;
  std::function<bool()> enabled;
  std::binary_semaphore sem{0};
  std::jthread thread;
  Scheduler *sched;
};

Scheduler::Scheduler() {}

Scheduler::~Scheduler() {}

static thread_local Thread *this_thread_ = nullptr;

void Scheduler::spawn(std::function<void()> body) {
  auto t = std::make_unique<Thread>();
  t->name = name_++;
  t->sched = this;

  t->thread = std::jthread([&t = *t, body] {
    this_thread_ = &t;
    this_thread_->sem.acquire();
    this_thread_->alive = true;
    try {
      body();
    } catch (Terminate) {
    }
    this_thread_->alive = false;
    this_thread_->sched->turn.release();
  });
  threads.emplace_back(std::move(t));
}

void Scheduler::run(Strategy &s) {
  int last = 0;
  for (size_t i = 0; i < threads.size(); i++) {
    turn.acquire();

    last = i;
    threads[i]->sem.release();
  }

  while (true) {
    turn.acquire();
    dirty = false;

    bool any_alive = false;
    std::vector<int> enabled;
    for (size_t i = 0; i < threads.size(); i++) {
      auto &t = threads[i];
      any_alive |= t->alive;
      if (t->alive && (!t->enabled || t->enabled())) {
        enabled.emplace_back(i);
      }
    }

    if (!any_alive)
      break;

    if (enabled.empty()) {
      std::stringstream ss;
      ss << "held={";
      for (auto *lock : locks) {
        if (lock != locks.front()) {
          ss << ", ";
        }
        ss << lock->name() << ":" << lock->owner()->name;
      }
      ss << "}";
      deadlock = ss.str();

      for (size_t i = 0; i < threads.size(); i++) {
        threads[i]->sem.release();
      }
      break;
    }

    last = s.pick(enabled, last);
    trace << threads[last]->name;

    threads[last]->sem.release();
  }
}

std::string Scheduler::trace_string() const { return trace.str(); }

void mark_dirty() {
  assert(this_thread_ && "this_thread is not set");

  this_thread_->sched->dirty = true;
}

void await_impl(std::function<bool()> cond) {
  assert(this_thread_ && "this_thread is not set");

  if (!this_thread_->sched->dirty && (!cond || cond()))
    return;

  this_thread_->enabled = cond;
  this_thread_->sched->turn.release();

  this_thread_->sem.acquire();
  this_thread_->enabled = nullptr;

  if (!this_thread_->sched->deadlock.empty()) {
    throw Terminate{};
  }
}

void await(std::function<bool()> cond) {
  bool wasFalse = false;
  bool wasTrue = false;
  const auto condWrapped = [&wasFalse, &wasTrue, &cond] {
    if (!wasTrue) {
      const bool res = cond();
      wasFalse |= !res;
      wasTrue |= res;
    }
    return wasTrue;
  };
  await_impl(condWrapped);
  if (wasFalse) {
    mark_dirty();
    log("await done");
  }
}

void point() {
  mark_dirty();
  log("window");
}

void log(std::string_view log) {
  assert(this_thread_ && "this_thread is not set");

  if (!this_thread_->sched->logging)
    return;

  std::stringstream ss;
  ss << this_thread_->name << "  " << log;
  this_thread_->sched->log.emplace_back(ss.str());
}

Thread *this_thread() { return this_thread_; }
} // namespace sched
