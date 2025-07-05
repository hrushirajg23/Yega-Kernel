#include "exceptions.h"

void exception_handler(void) {
    __asm__ volatile ("cli; hlt");
    __builtin_unreachable();
}