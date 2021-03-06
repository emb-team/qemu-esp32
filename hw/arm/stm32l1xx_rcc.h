#include "hw/sysbus.h"
#include "hw/arm/stm32_clktree.h"
#include "stm32l1xx.h"
#include "stm32_rcc.h"

typedef struct Stm32lixxRcc {
    /* Inherited */
    union {
        Stm32Rcc inherited;
        struct {
            /* Inherited */
            SysBusDevice busdev;

            /* Properties */
            uint32_t osc_freq;
            uint32_t osc32_freq;

            /* Private */
            MemoryRegion iomem;
            qemu_irq irq;
        };
    };
    
    /* Peripheral clocks */
    Clk PERIPHCLK[STM32_PERIPH_COUNT], // MUST be first field after `inherited`, because Stm32Rcc's last field aliases this array
    HSICLK, 
    HSECLK,
    MSICLK,
    SYSCLK,
    IWDGCLK,
    RTCCLK,
    LSECLK,
    LSICLK,

    PLLM, /* Applies "M" division and "N" multiplication factors for PLL */
    PLLCLK,
    PLL48CLK,

    PLLI2SM, /* Applies "M" division and "N" multiplication factors for PLLI2S */
    PLLI2SCLK,
    
    HCLK, /* Output from AHB Prescaler */
    PCLK1, /* Output from APB1 Prescaler */
    PCLK2; /* Output from APB2 Prescaler */

    /* Register Values */
    uint32_t
    RCC_CIR,
    RCC_ICSCR,
    RCC_APB1ENR,
    RCC_APB2ENR,
    RCC_APB1LPENR,
    RCC_APB2LPENR;

    /* Register Field Values */
    uint32_t
    RCC_CFGR_PPRE1,
    RCC_CFGR_PPRE2,
    RCC_CFGR_HPRE,
    RCC_AHBLPENR,
    RCC_AHB2ENR,
    RCC_AHB3ENR,
    RCC_CFGR_SW;

    uint8_t
    RCC_PLLCFGR_PLLM,
    RCC_PLLCFGR_PLLP,
    RCC_PLLCFGR_PLLSRC;

    uint16_t
    RCC_PLLCFGR_PLLN;

    uint8_t
    RCC_PLLI2SCFGR_PLLR,
    RCC_PLLI2SCFGR_PLLQ;

    uint16_t
    RCC_PLLI2SCFGR_PLLN;

} Stm32lixxRcc;
