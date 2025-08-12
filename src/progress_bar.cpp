#include "progress_bar.h"
#include <sstream>
#include <cmath>

namespace popplershot {

ProgressBar::ProgressBar(int total, int width, const std::string& fill, const std::string& empty)
    : total_(total), current_(0), bar_width_(width), fill_char_(fill), empty_char_(empty),
      description_("Processing"), start_time_(std::chrono::steady_clock::now()), finished_(false) {
    display();
}

ProgressBar::~ProgressBar() {
    if (!finished_) {
        finish();
    }
}

void ProgressBar::update(int increment) {
    if (finished_) return;
    
    current_.fetch_add(increment);
    display();
}

void ProgressBar::finish() {
    if (finished_) return;
    
    current_.store(total_);
    finished_ = true;
    display();
    std::cout << std::endl;
}

void ProgressBar::set_description(const std::string& desc) {
    std::lock_guard<std::mutex> lock(display_mutex_);
    description_ = desc;
}

void ProgressBar::display() {
    std::lock_guard<std::mutex> lock(display_mutex_);
    
    int current = current_.load();
    if (current > total_) current = total_;
    
    // Calculate progress
    double progress = total_ > 0 ? static_cast<double>(current) / total_ : 0.0;
    int filled_width = static_cast<int>(progress * bar_width_);
    
    // Calculate timing
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double>(now - start_time_).count();
    
    double rate = elapsed > 0 ? current / elapsed : 0.0;
    double eta = (rate > 0 && current < total_) ? (total_ - current) / rate : 0.0;
    
    // Build progress bar
    std::ostringstream bar;
    bar << "\r" << description_ << ": ";
    bar << std::fixed << std::setprecision(1) << (progress * 100.0) << "%|";
    
    // Draw bar
    for (int i = 0; i < bar_width_; ++i) {
        if (i < filled_width) {
            bar << fill_char_;
        } else {
            bar << empty_char_;
        }
    }
    
    bar << "| " << current << "/" << total_;
    
    // Add timing info
    if (elapsed > 0) {
        bar << " [" << format_time(elapsed);
        if (!finished_ && eta > 0) {
            bar << "<" << format_time(eta);
        }
        bar << ", " << std::fixed << std::setprecision(2) << rate << "it/s]";
    }
    
    // Pad to clear previous line
    bar << "    ";
    
    std::cout << bar.str() << std::flush;
}

std::string ProgressBar::format_time(double seconds) const {
    if (seconds < 60) {
        return std::to_string(static_cast<int>(seconds)) + "s";
    } else if (seconds < 3600) {
        int mins = static_cast<int>(seconds / 60);
        int secs = static_cast<int>(seconds) % 60;
        return std::to_string(mins) + ":" + 
               (secs < 10 ? "0" : "") + std::to_string(secs);
    } else {
        int hours = static_cast<int>(seconds / 3600);
        int mins = static_cast<int>(seconds / 60) % 60;
        return std::to_string(hours) + ":" + 
               (mins < 10 ? "0" : "") + std::to_string(mins) + "h";
    }
}

} // namespace popplershot
