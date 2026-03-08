#pragma once

#include <klibcpp/cstdint.hpp>
#include <klibcpp/cstdlib.hpp>
#include <log.hpp>

namespace ktest {
    struct Session {
        uint32_t    suite_count         = 0;
        uint32_t    case_count          = 0;
        uint32_t    check_count         = 0;
        uint32_t    failure_count       = 0;
        uint32_t    case_failure_marker = 0;

        const char* run_name            = nullptr;
        const char* current_suite       = nullptr;
        const char* current_case        = nullptr;

        bool        started             = false;
        bool        completed           = false;

        void start(const char* name) {
            if (started)
                return;

            started  = true;
            run_name = name;

            LOG_INFO("[ktest] Starting %s\n", run_name);
        }

        void begin_suite(const char* name) {
            ++suite_count;
            current_suite = name;

            LOG_INFO("[ktest] Suite %s\n", current_suite);
        }

        void end_suite() {
            current_suite = nullptr;
        }

        void begin_case(const char* name) {
            ++case_count;
            current_case        = name;
            case_failure_marker = failure_count;

            LOG_INFO("[ktest] Case %s\n", current_case);
        }

        void end_case() {
            if (failure_count == case_failure_marker)
                LOG_INFO("[ktest] PASS %s/%s\n", current_suite, current_case);
            else
                LOG_ERR("[ktest] FAIL %s/%s\n", current_suite, current_case);

            current_case = nullptr;
        }

        bool expect(bool condition, const char* expr, const char* file, int line) {
            ++check_count;

            if (condition)
                return true;

            ++failure_count;
            LOG_ERR("[ktest] CHECK FAILED %s/%s: %s (%s:%d)\n",
                current_suite ? current_suite : "<no-suite>",
                current_case ? current_case : "<no-case>",
                expr,
                file,
                line
            );
            return false;
        }

        void complete() {
            if (completed)
                return;

            completed = true;

            if (failure_count) {
                LOG_ERR("[ktest] Summary: %u suites, %u cases, %u checks, %u failures\n",
                    suite_count, case_count, check_count, failure_count);
            } else {
                LOG_INFO("[ktest] Summary: %u suites, %u cases, %u checks, 0 failures\n",
                    suite_count, case_count, check_count);
            }
        }
    };

    inline Session& session() {
        static Session value;
        return value;
    }

    template <typename Fn>
    inline void run_case(Session& sess, const char* name, Fn&& fn) {
        sess.begin_case(name);
        fn();
        sess.end_case();
    }
}

#define KTEST_EXPECT(sess, expr) \
    ((sess).expect((expr), #expr, __FILE__, __LINE__))

#define KTEST_ASSERT(sess, expr)         \
    do {                                 \
        if (!KTEST_EXPECT((sess), expr)) \
            return;                      \
    } while (0)
