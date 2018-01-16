/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"

#include "py/obj.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "lib/utils/pyexec.h"

int mp_hal_stdin_rx_chr(void) {
	uint8_t c;
	MP_THREAD_GIL_EXIT();
	uart_read_bytes(0, &c, 1, portMAX_DELAY);
	MP_THREAD_GIL_ENTER();
	return c;
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    fwrite(str, 1, len, stdout);
    fflush(stdout);
    mp_uos_dupterm_tx_strn(str, len);
}

// cooked is same as uncooked because the terminal does some postprocessing
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    mp_hal_stdout_tx_strn(str, len);
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

uint32_t mp_hal_ticks_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

uint32_t mp_hal_ticks_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

void mp_hal_delay_ms(uint32_t ms) {
    struct timeval tv_start;
    struct timeval tv_end;
    uint64_t dt;
    gettimeofday(&tv_start, NULL);
    for (;;) {
        gettimeofday(&tv_end, NULL);
        dt = (tv_end.tv_sec - tv_start.tv_sec) * 1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000;
        if (dt + portTICK_PERIOD_MS >= ms) {
            // doing a vTaskDelay would take us beyound requested delay time
            break;
        }
        MICROPY_EVENT_POLL_HOOK
	    MP_THREAD_GIL_EXIT();
        vTaskDelay(1);
        MP_THREAD_GIL_ENTER();
    }
    if (dt < ms) {
        // do the remaining delay accurately
        MP_THREAD_GIL_EXIT();
        ets_delay_us((ms - dt) * 1000);
        MP_THREAD_GIL_ENTER();
    }
}

void mp_hal_delay_us(uint32_t us) {
    ets_delay_us(us);
}

// this function could do with improvements (eg use ets_delay_us)
void mp_hal_delay_us_fast(uint32_t us) {
    uint32_t delay = ets_get_cpu_frequency() / 19;
    while (--us) {
        for (volatile uint32_t i = delay; i; --i) {
        }
    }
}

/*
extern int mp_stream_errno;
int *__errno() {
    return &mp_stream_errno;
}
*/
