#ifndef _LOG_H
#define _LOG_H
#include <string>
#include <sstream>
namespace util {
    namespace log {
        typedef size_t sink_id;
        typedef enum log_level_ {
            NO_LEVEL = 0,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL,
            MAX_LEVEL
        } log_level;
        typedef enum split_interval_ {
            ONE_HOUR = 60 * 60,
            TWO_HOURS = 2 * ONE_HOUR,
            HALF_DAY = 12 * ONE_HOUR,
            A_DAY = 24 * ONE_HOUR
        } split_interval;

        typedef enum split_size_ {
            SIZE_1MB = 1024 * 1024,
            SIZE_2MB = 2 * SIZE_1MB,
            SIZE_10MB = 5 * SIZE_2MB
        } split_size;

        typedef enum split_type_ {
            BY_TIME,
            BY_SIZE,
            BY_LINES
        } split_type;

        /*typedef struct sinker_ {
            log_level level;
            split_type s_type;
            split_interval interval;
            split_size size;
            std::string log_file;
        } sinker;*/

        class sinker {
        public:
            sinker()
                : level(DEBUG),
                  s_type(BY_TIME),
                  interval(ONE_HOUR),
                  size(SIZE_1MB),
                  lines(100000),
                  log_file("")
            {
            }

            log_level level;
            split_type s_type;
            split_interval interval;
            split_size size;
            u_int64_t lines;
            std::string log_file;

//            sinker &operator=(const sinker &other) = default;

            /*sinker &operator=(const sinker &other) {
                level = other.level;
                s_type = other.s_type;
                interval = other.interval;
                size = other.size;
                lines = other.lines;
                log_file = other.log_file;
                return *this;
            }*/

        };

        class message {
        public:
            //message();
            message(sink_id sk_id, log_level level, const std::string &prefix);
            message(sink_id sk_id, log_level level, bool lprefix, const char* file, int line, const char* fun);
            ~message();
            message& operator<<(bool val);
            message& operator<<(short val);
            message& operator<<(unsigned short val);
            message& operator<<(int val);
            message& operator<<(unsigned int val);
            message& operator<<(long val);
            message& operator<<(unsigned long val);
            message& operator<<(float val);
            message& operator<<(double val);
            message& operator<<(long double val);
            message& operator<<(char c);
            message& operator<<(signed char c);
            message& operator<<(unsigned char c);
            message& operator<<(const char* s);
            message& operator<<(const signed char* s);
            message& operator<<(const unsigned char* s);
            message& operator<<(void* val);
            message& operator<<(std::streambuf* sb);
            message& operator<<(const std::string& str);
        private:
            sink_id m_sk_id;
            std::stringstream m_sbuf;
            log_level m_level;
        };

        void log_ln_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...);
        void log_ld_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...);
        void log_li_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...);
        void log_lw_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...);
        void log_le_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...);
        void log_lf_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...);
    }

}

using namespace ::util::log;

namespace util {
    namespace log {
        bool log_init();

        /**
         *
         * @param sk
         * @param [out]id sink id is valid only when return true
         * @return true: add sinker successfully; false: failed
         */
        bool log_add_sinker(sinker &sk, sink_id &id);

        int log_close();

#define slog_cs(sk_id, custom_prefix) message(sk_id, MAX_LEVEL, custom_prefix)
#define slog_ln(sk_id) message(sk_id, NO_LEVEL, true, __FILE__, __LINE__, __FUNCTION__)
#define slog_ld(sk_id) message(sk_id, DEBUG, true, __FILE__, __LINE__, __FUNCTION__)
#define slog_li(sk_id) message(sk_id, INFO, true, __FILE__, __LINE__, __FUNCTION__)
#define slog_lw(sk_id) message(sk_id, WARN, true, __FILE__, __LINE__, __FUNCTION__)
#define slog_le(sk_id) message(sk_id, ERROR, true, __FILE__, __LINE__, __FUNCTION__)
#define slog_lf(sk_id) message(sk_id, FATAL, true, __FILE__, __LINE__, __FUNCTION__)

#define slog_n(sk_id) message(sk_id, NO_LEVEL, false, __FILE__, __LINE__, __FUNCTION__)
#define slog_d(sk_id) message(sk_id, DEBUG, false, __FILE__, __LINE__, __FUNCTION__)
#define slog_i(sk_id) message(sk_id, INFO, false, __FILE__, __LINE__, __FUNCTION__)
#define slog_w(sk_id) message(sk_id, WARN, false, __FILE__, __LINE__, __FUNCTION__)
#define slog_e(sk_id) message(sk_id, ERROR, false, __FILE__, __LINE__, __FUNCTION__)
#define slog_f(sk_id) message(sk_id, FATAL, false, __FILE__, __LINE__, __FUNCTION__)

#define log_ln(sk_id, fmt, ...) log_ln_internal(sk_id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_ld(sk_id, fmt, ...) log_ld_internal(sk_id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_li(sk_id, fmt, ...) log_li_internal(sk_id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_lw(sk_id, fmt, ...) log_lw_internal(sk_id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_le(sk_id, fmt, ...) log_le_internal(sk_id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define log_lf(sk_id, fmt, ...) log_lf_internal(sk_id, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

        void log_cs(sink_id sk_id, const char *format, ...);

        void log_n(sink_id sk_id, const char *format, ...);

        void log_d(sink_id sk_id, const char *format, ...);

        void log_i(sink_id sk_id, const char *format, ...);

        void log_w(sink_id sk_id, const char *format, ...);

        void log_e(sink_id sk_id, const char *format, ...);

        void log_f(sink_id sk_id, const char *format, ...);
    }
}

#endif //LOG_H
