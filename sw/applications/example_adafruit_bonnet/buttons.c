#include <stdio.h>
#include <stdbool.h>




#include "pad_control.h"
#include "pad_control_regs.h"
#include "core_v_mini_mcu.h"
#include "buttons.h"
#include "csr.h"
#include "rv_plic.h"
#include "gpio.h"


//private defines:
// --- Private defines ---

#define GPIO_TB_IN_UP     10
#define GPIO_TB_IN_DOWN  11
#define GPIO_TB_IN_LEFT  12
#define GPIO_TB_IN_RIGHT 9
#define GPIO_TB_IN_B     13
#define GPIO_TB_IN_A     14
/*
#define GPIO_TB_IN_UP     11
#define GPIO_TB_IN_DOWN  12
#define GPIO_TB_IN_LEFT  13
#define GPIO_TB_IN_RIGHT 10
#define GPIO_TB_IN_B     14
#define GPIO_TB_IN_A     15
*/
// --- Private variables ---
static const uint32_t gpio_tb[6] = {
    GPIO_TB_IN_UP, GPIO_TB_IN_DOWN, GPIO_TB_IN_LEFT,
    GPIO_TB_IN_RIGHT, GPIO_TB_IN_B, GPIO_TB_IN_A
};



static bool button_prev_state[6] = {1, 1, 1, 1, 1, 1};

// --- Forward declarations ---
static void x_button_common_handler(int button_idx);

// --- Public functions ---

void button_up_handler(void) {
    x_button_common_handler(0);
    gpio_intr_clear_stat(GPIO_TB_IN_UP);
}

void button_down_handler(void) {
    x_button_common_handler(1);
    gpio_intr_clear_stat(GPIO_TB_IN_DOWN);
}

void button_left_handler(void) {
    x_button_common_handler(2);
    gpio_intr_clear_stat(GPIO_TB_IN_LEFT);
}

void button_right_handler(void) {
    x_button_common_handler(3);
    gpio_intr_clear_stat(GPIO_TB_IN_RIGHT);
}

void button_b_handler(void) {
    //printf("B\n");
    x_button_common_handler(4);
    gpio_intr_clear_stat(GPIO_TB_IN_B);
}

void button_a_handler(void) {
    printf("A\n");
    x_button_common_handler(5);
    gpio_intr_clear_stat(GPIO_TB_IN_A);
}

void buttonsInit(void)
{

    printf("Buttons init\n");
    

    pad_control_t pad_control;
    pad_control.base_addr = mmio_region_from_addr((uintptr_t)PAD_CONTROL_START_ADDRESS);

    if (plic_Init() != kPlicOk) {
        printf("PLIC init failed\n");
        return;
    }

    CSR_SET_BITS(CSR_REG_MSTATUS, 0x8);
    CSR_SET_BITS(CSR_REG_MIE, (1 << 11));

    for (int i = 0; i < 6; i++) {
        gpio_cfg_t cfg = {
            .pin = gpio_tb[i],
            .mode = GpioModeIn,
            .en_input_sampling = true,
            .en_intr = true,
            .intr_type = GpioIntrEdgeRisingFalling
        };

        printf("gpio_tb[i] : %i\n", gpio_tb[i]); 

        if (gpio_config(cfg) != GpioOk) {
            printf("GPIO config failed for pin %d\n", gpio_tb[i]);
        }

        if (plic_irq_set_priority(gpio_tb[i], 1) != 0)
        {
            printf("Set priority failed for pin %i\n",  gpio_tb[i]);
        }
        if (plic_irq_set_enabled(gpio_tb[i], kPlicToggleEnabled) != 0)
        {
            printf("Set enable failed for pin %i\n",  gpio_tb[i]); 
        }
    }

    if (gpio_assign_irq_handler(GPIO_TB_IN_UP,    &button_up_handler) != 0)
    {
        printf("Error gpio_assign_irq_handler for GPIO_TB_IN_UP\n"); 
    }
    if (gpio_assign_irq_handler(GPIO_TB_IN_DOWN,    &button_down_handler) != 0)
    {
        printf("Error gpio_assign_irq_handler for GPIO_TB_IN_DOWN\n"); 
    }
    if (gpio_assign_irq_handler(GPIO_TB_IN_LEFT,    &button_left_handler)!= 0)
    {
        printf("Error gpio_assign_irq_handler for GPIO_TB_IN_LEFT\n"); 
    }
    if (gpio_assign_irq_handler(GPIO_TB_IN_RIGHT,    &button_right_handler)!= 0)
    {
        printf("Error gpio_assign_irq_handler for GPIO_TB_IN_RIGHT\n"); 
    }
    if (gpio_assign_irq_handler(GPIO_TB_IN_B,    &button_b_handler)!= 0)
    {
        printf("Error gpio_assign_irq_handler for GPIO_TB_IN_B\n"); 
    }
    if (gpio_assign_irq_handler(GPIO_TB_IN_A,    &button_a_handler)!= 0)
    {
        printf("Error gpio_assign_irq_handler for GPIO_TB_IN_A\n"); 
    }
    
}

// --- Private helper ---
/*
static void x_button_common_handler(int idx)
{
    static event_t event;

    bool state;
    gpio_read(gpio_tb[idx], &state);
    printf("GPIO pin %d state: %d\n", gpio_tb[idx], state);


    if (state) {
        event.type = ev_keydown;
        printf("Button %d pressed\n", idx);
    } else {
        event.type = ev_keyup;
        printf("Button %d released\n", idx);
    }
    button_prev_state[idx] = state;


    event.data1 = button_map[idx];
    event.data2 = 0;
    event.data3 = 0;
    D_PostEvent(&event);
}
*/

static void x_button_common_handler(int idx)
{
    //static event_t event;
    static bool toggled_state[6] = {false};  // one for each button

    //toggled_state[idx] = !toggled_state[idx];

    //event.type = toggled_state[idx] ? ev_keydown : ev_keyup;
    //event.data1 = button_map[idx];
    //event.data2 = 0;
    //event.data3 = 0;

    //D_PostEvent(&event);

    printf("Button %d %s (toggle mode)\n", idx, toggled_state[idx] ? "pressed" : "released");
}

// --- Interrupt handlers ---



// --- Optional helpers ---

int X_ButtonStateRaw(int id)
{
    bool state;
    gpio_read(gpio_tb[id], &state);
    return !state;
}

int buttonState(int num)
{
    return !button_prev_state[num];
}

void readButtons(void)
{
    // Not needed anymore, handled by interrupts
}