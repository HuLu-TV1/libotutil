#pragma once
#include "common.h"
#include <queue>
#include <mutex>
#include <functional>
#include <chrono>

/**
 * @class Timer
 * @brief A singleton class that manages timed events using a priority queue.
 *
 * The Timer class is designed to handle timed events by scheduling them in a priority queue.
 * Each event is represented by a TimerNode, which contains the arrival time, interval,
 * whether the event is repeated, and the task to be executed. The class provides methods
 * to add new timed events, reset the arrival timer, start and stop the timer, and run
 * the event loop to process the events.
 *
 * The Timer class is a singleton, meaning that only one instance of it can exist at any time.
 * This instance can be accessed using the getInstance() method.
 *
 * @note The class uses a priority queue to manage the events, ensuring that the earliest
 *       event is always processed first. The TimerNode class is used to represent each event,
 *       and it implements the less-than operator to facilitate the priority queue's ordering.
 *
 * @note The Timer class is not copyable or movable to enforce the singleton pattern.
 */
class Timer {
    /**
     * @struct TimerNode
     * @brief Represents a timed event in the Timer class.
     *
     * Each TimerNode contains the following information:
     * - arrive_time_: The time at which the event is scheduled to occur.
     * - interval_: The interval at which the event should repeat, if applicable.
     * - is_repeated_: A boolean indicating whether the event is repeated.
     * - task_: The function to be executed when the event occurs.
     *
     * The TimerNode class also implements the less-than operator to allow the priority
     * queue to order the events based on their arrival time.
     */
    struct TimerNode {
        int64_t arrive_time_;
        int64_t interval_;
        bool is_repeated_;
        std::function<void()> task_;

        TimerNode() = default;
        TimerNode(int64_t interval, bool repeated, std::function<void()> task)
            : interval_(interval), is_repeated_(repeated), task_(task) {
            auto now = std::chrono::steady_clock::now();
            arrive_time_ = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count() + interval_;
        }

        bool operator<(const TimerNode& other) const {
            return arrive_time_ > other.arrive_time_;
        }
    };

public:
    DISALLOW_COPY_AND_MOVE(Timer);
    static Timer& getInstance() {
        static Timer instance;
        return instance;
    }

    ~Timer();

    int addTimeEvent(int64_t time, std::function<void()> task,
                     bool is_repeated=false, bool need_reset=true);
    void resetArriveTimer();
    void startTimer();
    void stopTimer();
    void eventLoop();

protected:
    explicit Timer();

private:
    int timer_fd_;
    int fd_;
    int wake_fd_;
    bool is_looping_;
    std::mutex mutex_;
    std::priority_queue<TimerNode> pending_events_;
};