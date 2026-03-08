#pragma once

#include <klibcpp/atomic.hpp>
#include <klibcpp/spinlock.hpp>
#include <klibcpp/static_array.hpp>
#include <klibcpp/cstdint.hpp>
#include <klibcpp/trivial.hpp>
#include <int/idt.hpp>
#include <sys/smp.hpp>

namespace sched {

    enum class TaskState {
        READY,
        RUNNING,
        BLOCKED,
        TERMINATED
    };

    struct Task {
        public:
            uint32_t            id;

            idt::InterruptFrame ctx;

            uint32_t            stack_ptr;
            smp::BaseCoreStack* stack;
            bool                own_stack;

            TaskState           state;
            const char*         name;

            Task(uint32_t id, const char* name, smp::BaseCoreStack* task_stack);
            ~Task();

            static void _die() {
                __asm__ volatile (
                     /*
                         TODO:
                             We should call exit() here, but we don't have any system calls yet.
                             So we set eax to magic word to indicate that the task has terminated.
                             Scheduler will check eax and also ebp to determine if the task has terminated.
                     */
                     "xorl %ebp, %ebp\n"
                     "movl $0xdeaddead, %eax\n"
                     "1:\n"
                     "   pause\n"
                     "   jmp 1b\n"
                );
            }

        private:
            static void _crt_task_entry() {
                /*
                    EAX - entry point
                    EBX - stack pointer
                */
                __asm__ volatile (
                     "xorl %ebp, %ebp\n"
                     "movl %ebx, %esp\n"
                     "call *%eax\n"
                );
                _die();
            }
    };

    class Scheduler : public NonTransferable {
        private:
            kstd::StaticArray<Task, 256> tasks;
            uint32_t current_task_index;
            uint32_t next_task_id;
            kstd::Atomic<bool> initialized;
            uint32_t time_slice_ms;
            kstd::SpinLock lock;

            static void timer_tick(idt::InterruptFrame* ctx, void*);

        public:
            Scheduler(uint32_t time_slice_ms = 10);
            ~Scheduler();

            bool is_initialized() const { return initialized.load(kstd::MemoryOrder::Acquire); }

            Task&       create_task(const char* name, void (*entry_point)());

            void        schedule(idt::InterruptFrame* ctx);
            void        yield();

            Task&       current_task();
    };
}
