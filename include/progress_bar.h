#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>

namespace popplershot {

class ProgressBar {
public:
    ProgressBar(int total, int width = 50, const std::string& fill = "█", const std::string& empty = "░");
    ~ProgressBar();

    void update(int increment = 1);
    void finish();
    void set_description(const std::string& desc);

private:
    void display();
    std::string format_time(double seconds) const;
    
    int total_;
    std::atomic<int> current_;
    int bar_width_;
    std::string fill_char_;
    std::string empty_char_;
    std::string description_;
    
    std::chrono::steady_clock::time_point start_time_;
    std::mutex display_mutex_;
    bool finished_;
};

} // namespace popplershot
