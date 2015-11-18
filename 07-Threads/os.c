#include <stddef.h>
#include <stdint.h>
#include "reg.h"
#include "threads.h"
#include "malloc.h"
#include "str.h"

/* USART TXE Flag
 * This flag is cleared when data is written to USARTx_DR and
 * set when that data is transferred to the TDR
 */
#define USART_FLAG_TXE	((uint16_t) 0x0080)
#define USART_FLAG_RXE	((uint16_t) 0x0020)

extern int fibonacci(int x);

void usart_init(void)
{
	*(RCC_APB2ENR) |= (uint32_t) (0x00000001 | 0x00000004);
	*(RCC_APB1ENR) |= (uint32_t) (0x00020000);

	/* USART2 Configuration, Rx->PA3, Tx->PA2 */
	*(GPIOA_CRL) = 0x00004B00;
	*(GPIOA_CRH) = 0x44444444;
	*(GPIOA_ODR) = 0x00000000;
	*(GPIOA_BSRR) = 0x00000000;
	*(GPIOA_BRR) = 0x00000000;

	*(USART2_CR1) = 0x0000000C;
	*(USART2_CR2) = 0x00000000;
	*(USART2_CR3) = 0x00000000;
	*(USART2_CR1) |= 0x2000;
}

void print_str(const char *str)
{
	while (*str) {
		while (!(*(USART2_SR) & USART_FLAG_TXE));
		*(USART2_DR) = (*str & 0xFF);
		str++;
	}
}

void print_char(const char *ch)
{
        if(*ch){
	    while (!(*(USART2_SR) & USART_FLAG_TXE));
	    *(USART2_DR) = (*ch & 0xFF);
	   
	}

}
static void delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

static void busy_loop(void *str)
{
	while (1) {
		print_str(str);
		print_str(": Running...\n");
		delay(1000);
	}
}

void test1(void *userdata)
{
	busy_loop(userdata);
}

void test2(void *userdata)
{
	busy_loop(userdata);
}

void test3(void *userdata)
{
	busy_loop(userdata);
}
void fibonacci_task(int num)
{
	char *str = malloc(20);
	int result = fibonacci(num);
	itoa(result,str);
	print_str(str);
	print_char("\n");
	free(str);
	thread_self_terminal();
}

void shell(void *userdata)
{
	int i = 0;
	char str_buff[20];
	while(1){
		print_str("vodkaice@~$:");
		i = 0;
		while(1){
		    // USART receive char 
		    if(*(USART2_SR) & USART_FLAG_RXE){	    
			str_buff[i] = ( *(USART2_DR) & 0xFF);
			// end of input (detect \r or \n)
			if(str_buff[i] == '\r' || str_buff[i] == '\n'){	   
			    print_char("\n");
			    str_buff[i] = '\0';
			    print_str(str_buff);
			    if(strcmp(str_buff,"fib")==1){
				if(thread_create((void *)(fibonacci_task),(void *)(5)) == -1)
				    print_str("Fibonacci failed!\r\n");
				else 
				    print_str("Fibonacci successful!\r\n");
			    break;
			    }
			    else
				print_str("Not Fibonacci\r\n");
			    
			}
			// detect backspace or delete 
			else if(str_buff[i] == 8 || str_buff[i] == 127){
			    if(i != 0){
				print_str("\b \b");
				i--;
			    }
			}
			// print input on the shell
			else{	   
			    if(i < 20){
				print_char(&str_buff[i++]);
			    }
			    else{
				print_str("\nstring buff overflow!(please key in again)\n");
				print_str("vodkaice@~$:");
				i=0;
			    }
			}
		    }
		}
	}	
    
}

/* 72MHz */
#define CPU_CLOCK_HZ 72000000

/* 100 ms per tick. */
#define TICK_RATE_HZ 10

int main(void)
{
	//const char *str1 = "Task1", *str2 = "Task2", *str3 = "Task3";
	const char *username = "vodkaice";
	usart_init();

	if (thread_create(shell, (void *) username) == -1)
		print_str("Shell creation failed\r\n");

	/* SysTick configuration */
	*SYSTICK_LOAD = (CPU_CLOCK_HZ / TICK_RATE_HZ) - 1UL;
	*SYSTICK_VAL = 0;
	*SYSTICK_CTRL = 0x07;

	thread_start();

	return 0;
}
