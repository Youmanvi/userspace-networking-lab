#pragma once
#include <chrono>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "defination.hpp"

namespace uStack {

namespace docs {
static const char* tcb_aol_doc = R"(
FILE: tcb_aol.hpp
PURPOSE: Append-only JSON log for TCB operations - complete audit trail for debugging.

Events logged:
- tcb_registered: New TCB created (remote, local, port info)
- tcb_state_changed: State transition with reason
- backlog_queued: Connection added to listener backlog
- backlog_dequeued: Connection accepted and removed from backlog
- backlog_rejected: Connection rejected due to backlog full
- connection_rejected: Connection rejected (global limit, port limit, or backlog)
- data_sent: Bytes sent on connection
- data_received: Bytes received on connection

All events include:
- timestamp (ISO8601)
- remote_addr:port
- local_addr:port (if available)
)";
}

class tcb_aol {
private:
        tcb_aol() : log_file("tcb_operations.aol", std::ios::app) {
                if (!log_file.is_open()) {
                        DLOG(FATAL) << "[TCB AOL] Failed to open log file";
                }
        }
        ~tcb_aol() {
                if (log_file.is_open()) {
                        log_file.close();
                }
        }

        std::ofstream log_file;

        std::string get_timestamp() const {
                auto now = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);
                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) % 1000;

                std::ostringstream ss;
                ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S")
                   << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
                return ss.str();
        }

        void write_event(const std::string& json) {
                if (log_file.is_open()) {
                        log_file << json << "\n";
                        log_file.flush();
                }
        }

public:
        tcb_aol(const tcb_aol&) = delete;
        tcb_aol(tcb_aol&&) = delete;
        tcb_aol& operator=(const tcb_aol&) = delete;
        tcb_aol& operator=(tcb_aol&&) = delete;

        static tcb_aol& instance() {
                static tcb_aol instance;
                return instance;
        }

        void log_tcb_registered(const std::string& remote_addr, uint16_t remote_port,
                                const std::string& local_addr, uint16_t local_port) {
                std::ostringstream ss;
                ss << "{\"event\":\"tcb_registered\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"local\":\"" << local_addr << ":" << local_port << "\"}";
                write_event(ss.str());
        }

        void log_state_changed(const std::string& remote_addr, uint16_t remote_port,
                               const std::string& local_addr, uint16_t local_port,
                               const std::string& old_state, const std::string& new_state,
                               const std::string& reason = "") {
                std::ostringstream ss;
                ss << "{\"event\":\"tcb_state_changed\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"local\":\"" << local_addr << ":" << local_port << "\","
                   << "\"old_state\":\"" << old_state << "\","
                   << "\"new_state\":\"" << new_state << "\"";
                if (!reason.empty()) {
                        ss << ",\"reason\":\"" << reason << "\"";
                }
                ss << "}";
                write_event(ss.str());
        }

        void log_backlog_queued(uint16_t port, const std::string& remote_addr,
                               uint16_t remote_port, uint32_t current, uint32_t max) {
                std::ostringstream ss;
                ss << "{\"event\":\"backlog_queued\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"port\":" << port << ","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"backlog_current\":" << current << ","
                   << "\"backlog_max\":" << max << "}";
                write_event(ss.str());
        }

        void log_backlog_dequeued(uint16_t port, const std::string& remote_addr,
                                 uint16_t remote_port, uint32_t current) {
                std::ostringstream ss;
                ss << "{\"event\":\"backlog_dequeued\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"port\":" << port << ","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"backlog_current\":" << current << "}";
                write_event(ss.str());
        }

        void log_backlog_rejected(uint16_t port, const std::string& remote_addr,
                                 uint16_t remote_port, uint32_t total_rejected) {
                std::ostringstream ss;
                ss << "{\"event\":\"backlog_rejected\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"port\":" << port << ","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"reason\":\"backlog_full\","
                   << "\"total_rejected\":" << total_rejected << "}";
                write_event(ss.str());
        }

        void log_connection_rejected(uint16_t port, const std::string& remote_addr,
                                    uint16_t remote_port, const std::string& reason) {
                std::ostringstream ss;
                ss << "{\"event\":\"connection_rejected\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"port\":" << port << ","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"reason\":\"" << reason << "\"}";
                write_event(ss.str());
        }

        void log_data_sent(const std::string& remote_addr, uint16_t remote_port,
                          const std::string& local_addr, uint16_t local_port,
                          uint32_t bytes_sent) {
                std::ostringstream ss;
                ss << "{\"event\":\"data_sent\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"local\":\"" << local_addr << ":" << local_port << "\","
                   << "\"bytes\":" << bytes_sent << "}";
                write_event(ss.str());
        }

        void log_data_received(const std::string& remote_addr, uint16_t remote_port,
                              const std::string& local_addr, uint16_t local_port,
                              uint32_t bytes_received) {
                std::ostringstream ss;
                ss << "{\"event\":\"data_received\","
                   << "\"timestamp\":\"" << get_timestamp() << "\","
                   << "\"remote\":\"" << remote_addr << ":" << remote_port << "\","
                   << "\"local\":\"" << local_addr << ":" << local_port << "\","
                   << "\"bytes\":" << bytes_received << "}";
                write_event(ss.str());
        }
};

}  // namespace uStack
