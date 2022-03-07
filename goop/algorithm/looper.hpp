#pragma once

#include <thread>
#include <future>
#include <mutex>
#include <functional>
#include <queue>
#include <vector>

namespace goop
{
  template<typename R>
  bool is_ready(std::future<R> const& f)
  {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  }

  class looper
  {
  public:
    looper(int concurrency = std::thread::hardware_concurrency());

    template<typename Fun>
    std::future<std::invoke_result_t<Fun>> async(Fun&& fun)
    {
      using result_type = std::invoke_result_t<Fun>;
      auto promise = std::make_shared<std::promise<result_type>>();

      {
        std::unique_lock lock(_mtx);
        _runners.push([promise, f = std::forward<Fun>(fun)]{
          if constexpr (std::is_same_v<result_type, void>)
          {
            f();
            promise->set_value();
          }
          else
          {
            promise->set_value(f());
          }
          });
      }
      _cnd.notify_one();
      return promise->get_future();
    }

    template<typename Fun>
    void launch(Fun&& fun)
    {
      {
        std::unique_lock lock(_mtx);
        _runners.push([f = std::forward<Fun>(fun)]{ f(); });
      }
      _cnd.notify_one();
    }

  private:
    void loop(std::stop_token stop_token);

    static void loop_fun(std::stop_token stop_token, looper* self);

    std::vector<std::jthread> _threads;
    std::condition_variable _cnd;
    std::mutex _mtx;
    std::queue<std::function<void()>> _runners;
  };
}