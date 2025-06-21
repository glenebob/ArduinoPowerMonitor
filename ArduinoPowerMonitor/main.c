#include "Types.h"
#include "Abort.h"
#include "Interrupts.h"
#include "Task.h"
#include "SoftwareTimer.h"
#include "AsyncIo.h"
#include "Led.h"
#include "PowerOut.h"
#include "PowerMonitor.h"

static void current_draw_change(bool current_draw_detected);

int main()
{
    led_init();
    power_out_init();
    interrupts_init();
    task_queue_init();
    timers_init();
    io_task_init();
    power_monitor_init();
    power_monitor_begin(current_draw_change);

    task_queue_run();

    // We should never get here.
    fatal(1);
    
    // No return.
}

static void current_draw_change(bool current_draw_detected)
{
    if (current_draw_detected)
    {
        power_out_on();
    }
    else
    {
        power_out_off();
    }
}
