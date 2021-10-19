#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace Afina {
namespace Concurrency {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

    Executor(std::string name, size_t low_watermark = 2, size_t high_watermark = 2, size_t max_queue_size = 20, size_t idle_time=100)
        : name(std::move(name)), low_watermark(low_watermark), high_watermark(high_watermark),
          max_queue_size(max_queue_size), idle_time(idle_time), working_threads(0){
        std::unique_lock<std::mutex> lock(mutex); // for ++cur_threads
        for(int i = 0; i < low_watermark; ++i){
            std::thread([this] { return perform(this); }).detach();
            ++cur_threads;
        }
        state = State::kRun;
    }
    ~Executor();

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false){
        std::unique_lock<std::mutex> lock(mutex);
        state = State::kStopping;
        while(cur_threads != 0){
            stopped_condition.wait(lock);
        }
        state = State::kStopped;
    }

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex);
        if (state != State::kRun) {
            return false;
        }

        if(working_threads == cur_threads && cur_threads < high_watermark){
            std::thread([this] { return perform(this); }).detach();
            ++cur_threads;
        }
        // Enqueue new task
        if(tasks.size() <= max_queue_size) {
            tasks.push_back(exec);
            empty_condition.notify_one();
            return true;
        } else {
            return false;
        }
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor);

    std::string name;

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition;

    /**
     * Conditional variable to all workers to stop
     */
    std::condition_variable stopped_condition;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;

private: // Parameters

    size_t low_watermark;

    size_t high_watermark;

    size_t working_threads = 0;

    size_t cur_threads = 0;

    size_t max_queue_size;

    size_t idle_time;
};

} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
