#include <log.h>
#include <unistd.h>
#include <thread>

sink_id g_sk1_id;
sink_id g_sk2_id;
sink_id g_sk3_id;
sink_id g_sk4_id;

void f1() {
    while (1) {
        sleep(1);
        slog_d(g_sk1_id) << "Test log in f1";
        slog_li(g_sk1_id) << "Test log in f1";
		//slog_cs(g_sk1_id, "Hi there :)") << " Test log in f1";
		slog_cs(g_sk1_id, "") << "Test log in f1";
		log_cs(g_sk1_id, "[Danger] Test log in f1");
        //log_d(g_sk1_id, "%s","Test log in f1");
        //log_lw(g_sk1_id, "%s","Test log in f1");
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

void f3() {
    while (1) {
        sleep(1);
//        slog_d(g_sk2_id) << "Test log in f3";
//        slog_li(g_sk2_id) << "Test log in f3";
        log_d(g_sk3_id, "%s","Test log in f3");
        log_lw(g_sk3_id, "%s","Test log in f3");
    }
}

void f4() {
    while (1) {
        sleep(1);
//        slog_d(g_sk2_id) << "Test log in f3";
//        slog_li(g_sk2_id) << "Test log in f3";
        log_d(g_sk4_id, "%s","Test log in f4");
        log_lw(g_sk4_id, "%s","Test log in f4");
    }
}

int main() {
    log_init();
    ::util::log::sinker sk1;
    ::util::log::sinker sk2;
    ::util::log::sinker sk3;
    ::util::log::sinker sk4;
    sk1.s_type = ::util::log::BY_TIME;
    sk1.log_file = "./log/time.log";
    sk1.interval = ::util::log::ONE_HOUR;
    sk1.level = ::util::log::DEBUG;

    sk2.s_type = ::util::log::BY_LINES;
    sk2.sk_type = ::util::log::BY_BUF_SIZE;
    sk2.sink_buf_size = 1024;
    sk2.log_file = "./log/line.stat";
    sk2.lines = 5000;
    sk2.level = ::util::log::DEBUG;

    sk3.s_type = ::util::log::BY_SIZE;
    sk3.sk_type = ::util::log::BY_LINE;
    sk3.log_file = "./log/size.stat";
    sk3.size = (split_size)200000;
    sk3.level = ::util::log::DEBUG;

    sk4.s_type = ::util::log::BY_DATE;
    sk4.log_file = "./log/date.stat";
    sk4.level = ::util::log::DEBUG;

    log_add_sinker(sk1, g_sk1_id);
    log_add_sinker(sk2, g_sk2_id);
    log_add_sinker(sk3, g_sk3_id);
    log_add_sinker(sk4, g_sk4_id);

    std::thread t1(f1);
    std::thread t2(f2);
    std::thread t3(f3);
    std::thread t4(f4);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    return 0;

}

