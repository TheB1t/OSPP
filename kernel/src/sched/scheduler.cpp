#include <sched/scheduler.hpp>
#include <mm/pmm.hpp>
#include <mm/vmm.hpp>
#include <driver/pit.hpp>
#include <klibcpp/kstd.hpp>
#include <sys/smp.hpp>
#include <log.hpp>

namespace sched {

    static void dummy_func() {
        while (true)
            __hlt;
    }

    Task::Task(uint32_t id, const char* name, smp::BaseCoreStack* task_stack)
        : id(id), state(TaskState::READY), name(name) {

        if (task_stack) {
            stack     = task_stack;
            stack_ptr = task_stack->initial_sp();
            own_stack = false;
            ctx.eax   = 0;
            ctx.ebx   = 0;
        } else {
            stack     = new smp::BaseCoreStack;
            stack_ptr = stack->initial_sp();
            own_stack = true;

            uint32_t sp   = kstd::get_stack_pointer();

            auto*    test = smp::BaseCoreStack::from_sp(sp);
            stack->copy_anchor_from(*test);

            ctx.base.eip = (uint32_t)&Task::_crt_task_entry;
            ctx.eax      = (uint32_t)dummy_func;
            ctx.ebx      = stack_ptr;
        }

        ctx.ecx         = 0;
        ctx.edx         = 0;
        ctx.esi         = 0;
        ctx.edi         = 0;
        ctx.ebp         = 0;
        ctx.esp         = 0;

        ctx.ds          = 0x10;
        ctx.es          = 0x10;
        ctx.fs          = 0x10;
        ctx.gs          = 0x10;

        ctx.int_no      = 0;
        ctx.err_code    = 0;

        ctx.base.cs     = 0x8;
        ctx.base.eflags = 0x202;

    }

    Task::~Task() {
        if (stack->base() && own_stack)
            delete stack;
    }

    Scheduler::Scheduler(uint32_t time_slice_ms)
        : current_task_index(0), next_task_id(1),
          initialized(false), time_slice_ms(time_slice_ms) {
        kstd::InterruptGuard guard;

        if (initialized.load(kstd::MemoryOrder::Acquire))
            return;

        this->time_slice_ms = time_slice_ms;

        pit::register_handler(timer_tick, this, pit::TimerTrigger::Interval, time_slice_ms * 1000);

        idt::register_isr(48, [](uint32_t, idt::BaseInterruptFrame* base_ctx) {
                if (idt::get_isr_frame_kind<48>() == idt::InterruptFrameKind::ContextSwitch) {
                    idt::InterruptFrame* ctx = idt::InterruptFrame::from_base(base_ctx);
                    smp::Core* current_core  = smp::CoreManager::current_core();
                    current_core->scheduler().schedule(ctx);
                } else {
                    LOG_WARN("[sched] received reschedule interrupt without extended context\n");
                }
            });

        create_task("idle", []() {
                while (true)
                    __pause;
            });

        initialized.store(true, kstd::MemoryOrder::Release);

        LOG_INFO("[sched] Initialized with %ums time slice\n", time_slice_ms);
    }

    Scheduler::~Scheduler() {
        kstd::InterruptGuard guard;

        initialized.store(false, kstd::MemoryOrder::Release);

        pit::unregister_handler(timer_tick, this);
        idt::unregister_isr(48);
    }

    void Scheduler::timer_tick(idt::InterruptFrame* ctx, void* ptr) {
        Scheduler* c = reinterpret_cast<Scheduler*>(ptr);
        c->schedule(ctx);
    }

    Task& Scheduler::create_task(const char* name, void (*entry_point)()) {
        kstd::InterruptSpinLockGuard guard(lock);
        Task& task = tasks.emplace(next_task_id++, name, nullptr);

        task.ctx.eax = (uint32_t)entry_point;

        return task;
    }

    void Scheduler::schedule(idt::InterruptFrame* ctx) {
        kstd::InterruptSpinLockGuard guard(lock);

        if (!initialized.load(kstd::MemoryOrder::Acquire) || tasks.empty())
            return;

        /*
            By design there are always two tasks: "idle" and "kernel".
            If only one task exists at this point, it means init() has just run
            and we still need to create the "kernel" task, because `ctx` currently
            contains the BSP kernel context.
        */
        if (tasks.live_count() <= 1) {
            Task& task = tasks.emplace(next_task_id++, "kernel", &stack);

            memcpy((uint8_t*)&task.ctx, (uint8_t*)ctx, sizeof(idt::InterruptFrame));

            current_task_index = 1;
        } else if (tasks.occupied(current_task_index)) {
            Task& current = tasks[current_task_index];
            if (current.state == TaskState::RUNNING) {
                current.stack_ptr = (uint32_t)ctx + sizeof(idt::InterruptFrame);
                memcpy((uint8_t*)&current.ctx, (uint8_t*)ctx, sizeof(idt::InterruptFrame));

                if (ctx->eax == 0xDEADDEAD && ctx->ebp == 0x00000000) {
                    current.state = TaskState::TERMINATED;
                    LOG_INFO("[sched] Task %u (%s) terminated\n", current.id, current.name);
                } else {
                    current.state = TaskState::READY;
                }
            }
        }

        size_t start_index = current_task_index;
        do {
            current_task_index = (current_task_index + 1) % tasks.capacity();

            if (!tasks.occupied(current_task_index))
                continue;

            Task& next = tasks[current_task_index];
            if (next.state == TaskState::READY) {
                next.state = TaskState::RUNNING;
                memcpy((uint8_t*)ctx, (uint8_t*)&next.ctx, sizeof(idt::InterruptFrame));
                return;
            }

        } while (current_task_index != start_index);

        if (tasks.occupied(current_task_index)) {
            Task& current = tasks[current_task_index];
            current.state = TaskState::RUNNING;
        }
    }

    void Scheduler::yield() {
        if (!initialized.load(kstd::MemoryOrder::Acquire))
            return;

        kstd::trigger_interrupt<48>();
    }

    Task& Scheduler::current_task() {
        return tasks[current_task_index];
    }
}
