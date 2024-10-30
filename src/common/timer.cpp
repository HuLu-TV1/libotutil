#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include "timer.h"
#include "log.h"

const int MAX_EVENTS = 10;

Timer::Timer(): fd_(-1), timer_fd_(-1), wake_fd_(-1) {
    fd_ = epoll_create1(0);
    if (fd_ == -1) {
        CLOG(ERROR, "Failed to create epoll fd");
        return;
    }

    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd_ == -1) {
        CLOG(ERROR, "Failed to create timer fd");
        close(fd_);
        return;
    }

    wake_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (wake_fd_ == -1) {
        CLOG(ERROR, "Failed to create wake fd");
        close(fd_);
        close(timer_fd_);
        return;
    }
    CLOG(INFO, "fd_: " << fd_ << " timer_fd_: " << timer_fd_ << " wake_fd_: " << wake_fd_);

    struct epoll_event event;
    event.events = EPOLLIN;

    event.data.fd = timer_fd_;
    if (epoll_ctl(fd_, EPOLL_CTL_ADD, timer_fd_, &event) == -1) {
        CLOG(ERROR, "Failed to add timer_fd_ to epoll");
        close(fd_);
        close(timer_fd_);
        close(wake_fd_);
        return;
    }

    event.data.fd = wake_fd_;
    if (epoll_ctl(fd_, EPOLL_CTL_ADD, wake_fd_, &event) == -1) {
        CLOG(ERROR, "Failed to add wake_fd_ to epoll");
        close(fd_);
        close(timer_fd_);
        close(wake_fd_);
        return;
    }

    startTimer();
}

Timer::~Timer() {
    // Ensure that the timer is stopped safely
    if (is_looping_) {
        stopTimer();
    }

    // Close the file descriptors if they were successfully opened
    if (fd_ != -1) {
        if (close(fd_) == -1) {
            CLOG(ERROR, "Failed to close epoll fd");
        }
    }

    if (timer_fd_ != -1) {
        if (close(timer_fd_) == -1) {
            CLOG(ERROR, "Failed to close timer fd");
        }
    }

    if (wake_fd_ != -1) {
        if (close(wake_fd_) == -1) {
            CLOG(ERROR, "Failed to close wake fd");
        }
    }
}

int Timer::addTimeEvent(int64_t time, std::function<void()> task,
                        bool is_repeated, bool need_reset) {
    TimerNode node{time, is_repeated, task};
    bool is_reset = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_events_.empty() || time < pending_events_.top().arrive_time_) {
            is_reset = true;
        }
        pending_events_.emplace(node);
    }

    if (is_reset && need_reset) {
        resetArriveTimer();
    }

    return 0;
}

void Timer::resetArriveTimer() {
    TimerNode earliest_node;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_events_.empty()) {
            CLOG(INFO, "No timerevent pending, size = 0");
            return;
        }
        earliest_node = pending_events_.top();
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::seconds(earliest_node.arrive_time_) - now.time_since_epoch();

    if (duration.count() <= 0) {
        CLOG(INFO, "Timer event already expired, executing now");
        earliest_node.task_();
        return;
    }

    itimerspec new_value;
    new_value.it_value.tv_sec = duration.count();
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd_, TFD_TIMER_ABSTIME, &new_value, nullptr) == -1) {
        CLOG(ERROR, "Failed to set timerfd time");
    }
}

void Timer::startTimer() {
    is_looping_ = true;
    std::thread thread(&Timer::eventLoop, this);
    thread.join();
}

void Timer::stopTimer() {
    is_looping_ = false;
    char data = 'w';
    ssize_t n = write(wake_fd_, &data, sizeof(char));
    if (n == -1) {
        CLOG(ERROR, "Failed to write to wake_fd_");
    }
}

void Timer::eventLoop() {
    struct epoll_event events[MAX_EVENTS];
    while (is_looping_) {
        int nfds = epoll_wait(fd_, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            CLOG(ERROR, "epoll_wait failed.");
            break;
        }
        for (int i=0; i<=nfds; i++) {
            if (events[i].data.fd == wake_fd_) continue;
            else if (events[i].data.fd == timer_fd_) {
                TimerNode node;
                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    node = pending_events_.top();
                    pending_events_.pop();
                }
                node.task_();
                if (node.is_repeated_) {
                    addTimeEvent(node.interval_, node.task_, node.is_repeated_, false);
                }
                resetArriveTimer();
            }
        }
    }
}
