#include ""xinu.h"" 

#ifndef kprintf 
void kprintf(const char *format, ...) {
    va_list ap;
    char buffer[2048]; 
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap); 
    va_end(ap);
    printf(""%s"", buffer); 
    fflush(stdout);     
}
#endif

void initialize_system(void) {
    // kprintf(""Minimal Xinu system initialization for simulation...\\n"");
}
