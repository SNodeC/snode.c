#include <cstdio>
#include <dlfcn.h>

extern "C" {

static int spaces = 0;

static void __attribute__((no_instrument_function)) _print_pretty(void* this_fn, bool is_enter, int in_spaces) {
    Dl_info info;
    int rc = 0;
    rc = dladdr(this_fn, &info);
    if (rc == 0) {
        printf("couldn't decode: %p\n", __builtin_return_address(0));
    }
    for (int iter = 0; iter < in_spaces; ++iter) {
        printf(" ");
    }
    printf("[%s] %s\n", is_enter ? "+" : "-", info.dli_sname);
}

void __attribute__((no_instrument_function)) __cyg_profile_func_enter(void* this_fn, [[maybe_unused]] void* call_site) {
    _print_pretty(this_fn, true, spaces++);
}

void __attribute__((no_instrument_function)) __cyg_profile_func_exit(void* this_fn, [[maybe_unused]] void* call_site) {
    _print_pretty(this_fn, false, --spaces);
}

}
