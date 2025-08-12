#pragma once

#include <klibcpp/vector.hpp>
#include <klibcpp/cstdint.hpp>
#include <int/idt.hpp>

namespace sched {

    enum class TaskState {
        READY,
        RUNNING,
        BLOCKED,
        TERMINATED
    };

    struct Task {
        public:
            uint32_t id;

            idt::InterruptContext ctx;

            uint32_t stack_ptr;
            uint32_t stack_base;
            uint32_t stack_size;

            TaskState state;
            const char* name;
            
            Task(uint32_t id, const char* name, uint32_t stack_size);
            ~Task();

        private:
            static void _crt_task_entry() {
                /*
                    EAX - entry point
                    EBX - stack pointer
                */
                INLINE_ASSEMBLY(
                    "xorl %ebp, %ebp\n"
                    "movl %ebx, %esp\n"
                    "call *%eax\n"

                    /*
                        TODO:
                            We should call exit() here, but we don't have any system calls yet.
                            So we set eax to magic word to indicate that the task has terminated.
                            Scheduler will check eax and also ebp to determine if the task has terminated.
                    */
                    "movl $0xdeaddead, %eax\n"
                    "1:\n"
                    "   pause\n"
                    "   jmp 1b\n"
                );
            }
    };

    class Scheduler {
        private:           
            kstd::vector<Task*> tasks;
            uint32_t current_task_index;
            uint32_t next_task_id;
            bool initialized;
            uint32_t time_slice_ms;
            
        public:
            Scheduler();
            ~Scheduler();

            static Scheduler* get();
            
            void init(void (*entry_point)(), uint32_t time_slice_ms = 10);
            
            Task* create_task(const char* name, void (*entry_point)(), uint32_t stack_size = 4096);

            void schedule(idt::InterruptContext* ctx);
            void yield();

            Task* current_task();
            void block_current();
            void unblock(uint32_t task_id);
            void terminate_current();
            bool is_initialized() const { return initialized; }
    };
}