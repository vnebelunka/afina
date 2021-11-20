#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

namespace Afina {
namespace Coroutine {




void Engine::Store(context &ctx) {
    char a = 'a';
    ctx.Hight = &a;

    size_t store_sz = ctx.Low - ctx.Hight;
    auto &stack = std::get<0>(ctx.Stack);
    auto &old_sz = std::get<1>(ctx.Stack);
    if(old_sz < store_sz) {
        delete[] stack;
        stack = new char[store_sz];
    }
    old_sz = store_sz;
    std::memcpy(stack, ctx.Hight, store_sz);
}

void Engine::Restore(context &ctx) {
    char stack_pos;
    if (&stack_pos >= ctx.Hight && &stack_pos <= ctx.Low) {
        Restore(ctx);
    }
    std::memcpy(ctx.Hight, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    if(alive != nullptr){
        if(alive == cur_routine){
            if(alive->next) {
                sched(alive->next);
            } else {
                return;
            }
        } else {
            sched(alive);
        }
    }
}

void Engine::sched(void *routine_) {
    if (routine_ == nullptr) {
        yield();
    } else {
        context *ctx = (context *)routine_;
        if(cur_routine && cur_routine != routine_) { // No need to save again
            if (setjmp(cur_routine->Environment) > 0) {
                return;
            }
            Store(*cur_routine);
        }
        cur_routine = ctx;
        Restore(*ctx);
    }
}
} // namespace Coroutine
} // namespace Afina