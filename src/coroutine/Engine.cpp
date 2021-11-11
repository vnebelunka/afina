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
        if(cur_routine != nullptr) {
            Store(*cur_routine);
        }
        cur_routine = (context *) alive;
        Restore(*alive);
    }
}

void Engine::sched(void *routine_) {
    if (routine_ == nullptr) {
        yield();
    } else {
        context *ctx = (context *)routine_;
        if(cur_routine != idle_ctx) { // No need to jump here if its idle_ctx
            if (setjmp(cur_routine->Environment) > 0) {
                if (cur_routine != nullptr) { // if coroutine wasn't ended
                    return;
                } else { // coroutine ended: returning to idle_ctx
                    cur_routine = idle_ctx;
                    Restore(*ctx);
                }
            }
        }
        Store(*cur_routine);
        cur_routine = ctx;
        Restore(*ctx);
    }
}
} // namespace Coroutine
} // namespace Afina