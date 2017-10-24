#include <log.h>
#include <unistd.h>
#include <thread>

sink_id g_sk1_id;
sink_id g_sk2_id;

void f1() {
    while (1) {
        sleep(1);
//        slog_d(g_sk1_id) << "Test log in f1";
//        slog_li(g_sk1_id) << "Test log in f1";
        log_d(g_sk1_id, "%s","Test log in f1");
        log_lw(g_sk1_id, "%s","Test log in f1");
    }
}
void f2() {
    while (1) {
        sleep(1);
//        slog_d(g_sk2_id) << "Test log in f2";
//        slog_li(g_sk2_id) << "Test log in f2";
        log_d(g_sk2_id, "%s","Test log in f2");
        log_lw(g_sk2_id, "%s","Test log in f2");
    }
}

int main() {
    log_init();
    ::util::log::sinker sk1;
    ::util::log::sinker sk2;
    sk1.log_file = "./log/test.log";
    sk1.interval = ::util::log::ONE_HOUR;
    sk1.level = ::util::log::DEBUG;
    sk2.log_file = "./log/test.stat";
    sk2.interval = ::util::log::ONE_HOUR;
    sk2.level = ::util::log::DEBUG;

    log_add_sinker(sk1, g_sk1_id);
    log_add_sinker(sk2, g_sk2_id);

    std::thread t1(f1);
    std::thread t2(f2);
    t1.join();
    t2.join();
    return 0;

}