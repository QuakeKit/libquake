#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

using namespace std::chrono;

class Metrics {
public:
  using Clock = std::chrono::high_resolution_clock;
  using ms = std::chrono::milliseconds;

  static Metrics &instance() {
    if (m_instance == nullptr) {
      m_instance = new Metrics();
    }
    return *m_instance;
  }

  void startTimer(const std::string &eventName) { m_timers[eventName] = Clock::now(); }

  void finalizeTimer(const std::string &eventName) {
    auto it = m_timers.find(eventName);
    if (it != m_timers.end()) {
      m_metrics[eventName] = duration_cast<ms>(Clock::now() - it->second).count();
    }
  }

  long long getMetrics(const std::string &eventName) { return m_metrics[eventName]; }

private:
  std::unordered_map<std::string, Clock::time_point> m_timers;
  std::unordered_map<std::string, long long> m_metrics;
  inline static Metrics *m_instance = nullptr;
};