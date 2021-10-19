#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {
    void perform(Executor *executor){
        while(true){
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(executor->mutex);

                --executor->working_threads; // free for work;

                executor->empty_condition.wait_for(lock, std::chrono::milliseconds(executor->idle_time),
                                                   [executor]{return executor->tasks.empty();});
                if(executor->tasks.empty()) {
                    if(executor->state != Executor::State::kRun ||
                            (executor->cur_threads > executor->low_watermark)) {
                        --executor->cur_threads;
                        if(executor->cur_threads == 0){
                            executor->stopped_condition.notify_all();
                        }
                        return;
                    } else {
                        continue;
                    }
                }
                ++executor->working_threads; // starting work
                job = executor->tasks.front();
                executor->tasks.pop_front();
            }
            job();
        }
    }

}
} // namespace Afina
