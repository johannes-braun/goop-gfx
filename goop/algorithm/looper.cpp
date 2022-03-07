#include "looper.hpp"

namespace goop {

  looper::looper(int concurrency) {
    _threads.resize(concurrency);
    for (auto& t : _threads)
      t = std::jthread(&looper::loop_fun, this);
  }
  void looper::loop(std::stop_token stop_token)
  {
    while (!stop_token.stop_requested())
    {
      std::unique_lock lock(_mtx);
      do {
        _cnd.wait(lock, [&] { return stop_token.stop_requested() || !_runners.empty(); });
      } while (!stop_token.stop_requested() && _runners.empty());

      if (stop_token.stop_requested())
        return;

      auto const runner = std::move(_runners.front());
      _runners.pop();
      lock.unlock();
      runner();
    }
  }

  void looper::loop_fun(std::stop_token stop_token, looper* self) {
    self->loop(stop_token);
  }

}