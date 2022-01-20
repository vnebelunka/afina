#include <afina/coroutine/Engine.h>

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <cstring>

namespace Afina {
namespace Coroutine {




void Engine::Store(context &ctx) {
    char a = 'a';
    ctx.Low = &a;

    size_t store_sz = ctx.Hight - ctx.Low;
    auto &stack = std::get<0>(ctx.Stack);
    auto &old_sz = std::get<1>(ctx.Stack);
    if(old_sz < store_sz || old_sz > 2 * store_sz) {
        delete[] stack;
        stack = new char[store_sz];
        old_sz = store_sz;
    }
    std::memcpy(stack, ctx.Low, store_sz);
}

void Engine::Restore(context &ctx) {
    char stack_pos;
    if (&stack_pos >= ctx.Low && &stack_pos <= ctx.Hight) {
        Restore(ctx);
    }
    std::memcpy(ctx.Low, std::get<0>(ctx.Stack), std::get<1>(ctx.Stack));
    longjmp(ctx.Environment, 1);
}

void Engine::yield() {
    if(alive == nullptr || (cur_routine == alive && alive->next == nullptr)) {
        return;
    }
    // never change  to nullptr
    if(alive == cur_routine) {
        change_routine(alive->next);
    } else {
        change_routine(alive);
    }
}

void Engine::sched(void *routine_) {
    context *ctx = (context *)routine_;
    if(ctx == nullptr) { // as yield
        yield();
        return; // if we are here: no routine's remain
    }
    change_routine(ctx);
}

void Engine::change_routine(context *ctx){
    if(ctx == nullptr) {
        std::cerr<< "Engine::change_routine called with null pointer\n";
        return;
    }
    if(cur_routine == ctx) {
        return;
    }
    if(cur_routine != idle_ctx) { // No need to save again
        if (setjmp(cur_routine->Environment) > 0) {
            return;
        }
        Store(*cur_routine);
    }
    cur_routine = ctx;
    Restore(*ctx);
}


} // namespace Coroutine
} // namespace Afina