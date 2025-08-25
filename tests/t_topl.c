/* Test harness for hack.topl.c buffer overflow vulnerabilities */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define BUFSZ 256
#define CO 80
#define COLNO 80

/* Mock globals and minimal dependencies */
char toplines[BUFSZ];
int tlx = 1, tly = 1, curx = 1, cury = 1;
struct flag_s {
    int toplin;
    int standout;
} flags = {0, 0};

/* Mock functions to avoid full game dependency */
void home(void) { printf("[HOME]"); }
void cl_end(void) { printf("[CL_END]"); }
void putchar_mock(char c) { putchar(c); }
void nscr(void) {}
void standoutbeg(void) { printf("[SO_BEGIN]"); }
void standoutend(void) { printf("[SO_END]"); }
void xwaitforspace(const char *s) { printf("[WAIT:%s]", s ? s : ""); }
void docorner(int x, int y) { printf("[CORNER:%d,%d]", x, y); }
char *index(const char *s, int c) { return strchr(s, c); }
char *eos(char *s) { return s + strlen(s); }

/* Test version of pline() with exact same logic as original */
void test_pline(const char *line, ...) {
    char pbuf[BUFSZ];
    char *bp = pbuf, *tl;
    int n, n0;
    va_list args;

    if (!line || !*line) return;
    
    va_start(args, line);
    vsnprintf(pbuf, BUFSZ, line, args);
    va_end(args);
    
    n0 = strlen(bp);
    
    toplines[0] = 0;  /* Clear for test */
    while (n0) {
        if (n0 >= CO) {
            n0 = CO - 2;  /* Simplified for test */
        }
        tl = eos(toplines);
        /* VULNERABLE: This is the exact problematic logic */
        if (tl - toplines + n0 + 2 < BUFSZ) {
            strncpy(tl, bp, n0);
            tl[n0] = 0;
            bp += n0;
            n0 = strlen(bp);
            if (n0 && tl[0]) {
                if (tl - toplines + strlen(tl) + 1 < BUFSZ)
                    strcat(tl, "\n");
            }
        } else {
            break;
        }
    }
}

int main(void) {
    printf("=== Testing hack.topl.c buffer overflow scenarios ===\n");
    
    /* Test 1: Normal message - should be safe */
    printf("\nTest 1: Normal message\n");
    test_pline("You hit the kobold.");
    printf("Result length: %zu\n", strlen(toplines));
    
    /* Test 2: Very long message that wraps multiple times */
    printf("\nTest 2: Long wrapping message\n");
    test_pline("This is an extremely long message that should wrap across multiple lines and potentially cause buffer overflow issues when the wrapping logic accumulates more content than the buffer can hold safely.");
    printf("Result length: %zu\n", strlen(toplines));
    
    /* Test 3: Message exactly at buffer boundary */
    printf("\nTest 3: Boundary condition\n");
    char long_msg[300];
    memset(long_msg, 'A', 250);
    long_msg[250] = '\0';
    test_pline("%s", long_msg);
    printf("Result length: %zu\n", strlen(toplines));
    
    /* Test 4: Multiple concatenated messages (simulating toplines persistence) */
    printf("\nTest 4: Concatenation overflow\n");
    strcpy(toplines, "Existing content that takes up significant space in the buffer already");
    test_pline("Additional message that might overflow when combined with existing content");
    printf("Final length: %zu\n", strlen(toplines));
    
    printf("\n=== Test complete ===\n");
    return 0;
}