 /* Author: Hulbert Zeng
 * Partner(s) Name (if applicable):  
 * Lab Section: 021
 * Assignment: Lab #13  Exercise #5
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: https://youtu.be/e-h5qmduurA
 */ 
#include <avr/io.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

#include "timer.h"
#include "scheduler.h"

void A2D_init() {
      ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
    // ADEN: Enables analog-to-digital conversion
    // ADSC: Starts analog-to-digital conversion
    // ADATE: Enables auto-triggering, allowing for constant
    //        analog to digital conversions.
}

void transmit_data(unsigned char data, unsigned char gnds) {
    int i;
    for (i = 0; i < 8 ; ++i) {
         // Sets SRCLR to 1 allowing data to be set
         // Also clears SRCLK in preparation of sending data
         PORTC = 0x08;
         PORTD = 0x08;
         // set SER = next bit of data to be sent.
         PORTC |= ((data >> i) & 0x01);
         PORTD |= ((gnds >> i) & 0x01);
         // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
         PORTC |= 0x02;
         PORTD |= 0x02;
    }
    // set RCLK = 1. Rising edge copies data from â€œShiftâ€ register to â€œStorageâ€ register
    PORTC |= 0x04;
    PORTD |= 0x04;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
    PORTD = 0x00;
}

// Pins on PORTA are used as input for A2D conversion
    //    The default channel is 0 (PA0)
    // The value of pinNum determines the pin on PORTA
    //    used for A2D conversion
    // Valid values range between 0 and 7, where the value
    //    represents the desired pin for A2D conversion
void Set_A2D_Pin(unsigned char pinNum) {
    ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
    // Allow channel to stabilize
    static unsigned char i = 0;
    for ( i=0; i<15; i++ ) { asm("nop"); } 
}


// shared task variables
unsigned short row = 0xDF;
unsigned short pattern = 0x10;
unsigned short LED_time = 0;

enum horizontal { rest, right, left };
// task variables

int horizontal(int state) {
    Set_A2D_Pin(0);

    TimerSet(1);
    while(!TimerFlag);
    TimerFlag = 0;

    unsigned short x = ADC;
    switch(state) {
        case rest:  
            if(x < 500) {
                if(x > 450) {
                    LED_time = 19;
                } else if(x > 350) {
                    LED_time = 9;
                } else if(x > 250) {
                    LED_time = 4;
                } else {
                    LED_time = 1;
                }
                state = right;
            }
            if(x > 550) {
                if(x < 600) {
                    LED_time = 19;
                } else if(x < 700) {
                    LED_time = 9;
                } else if(x < 800) {
                    LED_time = 4;
                } else {
                    LED_time = 1;
                }
                state = left;
            }
            break;
        case right:
            if(LED_time == 0) {
                if(pattern > 0x01) {
                    pattern = pattern >> 1;
                }
                state = rest;
            } else {
                --LED_time;
                state = right;
            }
            break;
        case left:
            if(LED_time == 0) {
                if(pattern < 0x80) {
                    pattern = pattern << 1;
                }
                state = rest;
            } else {
                --LED_time;
                state = left;
            }
            break;
        default: state = rest; break;
    }

    return state;
}


enum vertical { wait, up, down };
// task variables

int vertical(int state) {
    Set_A2D_Pin(1);

    TimerSet(1);
    while(!TimerFlag);
    TimerFlag = 0;

    unsigned short x = ADC;
    switch(state) {
        static signed char y = 0;
        case wait: 
            if(x < 500) {
                if(x > 450) {
                    LED_time = 19;
                } else if(x > 350) {
                    LED_time = 9;
                } else if(x > 250) {
                    LED_time = 4;
                } else {
                    LED_time = 1;
                }
                state = up;
            }
            if(x > 600) {
                if(x < 650) {
                    LED_time = 19;
                } else if(x < 750) {
                    LED_time = 9;
                } else if(x < 850) {
                    LED_time = 4;
                } else {
                    LED_time = 1;
                }
                state = down;
            }
            break;
        case up:
            if(LED_time == 0) {
                if(y < 2) {
                    row = (row >> 1) | 0x80;
                    ++y;
                 }
                 state = wait;
            } else {
                --LED_time;
                state = up;
            }
            break;
        case down:
            if(LED_time == 0) {
                if(y > -2) {
                    row = (row << 1) | 0x01;
                    --y;
                }
                state = wait;
            } else {
                --LED_time;
                state = down;
            }
            break;
        default: state = wait; break;
    }

    return state;
}


enum display { show };
// task variables

int display(int state) {
    switch(state) {
        case show: transmit_data(pattern, row); break;
        default: state = show; break;
    }
    return state;
}

int main(void) {
    /* Insert DDR and PORT initializations */
    DDRA = 0x00; PORTA = 0xFF;
    DDRC = 0xFF; PORTC = 0x00;
    DDRD = 0xFF; PORTD = 0x00;
    /* Insert your solution below */
    static task task1, task2, task3;
    task *tasks[] = { &task1, &task2, &task3 };
    const unsigned short numTasks = sizeof(tasks)/sizeof(*tasks);
    const char start = -1;
    // LED horizontal control
    task1.state = start;
    task1.period = 50;
    task1.elapsedTime = task1.period;
    task1.TickFct = &horizontal;

    // LED vertical control
    task2.state = start;
    task2.period = 50;
    task2.elapsedTime = task2.period;
    task2.TickFct = &vertical;

    // LED display
    task3.state = start;
    task3.period = 25;
    task3.elapsedTime = task3.period;
    task3.TickFct = &display;

    unsigned short i;

    unsigned long GCD = tasks[0]->period;
    for(i = 0; i < numTasks; i++) {
        GCD = findGCD(GCD, tasks[i]->period);
    }

    A2D_init();

    TimerSet(GCD);
    TimerOn();
    while (1) {
        for(i = 0; i < numTasks; i++) {
            if(tasks[i]->elapsedTime == tasks[i]->period) {
                tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
                tasks[i]->elapsedTime = 0;
            }
            tasks[i]->elapsedTime += GCD;

            TimerSet(GCD);
        }
        while(!TimerFlag);
        TimerFlag = 0;
    }
    return 1;
}
