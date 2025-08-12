#include <sched/scheduler.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <driver/pit.hpp>
#include <klibcpp/kstd.hpp>
#include <log.hpp>

namespace sched {

    static void dummy_func() {
        LOG_INFO("Task runs dummy_func because eip not specified\n");
        while (true)
            __hlt;
    }

    Task::Task(uint32_t id, const char* name)
        : id(id), stack_size(0),
        state(TaskState::READY), name(name)
    {
        stack_base = 0;
        stack_ptr = 0;

        memset((uint8_t*)&ctx, 0, sizeof(idt::InterruptContext));

        ctx.ds = 0x10;
        ctx.es = 0x10;
        ctx.fs = 0x10;
        ctx.gs = 0x10;

        ctx.base.eip = 0;
        ctx.base.cs = 0x8;
        ctx.base.eflags = 0x202;

        LOG_INFO("Created task %d (%s) without stack\n", id, name);
    }

    Task::Task(uint32_t id, const char* name, uint32_t stack_size)
        : id(id), stack_size(stack_size),
        state(TaskState::READY), name(name)
    {
        stack_base = (uint32_t)new uint8_t[stack_size];
        stack_ptr = stack_base + stack_size;

        ctx.eax = (uint32_t)dummy_func;
        ctx.ebx = stack_ptr;
        ctx.ecx = 0;
        ctx.edx = 0;
        ctx.esi = 0;
        ctx.edi = 0;
        ctx.ebp = 0;
        ctx.esp = 0;

        ctx.ds = 0x10;
        ctx.es = 0x10;
        ctx.fs = 0x10;
        ctx.gs = 0x10;

        ctx.int_no = 0;
        ctx.err_code = 0;

        ctx.base.eip = (uint32_t)&Task::_crt_task_entry;
        ctx.base.cs = 0x8;
        ctx.base.eflags = 0x202;

        LOG_INFO("Created task %d (%s) with stack at 0x%08x\n", id, name, stack_base);
    }

    Task::~Task() {
        if (stack_base) {
            delete[] (uint8_t*)stack_base;
        }

        LOG_INFO("Destroyed task %d (%s)\n", id, name);
    }

    Scheduler::Scheduler()
        : current_task_index(0), next_task_id(1),
        initialized(false), time_slice_ms(10)
    {}

    Scheduler::~Scheduler() {
        kstd::InterruptGuard guard;

        initialized = false;

        pit::unregister_handler(timer_tick, this);
        idt::unregister_isr(33);

        for (auto* task : tasks) {
            delete task;
        }
    }

    void Scheduler::timer_tick(idt::InterruptContext* ctx, void* ptr) {
        Scheduler* c = reinterpret_cast<Scheduler*>(ptr);
        c->schedule(ctx);
    }

    void Scheduler::init(uint32_t time_slice_ms) {
        kstd::InterruptGuard guard;

        if (initialized)
            return;

        this->time_slice_ms = time_slice_ms;

        pit::register_handler(timer_tick, this, pit::TimerTrigger::Interval, time_slice_ms * 1000);

        idt::register_isr(33, [](uint32_t, bool has_ext, idt::BaseInterruptContext* base_ctx) {
            if (has_ext) {
                idt::InterruptContext* ctx = idt::InterruptContext::from_base(base_ctx);
                get()->schedule(ctx);
            } else {
                LOG_WARN("Received rechedule interrupt without extended context!\n");
            }
        });

        create_task("idle", []() {
            while (true)
                __pause;
        });

        initialized = true;

        LOG_INFO("Scheduler initialized with %dms time slice\n", time_slice_ms);
    }

    Task* Scheduler::create_task(const char* name, void (*entry_point)(), uint32_t stack_size) {
        Task* task = new Task(next_task_id++, name, stack_size);

        task->ctx.eax = (uint32_t)entry_point;
        tasks.push_back(task);

        return task;
    }

    void Scheduler::schedule(idt::InterruptContext* ctx) {
        if (!initialized || tasks.empty())
            return;

        /*
            By design there are always two tasks: "idle" and "kernel".
            If only one task exists at this point, it means init() has just run
            and we still need to create the "kernel" task, because `ctx` currently
            contains the BSP kernel context.
        */
        if (tasks.size() <= 1) {
            Task* task = new Task(next_task_id++, "kernel");

            memcpy((uint8_t*)&task->ctx, (uint8_t*)ctx, sizeof(idt::InterruptContext));

            tasks.push_back(task);
            current_task_index = 1;
        } else if (current_task_index < tasks.size()) {
            Task* current = tasks[current_task_index];
            if (current->state == TaskState::RUNNING) {
                current->stack_ptr = (uint32_t)ctx + sizeof(idt::InterruptContext);
                memcpy((uint8_t*)&current->ctx, (uint8_t*)ctx, sizeof(idt::InterruptContext));

                if (ctx->eax == 0xDEADDEAD && ctx->ebp == 0x00000000) {
                    current->state = TaskState::TERMINATED;
                    LOG_INFO("Task %d (%s) terminated\n", current->id, current->name);
                } else {
                    current->state = TaskState::READY;
                }
            }
        }

        size_t start_index = current_task_index;
        do {
            current_task_index = (current_task_index + 1) % tasks.size();

            Task* next = tasks[current_task_index];
            if (next->state == TaskState::READY) {
                next->state = TaskState::RUNNING;
                memcpy((uint8_t*)ctx, (uint8_t*)&next->ctx, sizeof(idt::InterruptContext));
                return;
            }

        } while (current_task_index != start_index);

        if (current_task_index < tasks.size()) {
            Task* current = tasks[current_task_index];
            current->state = TaskState::RUNNING;
        }
    }

    void Scheduler::yield() {
        if (!initialized)
            return;

        kstd::trigger_interrupt<33>();
    }

    Task* Scheduler::current_task() {
        if (!initialized)
            return nullptr;

        return tasks[current_task_index];
    }
}