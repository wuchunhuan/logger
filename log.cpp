#include <iostream>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>

#ifdef USE_BOOST
#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
//#include <boost/date_time/gregorian/gregorian.hpp>
//#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#else

#include <mutex>
#include <condition_variable>
#include <thread>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#endif

#include <fstream>
#include <queue>
#include <map>
#include "log.h"

using namespace std;
#ifdef USE_BOOST
using namespace boost;
//using namespace boost::posix_time;
using namespace boost::filesystem;

#endif
//using namespace util::log;
namespace util {
    namespace log {
        typedef struct msg_data_ {
            sink_id sk_id;
            std::string msg;
        } msg_data;

        class msg_queue {
        private:
            bool enabled;
            std::queue<msg_data> the_queue;
#ifdef USE_BOOST
            mutable boost::mutex the_mutex;
            boost::condition_variable the_condition_variable;
#else
            mutable std::mutex the_mutex;
            std::condition_variable the_condition_variable;
#endif
        public:
            msg_queue();

            ~msg_queue();

            void destroy();

            bool send(msg_data const &value);

            bool send(msg_data const &&value);

            bool empty() const;

            bool try_receive(msg_data &popped_value);

            bool receive(msg_data &popped_value);

        };

        msg_queue::msg_queue() : enabled(true) {

        }

        msg_queue::~msg_queue() {
            destroy();
        }

        void msg_queue::destroy() {
            enabled = false;
            the_condition_variable.notify_all();
        }

        bool msg_queue::send(msg_data const &value) {
            if (enabled) {
#ifdef USE_BOOST
                boost::mutex::scoped_lock lock(the_mutex);
#else
                std::unique_lock<std::mutex> lock(the_mutex);
#endif
                the_queue.push(value);
                lock.unlock();
                the_condition_variable.notify_one();
                return true;
            } else {
                return false;
            }
        }

        bool msg_queue::send(msg_data const &&value) {
            if (enabled) {
#ifdef USE_BOOST
                boost::mutex::scoped_lock lock(the_mutex);
#else
                std::unique_lock<std::mutex> lock(the_mutex);
#endif
                the_queue.push(std::move(value));
                lock.unlock();
                the_condition_variable.notify_one();
                return true;
            } else {
                return false;
            }
        }

        bool msg_queue::empty() const {
#ifdef USE_BOOST
            boost::mutex::scoped_lock lock(the_mutex);
#else
            std::unique_lock<std::mutex> lock(the_mutex);
#endif
            return the_queue.empty();
        }

        bool msg_queue::try_receive(msg_data &popped_value) {
            if (enabled) {
#ifdef USE_BOOST
                boost::mutex::scoped_lock lock(the_mutex);
#else
                std::unique_lock<std::mutex> lock(the_mutex);
#endif
                if (the_queue.empty()) {
                    return false;
                }

                popped_value = the_queue.front();
                the_queue.pop();
                return true;
            } else {
                return false;
            }
        }

        bool msg_queue::receive(msg_data &popped_value) {
#ifdef USE_BOOST
            boost::mutex::scoped_lock lock(the_mutex);
#else
            std::unique_lock<std::mutex> lock(the_mutex);
#endif
            while (the_queue.empty()) {
                if (enabled) {
                    the_condition_variable.wait(lock);
                } else {
                    return false;
                }
            }

            popped_value = the_queue.front();
            the_queue.pop();
            return true;
        }

        typedef struct sinker_internal_ {
            sinker sk;
            std::ofstream* m_ofs;
        } sinker_internal;

        class logger {
        public:
            friend class message;

//            int init(string log_path, int max_msg_size, int max_bytes_num, split_interval interval, log_level level);
            bool init();

            bool add_sinker(sinker& sk, sink_id& id);

            static logger *get_instance();

            bool get_stat();

            log_level get_level(sink_id sk_id);

            string get_prefix(string level);

            string get_lprefix(string level, const char* file, int line, const char* fun);

//    void log_internal(const char * prefix, const char *format, ...);
            void log_internal(sink_id sk_id, const char *prefix, const char *format, va_list arg);

            int close();
        private:

            logger();

            void thread();

            time_t last_sharp_time();

            int log_file_cut(sink_id sk_id);

            int rotate_files();

            int sink(sink_id sk_id, std::string& msg);

        private:
            bool m_initialized;
            int m_enabled;
            msg_queue msgq;
//            int m_msgq_id;
//            int m_max_msg_size;
#ifdef USE_BOOST
            boost::shared_ptr<boost::thread> m_thread;
#else
            std::shared_ptr<std::thread> m_thread;
#endif
//            split_interval m_split_interval;
            time_t m_last_sharp_time;
//            string m_log_path;
//            std::ofstream m_ofs;
//            log_level m_level;
            //FILE * fp;
            std::map<sink_id, sinker_internal> sinkers;
            static logger *p_instance;
        };

        logger *logger::p_instance = NULL;

        logger::logger()
            : m_initialized(false),
              m_enabled(0),
              m_last_sharp_time(0) {
        }

        logger *logger::get_instance() {
            if (p_instance == NULL) {
                p_instance = new logger();
            }
            return p_instance;
        }

        bool logger::get_stat() {
            return m_initialized;
        }

        log_level logger::get_level(sink_id sk_id) {
            auto it = sinkers.find(sk_id);
            if (it == sinkers.end()) {
                return MAX_LEVEL;
            }
            return it->second.sk.level;
        }

        time_t logger::last_sharp_time() {
            time_t tt;
            struct tm vtm;

            time(&tt);
            localtime_r(&tt, &vtm);
            vtm.tm_min = 0;
            vtm.tm_sec = 0;

            tt = mktime(&vtm);
            return tt;
        }

        int logger::rotate_files() {
            int rst = 0;
            for (auto& i : sinkers) {
                rst = log_file_cut(i.first);
                if (rst != 0) {
                    break;
                }
            }
            return rst;
        }

        int logger::log_file_cut(sink_id sk_id) {
            struct tm vtm;
            time_t now = time(0);

            auto it = sinkers.find(sk_id);
            if (it == sinkers.end()) {
                return -1;
            }

            if (now - m_last_sharp_time < it->second.sk.interval) {
                return 0;
            }

            //1.close old log file
            it->second.m_ofs->close();

            time_t tt = last_sharp_time();
            if (tt == m_last_sharp_time) {
                return 0;
            }

            char string1[128] = {0};
            localtime_r(&m_last_sharp_time, &vtm);
            strftime(string1, 128, "%Y%m%d%H", &vtm);
            string new_file_name = it->second.sk.log_file + string1;
#ifdef USE_BOOST
            rename(path(m_log_path), path(new_file_name));
#else
            rename(it->second.sk.log_file.c_str(), new_file_name.c_str());
#endif

            m_last_sharp_time = tt;

            //3.open new log file
            it->second.m_ofs->open(it->second.sk.log_file.c_str(), ios_base::out | ios_base::app);
            if (!it->second.m_ofs->is_open()) {
                return -1;
            }

            return 0;
        }

        int logger::sink(sink_id sk_id, std::string& msg) {
            auto it = sinkers.find(sk_id);
            if (it == sinkers.end()) {
                return -1;
            }

            if (it->second.m_ofs->is_open()) {
                *it->second.m_ofs << msg << endl;
                it->second.m_ofs->flush();
            }
            return 0;
        }

        bool logger::add_sinker(sinker& sk, sink_id& id) {
            sink_id hash_id = std::hash<std::string>()(sk.log_file);
            auto it = sinkers.find(hash_id);
            if (it == sinkers.end()) {

#ifdef USE_BOOST
                path p(log_path);
                if (!exists(p.parent_path())) {
                    create_directories(p.parent_path());
                }
#else
                unsigned long size = sk.log_file.size() + 1;
                char sz[size];
                strncpy(sz, sk.log_file.c_str(), sk.log_file.size());
                char *parent_path = dirname(sz);
                if (access(parent_path, F_OK) != 0) {
//                    mkdir(parent_path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                    char mkcmd[size + 10];
                    snprintf(mkcmd, size + 10, "mkdir -p %s", parent_path);
                    system(mkcmd);
                }
#endif

                sinker_internal sk_i;
                sk_i.sk = sk;
                sk_i.m_ofs = new std::ofstream;
                if (sk_i.m_ofs == NULL) {
                    return false;
                }
                sk_i.m_ofs->open(sk.log_file.c_str(), ios_base::out | ios_base::app);
                if (!sk_i.m_ofs->is_open()) {
                    return false;
                }
                sinkers[hash_id] = sk_i;
            }
            id = hash_id;
            return true;
        }

        void logger::thread() {
//    char msg_buf[sizeof(long) + m_max_msg_size];
//    int result;

            do {
                msg_data data;
                bool rst = msgq.receive(data);
                log_file_cut(data.sk_id);
                if (rst) {
                    sink(data.sk_id, data.msg);
                }
                /*if (m_ofs.is_open() && rst) {
                    m_ofs << data << endl;
                    m_ofs.flush();
                }*/
            } while (m_enabled);
        }

        bool logger::init() {
            if (m_initialized) {
                return true;
            }

            m_enabled = 1;
            m_last_sharp_time = last_sharp_time();
#ifdef USE_BOOST
            m_thread.reset(new boost::thread(bind(&logger::thread, this)));
#else
            m_thread.reset(new std::thread(&logger::thread, this));
#endif

            //log_i("Init log sucessfully!");
            m_initialized = true;
            return true;
        }

        int logger::close() {
            if (!m_initialized) {
                return 0;
            }
            m_enabled = 0;
            msgq.destroy();
            m_thread->join();
            for (auto i : sinkers) {
                if (i.second.m_ofs->is_open()) {
                    i.second.m_ofs->close();
                    delete i.second.m_ofs;
                }
            }
            m_initialized = false;
            return 0;
        }

        void logger::log_internal(sink_id sk_id, const char *prefix, const char *format, va_list arg) {
            va_list arg_copy;
            va_copy(arg_copy, arg);
            const int sz = std::vsnprintf(NULL, 0, format, arg);
            std::string msg(sz, ' ');
            std::vsnprintf((char *) msg.data(), sz + 1, format, arg_copy);
            msg = prefix + msg;
            if (m_enabled == 1) {
                msg_data data;
                data.msg = msg;
                data.sk_id = sk_id;
                msgq.send(data);

            } else {
                cout << msg << endl;
            }
            va_end (arg_copy);
        }

        string logger::get_lprefix(string level, const char* file, int line, const char* fun) {
            char string1[128] = {0};
            time_t Today;
            tm *Time;
            time(&Today);
            Time = localtime(&Today);

            strftime(string1, 128, "%Y-%m-%d %H:%M:%S", Time);
            std::stringstream prefix;
            prefix << string1 << " [" << level << " ";
            prefix << file << ":" << line << ":" << fun << "]: ";
            return prefix.str();
        }

        string logger::get_prefix(string level) {
            char string1[128] = {0};
            time_t Today;
            tm *Time;
            time(&Today);
            Time = localtime(&Today);

            strftime(string1, 128, "%Y-%m-%d %H:%M:%S", Time);
            std::stringstream prefix;
            prefix << string1 << " [" << level << "]: ";
            return prefix.str();
        }

        message::message(sink_id sk_id, log_level level, const std::string &prefix) {
            m_sk_id = sk_id;
            m_level = level;
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << prefix;
            }
        }

        message::message(sink_id sk_id, log_level level, bool lprefix, const char* file, int line, const char* fun) {
            m_sk_id = sk_id;
            m_level = level;
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                std::string lstr;
                switch (level) {
                    case DEBUG:
                        lstr = "DEBUG";
                        break;
                    case INFO:
                        lstr = "INFO";
                        break;
                    case WARN:
                        lstr = "WARN";
                        break;
                    case ERROR:
                        lstr = "ERROR";
                        break;
                    case FATAL:
                        lstr = "FATAL";
                        break;
                    default:
                        lstr = "DEBUG";
                        break;
                }
                if (lprefix) {
                    m_sbuf << logger::get_instance()->get_lprefix(lstr, file, line, fun);
                } else {
                    m_sbuf << logger::get_instance()->get_prefix(lstr);
                }
            }
            return;
        }

        message::~message() {
//#ifndef LOG_DISABLE
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                if (logger::get_instance()->m_enabled == 1) {
                    msg_data data;
                    data.msg = std::move(m_sbuf.str());
                    data.sk_id = m_sk_id;
                    logger::get_instance()->msgq.send(data);
                } else {
                    cout << m_sbuf.str() << endl;
                }
            }
//#endif
        }

        message &message::operator<<(bool val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(short val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(unsigned short val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(int val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(unsigned int val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(long val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(unsigned long val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(float val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(double val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(long double val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(char c) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << c;
            }
            return *this;
        }

        message &message::operator<<(signed char c) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << c;
            }
            return *this;
        }

        message &message::operator<<(unsigned char c) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << c;
            }
            return *this;
        }

        message &message::operator<<(const char *s) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << s;
            }
            return *this;
        }

        message &message::operator<<(const signed char *s) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << s;
            }
            return *this;
        }

        message &message::operator<<(const unsigned char *s) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << s;
            }
            return *this;
        }

        message &message::operator<<(void *val) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << val;
            }
            return *this;
        }

        message &message::operator<<(streambuf *sb) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << sb;
            }
            return *this;
        }

        message &message::operator<<(const std::string &str) {
            if (logger::get_instance()->get_stat() && m_level >= logger::get_instance()->get_level(m_sk_id)) {
                m_sbuf << str;
            }
            return *this;
        }


        bool log_init() {
            return logger::get_instance()->init();
        }

        bool log_add_sinker(sinker& sk, sink_id& id) {
            return logger::get_instance()->add_sinker(sk, id);
        }

        int log_close() {
            return logger::get_instance()->close();
        }

        void log_ld_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::DEBUG >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_lprefix("DEBUG", file, line, fun);
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_li_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::INFO >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_lprefix("INFO", file, line, fun);
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_lw_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::WARN >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_lprefix("WARN", file, line, fun);
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_le_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::ERROR >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_lprefix("ERROR", file, line, fun);
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_lf_internal(sink_id sk_id, const char* file, int line, const char* fun, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::FATAL >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_lprefix("FATAL", file, line, fun);
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_d(sink_id sk_id, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::DEBUG >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_prefix("DEBUG");
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_i(sink_id sk_id, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::INFO >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_prefix("INFO");
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_w(sink_id sk_id, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::WARN >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_prefix("WARN");
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_e(sink_id sk_id, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::ERROR >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_prefix("ERROR");
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }

        void log_f(sink_id sk_id, const char *format, ...) {
            va_list arg;
            va_start (arg, format);
            if (logger::get_instance()->get_stat() && ::util::log::FATAL >= logger::get_instance()->get_level(sk_id)) {
                string prefix = logger::get_instance()->get_prefix("FATAL");
                logger::get_instance()->log_internal(sk_id, prefix.c_str(), format, arg);
            }
            va_end (arg);
        }
    }
}


