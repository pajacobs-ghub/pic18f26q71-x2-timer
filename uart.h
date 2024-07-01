// uart.h
// PJ, 2023-12-01, 2024-07-01 simplify again for x2-timer.

#ifndef MY_UART
#define MY_UART
void uart1_init(long baud);
void uart1_putch(char data);
void uart1_flush_rx(void);
char uart1_getch(void);
void uart1_close(void);

void putch(char data);
int getch(void);
int getche(void);

int getstr(char* buf, int nbuf);
void putstr(char* str);

#define XON 0x11
#define XOFF 0x13

#endif
