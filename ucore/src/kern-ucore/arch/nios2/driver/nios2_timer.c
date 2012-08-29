#include <types.h>
#include <nios2_timer.h>
#include <nios2.h>
#include <system.h>
#include <alt_types.h>
#include <altera_avalon_timer_regs.h>
#include <clock.h>
#include <stdio.h>
#include <nios2_irq.h>

#define TimerTimeoutL ((uint16_t*)(0xE0000000 + TIMER_BASE + 8))
#define TimerTimeoutH ((uint16_t*)(0xE0000000 + TIMER_BASE + 12))
#define TimerSnapshotL ((uint16_t*)(0xE0000000 + TIMER_BASE + 16))
#define TimerSnapshotH ((uint16_t*)(0xE0000000 + TIMER_BASE + 20))

static void
alt_avalon_timer_sc_init (void* base, alt_u32 irq_controller_id, alt_u32 irq)
{  
  /* set to free running mode */
    
    IOWR_ALTERA_AVALON_TIMER_CONTROL (base, 
         ALTERA_AVALON_TIMER_CONTROL_ITO_MSK  |
         ALTERA_AVALON_TIMER_CONTROL_CONT_MSK |
         ALTERA_AVALON_TIMER_CONTROL_START_MSK);
    
    alt_irq_enable(irq);
}



/* timer_init() */
void
nios2_timer_init(void) {
    //*(TimerTimeoutL)=0xffff;
    //*(TimerTimeoutH)=0xffff;
    alt_avalon_timer_sc_init((void*) (TIMER_BASE + 0xE0000000),
                               0,
                               TIMER_IRQ
    );
    kprintf("++ setup timer interrupts\n");
}

static int
snapshot() {
    *(TimerSnapshotL) = 0; //write to timer to get snapshot
    volatile int numclow =  *(TimerSnapshotL); //get low part
    volatile int numchigh = *(TimerSnapshotH ); //get high part
    return numclow +(numchigh << 16); //assemble full number
}

void
nios2_timer_usleep(int usec) {
    int tks = usec / 10000 + 1;//T=10ms
    if (tks < 2) tks = 2;
    int cur = snapshot(), next;
    do {
        next = snapshot();
        if (cur != next) {
            //++cnt;
            if (next > cur)
                --tks;
            cur = next;
            //if (tks % 10 == 0)
            //    kprintf("tks=%d next=%d\n", tks, next);
        }
    } while (tks);
}

