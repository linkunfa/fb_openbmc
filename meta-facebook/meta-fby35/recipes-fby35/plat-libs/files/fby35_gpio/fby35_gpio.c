/* Copyright 2020-present Facebook. All Rights Reserved.
 *
 * This file contains code to support IPMI2.0 Specificaton available @
 * http://www.intel.com/content/www/us/en/servers/ipmi/ipmi-specifications.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <syslog.h>
#include <facebook/fby35_common.h>
#include <facebook/bic_ipmi.h>
#include <facebook/bic_xfer.h>
#include "fby35_gpio.h"


//Crater lake BIC GPIO
const char *cl_gpio_pin_name[] = {
  "FM_BMC_PCH_SCI_LPC_R_N",         //0
  "FM_BIOS_POST_CMPLT_BMC_N",
  "FM_SLPS3_PLD_N",
  "IRQ_BMC_PCH_SMI_LPC_R_N",
  "IRQ_UV_DETECT_N",
  "FM_UV_ADR_TRIGGER_EN_R",
  "IRQ_SMI_ACTIVE_BMC_N",
  "HSC_SET_EN_R",
  "FM_BIC_RST_RTCRST_R",
  "RST_USB_HUB_N_R",
  "A_P3V_BAT_SCALED_EN_R",          //10
  "FM_SPI_PCH_MASTER_SEL_R",
  "FM_PCHHOT_N",
  "FM_SLPS4_PLD_N",
  "FM_S3M_CPU0_CD_INIT_ERROR",
  "PWRGD_SYS_PWROK",
  "FM_HSC_TIMER",
  "IRQ_SMB_IO_LVC3_STBY_ALRT_N",
  "IRQ_CPU0_VRHOT_N",
  "DBP_CPU_PREQ_BIC_N",
  "FM_CPU_THERMTRIP_LATCH_LVT3_N",  //20
  "FM_CPU_SKTOCC_LVT3_PLD_N",
  "H_CPU_MEMHOT_OUT_LVC3_N",
  "RST_PLTRST_PLD_N",
  "PWRBTN_N",
  "RST_BMC_R_N",
  "H_BMC_PRDY_BUF_N",
  "BMC_READY",
  "BIC_READY",
  "FM_RMCA_LVT3_N",
  "HSC_MUX_SWITCH_R",               //30
  "FM_FORCE_ADR_N_R",
  "PWRGD_CPU_LVC3",
  "FM_PCH_BMC_THERMTRIP_N",
  "FM_THROTTLE_R_N",
  "IRQ_HSC_ALERT2_N",
  "SMB_SENSOR_LVC3_ALERT_N",
  "FM_CATERR_LVT3_N",
  "SYS_PWRBTN_N",
  "RST_PLTRST_BUF_N",
  "IRQ_BMC_PCH_NMI_R",              //40
  "IRQ_SML1_PMBUS_ALERT_N",
  "IRQ_PCH_CPU_NMI_EVENT_N",
  "FM_BMC_DEBUG_ENABLE_N",
  "FM_DBP_PRESENT_N",
  "FM_FAST_PROCHOT_EN_N_R",
  "FM_SPI_MUX_OE_CTL_PLD_N",
  "FBRK_N_R",
  "FM_PEHPCPU_INT",
  "FM_BIOS_MRC_DEBUG_MSG_DIS_R",
  "FAST_PROCHOT_N",                 //50
  "FM_JTAG_TCK_MUX_SEL_R",
  "BMC_JTAG_SEL_R",
  "H_CPU_ERR0_LVC3_N",
  "H_CPU_ERR1_LVC3_N",
  "H_CPU_ERR2_LVC3_N",
  "RST_RSMRST_BMC_N",
  "FM_MP_PS_FAIL_N",
  "H_CPU_MEMTRIP_LVC3_N",
  "FM_CPU_BIC_PROCHOT_LVT3_N",
  "BOARD_ID2",                      //60
  "IRQ_PVCCD_CPU0_VRHOT_LVC3_N",
  "FM_PVCCIN_CPU0_PWR_IN_ALERT_N",
  "BOARD_ID0",
  "BOARD_ID1",
  "BOARD_ID3",
  "FM_THROTTLE_IN_N",
  "AUTH_COMPLETE",
  "AUTH_PRSNT_N",
  "SGPIO_BMC_CLK_R",
  "SGPIO_BMC_LD_R_N",               //70
  "SGPIO_BMC_DOUT_R",
  "SGPIO_BMC_DIN",
};

//Halfdome BIC GPIO
const char *hd_gpio_pin_name[] = {
  "FM_BIOS_POST_CMPLT_BIC_N",    //0
  "FM_CPU_BIC_SLP_S3_N",
  "APML_CPU_ALERT_BIC_N",
  "IRQ_UV_DETECT_N",
  "PVDDCR_CPU0_BIC_OCP_N",
  "HSC_OCP_GPIO1_R",
  "PVDDCR_CPU1_BIC_OCP_N",
  "RST_USB_HUB_R_N",
  "P3V_BAT_SCALED_EN_R",
  "HDT_BIC_TRST_R_N",
  "FM_CPU_BIC_SLP_S5_N",      //10
  "PVDD11_S3_BIC_OCP_N",
  "FM_HSC_TIMER",
  "IRQ_SMB_IO_LVC3_STBY_ALRT_N",
  "PVDDCR_CPU1_PMALERT_N",
  "FM_CPU_BIC_THERMTRIP_N",
  "FM_PRSNT_CPU_BIC_N",
  "AUTH_PRSNT_BIC_N",
  "RST_CPU_RESET_BIC_N",
  "PWRBTN_R1_N",
  "RST_BMC_R_N",                    //20
  "HDT_BIC_DBREQ_R_N",
  "BMC_READY",
  "BIC_READY",
  "FM_SOL_UART_CH_SEL_R",
  "PWRGD_CPU_LVC3",
  "CPU_ERROR_BIC_LVC3_R_N",
  "PVDD11_S3_PMALERT_N",
  "IRQ_HSC_ALERT1_N",
  "SMB_SENSOR_LVC3_ALERT_N",
  "SYS_PWRBTN_BIC_N",        //30
  "RST_PLTRST_BIC_N",
  "CPU_SMERR_BIC_N",
  "IRQ_HSC_ALERT2_N",
  "BIC_CPU_NMI_R_N",
  "FM_BMC_DEBUG_ENABLE_N",
  "FM_DBP_PRESENT_N",
  "FM_FAST_PROCHOT_EN_R_N",
  "FM_BIOS_ABL_DEBUG_MSG_DIS_N",
  "FM_BIOS_MRC_DEBUG_MSG_DIS",
  "FAST_PROCHOT_N",
  "BIC_CPU_RSVD_N",
  "BIC_JTAG_SEL_R",
  "HSC_OCP_GPIO2_R",        //40
  "HSC_OCP_GPIO3_R",
  "RST_RSMRST_BMC_N",
  "FM_CPU_BIC_PROCHOT_LVT3_N",
  "BIC_JTAG_MUX_SEL",
  "BOARD_ID2",
  "BOARD_ID0",
  "BOARD_ID1",
  "BOARD_ID3",
  "BOARD_ID5",                    //50
  "BOARD_ID4",
  "HSC_TYPE_0",
  "HSC_TYPE_1",
};

//Great Lakes BIC GPIO
const char *gl_gpio_pin_name[] = {
  "HSC_TYPE",                        //0
  "H_CPU_MON_FAIL_LVC3_N",
  "FM_SLPS3_LVC3_N",
  "IRQ_BMC_NMI_R_N",
  "IRQ_UV_DETECT_N",
  "FM_UV_ADR_TRIGGER_EN_R",
  "HSC_SET_EN_R",
  "RST_USB_HUB_N_R",
  "A_P3V_BAT_SCALED_EN_R",
  "DAM_BIC_R_EN",
  "FM_SLPS4_PLD_N",                  //10
  "FM_CPU0_CD_INIT_ERROR",
  "FM_HSC_TIMER",
  "IRQ_SMB_IO_LVC3_STBY_ALRT_N",
  "IRQ_PVCCD_CPU0_VRHOT_LVC3_N",
  "DBP_CPU_PREQ_BIC_N",
  "FM_CPU_THERMTRIP_LATCH_LVT3_N",
  "FM_CPU_SKTOCC_LVT3_PLD_N",
  "H_CPU_MEMHOT_OUT_LVC3_N",
  "RST_PLTRST_SYNC_LVC3_N",
  "PWRBTN_N",                        //20
  "RST_BMC_R_N",
  "H_BMC_PRDY_BUF_N",
  "BMC_READY",
  "BIC_READY",
  "FM_RMCA_LVT3_N",
  "PWRGD_AUX_PWRGD_BMC_LVC3",
  "FM_FORCE_ADR_N_R",
  "PWRGD_CPU_LVC3",
  "IRQ_HSC_ALERT2_N",
  "SMB_SENSOR_LVC3_ALERT_N",         //30
  "FM_CATERR_LVT3_N",
  "SYS_PWRBTN_LVC3_N",
  "RST_PLTRST_BUF_N",
  "IRQ_SML1_PMBUS_ALERT_N",
  "FM_BMC_DEBUG_ENABLE_R_N",
  "FM_DBP_PRESENT_N",
  "FM_FAST_PROCHOT_EN_R_N",
  "FBRK_R_N",
  "FM_PEHPCPU_INT",
  "FAST_PROCHOT_N",                  //40
  "BMC_JTAG_SEL_R",
  "H_CPU_ERR0_LVC3_R_N",
  "H_CPU_ERR1_LVC3_R_N",
  "H_CPU_ERR2_LVC3_R_N",
  "FM_MP_PS_FAIL_N",
  "H_CPU_MEMTRIP_LVC3_N",
  "FM_CPU_BIC_PROCHOT_LVT3_N",
  "AUTH_COMPLETE",
  "BOARD_ID2",
  "IRQ_PVCCIN_CPU0_VRHOT_N",         //50
  "IRQ_PVCCINF_CPU0_VRHOT_N",
  "BOARD_ID0",
  "BOARD_ID1",
  "BOARD_ID3",
  "AUTH_PRSNT_N",
};

//Great Lakes BIC virtual GPIO
const char *gl_virtual_gpio_pin_name[] = {
  "BIOS_POST_COMPLETE",    //0
  "",                      //1
  "ADR_MODE0",             //2
};

//Vernal Falls BIC GPIO
const char *vf_gpio_pin_name[] = {
  "IRQ_P12V_E1S_3_FLT_N",       //0
  "IRQ_P12V_E1S_2_FLT_N",
  "IRQ_P12V_E1S_1_FLT_N",
  "IRQ_P12V_E1S_0_FLT_N",
  "IRQ_P3V3_E1S_3_FLT_N",
  "IRQ_P3V3_E1S_2_FLT_N",
  "IRQ_P3V3_E1S_1_FLT_N",
  "IRQ_P3V3_E1S_0_FLT_N",
  "PWRGD_P12V_AUX",
  "IRQ_P12V_HSC_ALERT1_N",
  "IRQ_P12V_HSC_ALERT2_N",      //10
  "PU_DB800_HI_BW",
  "LED_PWRGD_P12V_E1S_ALL",
  "FM_AUX_PWR_EN",
  "RST_SMB_E1S_3_N",
  "RST_SMB_E1S_2_N",
  "BOARD_ID0",
  "BOARD_ID1",
  "BOARD_ID2",
  "BOARD_ID3",
  "RST_BIC_E1S_3_N",            //20
  "RST_BIC_E1S_2_N",
  "RST_BIC_E1S_1_N",
  "RST_BIC_E1S_0_N",
  "FM_MB_SLOT_ID0",
  "FM_PWRDIS_E1S_3",
  "FM_PWRDIS_E1S_2",
  "FM_PWRDIS_E1S_1",
  "FM_PWRDIS_E1S_0",
  "FM_P12V_E1S_0_EN",
  "IRQ_TMP75_ALERT_N",          //30
  "FM_P3V3_E1S_0_SW_EN",
  "FM_POWER_EN",
  "PWRGD_EXP_PWROK",
  "RST_MB_N",
  "FM_FRU_WC_N",
  "P1V2_VDD_PG_R",
  "RST_SMB_E1S_1_N",
  "RST_SMB_E1S_0_N",
  "IRQ_SMB_ALERT_N",
  "FM_P3V3_E1S_EN",             //40
  "FM_P12V_EDGE_EN",
  "IRQ_P3V3_EDGE_FLT_N",
  "IRQ_P12V_EDGE_FLT_N",
  "LED_BIC_E1S_3",
  "LED_BIC_E1S_2",
  "LED_BIC_E1S_1",
  "LED_BIC_E1S_0",
  "FM_MB_SLOT_ID1",
  "FM_PRSNT_E1S_3_N",
  "FM_PRSNT_E1S_2_N",           //50
  "FM_PRSNT_E1S_1_N",
  "FM_PRSNT_E1S_0_N",
  "FM_BOARD_REV_ID2",
  "FM_BOARD_REV_ID1",
  "FM_BOARD_REV_ID0",
  "HSC_SEL_ID2",
  "HSC_SEL_ID1",
  "HSC_SEL_ID0",
  "FM_P12V_E1S_2_EN",
  "FM_P12V_E1S_1_EN",           //60
  "IRQ_INA230_E1S_0_ALERT_N",
  "FM_CLKBUF_EN",
  "FM_P12V_E1S_3_EN",
  "IRQ_INA230_E1S_3_ALERT_N",
  "IRQ_INA230_E1S_2_ALERT_N",
  "IRQ_INA230_E1S_1_ALERT_N",
  "CLKBUF_E1S_3_OE_N",
  "CLKBUF_E1S_2_OE_N",
  "CLKBUF_E1S_1_OE_N",
  "CLKBUF_E1S_0_OE_N",          //70
  "FM_P3V3_E1S_3_SW_EN",
  "FM_P3V3_E1S_2_SW_EN",
  "FM_P3V3_E1S_1_SW_EN",
};

//Rainbow Falls BIC GPIO
const char *rf_gpio_pin_name[] = {
  "ASIC_DEV_RST_N",             //0
  "ASIC_PERST0_N",
  "ASIC_PERST1_N",
  "ASIC_FAIL_N",
  "ASIC_EVENT_N",
  "ASIC_DUALPORTEN_N",
  "JTAG2_BIC_ASIC_NTRST2",
  "ASIC_TAP_SEL",
  "ASIC_CPU_BOOT_0",
  "ASIC_CPU_BOOT_1",
  "ASIC_M_SCAN_PCAP_SEL",       //10
  "ASIC_GPIO_R_0",
  "ASIC_GPIO_R_1",
  "AUX_PWR_EN_4C",
  "I2CS_SRSTB_GPIO",
  "FM_ISOLATED_EN_N",
  "FM_P0V8_ASICD_EN",
  "P1V8_ASIC_EN_R",
  "FM_P0V8_ASICA_EN",
  "PVTT_AB_EN_R",
  "PVTT_CD_EN_R",               //20
  "FM_P0V9_ASICA_EN",
  "PVPP_CD_EN_R",
  "FM_PVDDQ_AB_EN",
  "PVPP_AB_EN_R",
  "FM_PVDDQ_CD_EN",
  "SLOT_ID0",
  "PVPP_CD_PG_R",
  "P0V8_ASICA_PWRGD",
  "PVTT_AB_PG_R",
  "SMB_SENSOR_LVC3_ALERT_N",    //30
  "PVTT_CD_PG_R",
  "FM_POWER_EN",
  "PWRGD_CARD_PWROK",
  "RST_MB_N",
  "SPI_MASTER_SEL",
  "FM_SPI_MUX_OE_CTL_N",
  "SMB_12V_INA_ALRT_N",
  "SMB_3V3_INA_ALRT_N",
  "FM_MEM_THERM_EVENT_LVT3_N",
  "SPI_RST_FLASH_N",            //40
  "SMBUS_ALERT_R_N",
  "LSFT_SMB_DIMM_EN",
  "P0V9_ASICA_PWRGD",
  "P1V8_ASIC_PG_R",
  "JTAG2_ASIC_PORT_SEL_EN_R",
  "SAVE_N_BIC",
  "FM_ADR_COMPLETE_DLY",
  "P5V_STBY_PG",
  "PVPP_AB_PG_R",
  "P1V2_STBY_PG_R",             //50
  "SLOT_ID1",
  "SMB_VR_PVDDQ_CD_ALERT_N",
  "P0V8_ASICD_PWRGD",
  "PWRGD_PVDDQ_CD",
  "SMB_VR_PASICA_ALERT_N",
  "JTAG2_BIC_SHIFT_EN",
  "SMB_VR_PVDDQ_AB_ALERT_N",
  "SPI_BIC_SHIFT_EN",
  "PWRGD_PVDDQ_AB",
  "P0V9_ASICA_FT_R",            //60
  "PVDDQ_AB_FT_R",
  "PVDDQ_CD_FT_R",
  "FM_PWRBRK_PRIMARY_R_N",
  "P0V8_ASICD_FT_R",
  "P0V8_ASICA_FT_R",
  "LED_CXL_POWER",
  "FM_BOARD_REV_ID2",
  "FM_BOARD_REV_ID1",
  "FM_BOARD_REV_ID0",
  "BOARD_ID0",                  //70
  "BOARD_ID1",
  "BIC_SECUREBOOT",
  "BOARD_ID2",
  "BOARD_ID3",
  "BIC_ESPI_SELECT",
  "LED_CXL_FAULT",
  "CLK_100M_OSC_EN",
};

//Java Island BIC GPIO
const char *ji_gpio_pin_name[] = {
  "INA230_E1S_ALERT_L",             //0
  "I2C_2_CPU_ALERT_R_L",
  "FPGA_WATCH_DOG_TIMER0_L",
  "FPGA_WATCH_DOG_TIMER1_L",
  "HSC_OCP_GPIO1",
  "FPGA_WATCH_DOG_TIMER2_L",
  "RST_USB_HUB_R_L",
  "P3V_BAT_SCALED_EN_R",
  "WP_HW_EXT_CTRL_L",
  "BIC_EROT_LVSFT_EN",
  "RTC_CLR_L",                      //10
  "I2C_SSIF_ALERT_L",
  "FPGA_CPU_BOOT_DONE",
  "FM_HSC_TIMER",
  "IRQ_I2C_IO_LVC_STBY_ALRT_L",
  "BIC_TMP_LVSFT_EN",
  "BIC_I2C_0_FPGA_ALERT_L",
  "BIC_I2C_1_FPGA_ALERT_L",
  "JTAG_TRST_CPU_BMC_L",
  "RST_I2C_E1S_L",
  "FM_PWRDIS_E1S",                  //20
  "PWRBTN_L",
  "RST_BMC_L",
  "BMC_READY",
  "BIC_READY",
  "PWR_BRAKE_CPU1",
  "RUN_POWER_PG",
  "BIC_RETIMER_MUX_SEL",
  "SPI_BMC_FPGA_INT_L",
  "IRQ_HSC_ALERT1_L",
  "I2C_SENSOR_LVC_ALERT_L",         //30
  "INA_CRIT_ALERT1_L",
  "RUN_POWER_EN",
  "SPI_HOST_TPM_RST_L",
  "HSC_TYPE_0",
  "THERM_WARN_CPU1_L_3V3",
  "RUN_POWER_FAULT_L",
  "SENSOR_AIR0_THERM_L",
  "SENSOR_AIR1_THERM_L",
  "FM_FAST_PROCHOT_EN",
  "THERM_BB_OVERT_L",               //40
  "THERM_BB_WARN_L",
  "BIC_CPU_JTAG_MUX_SEL",
  "FM_VR_FW_PROGRAM_L",
  "FAST_PROCHOT_L",
  "CPU_EROT_FATAL_ERROR_L",
  "BIC_REMOTEJTAG_EN",
  "THERM_OVERT_CPU1_L_3V3",
  "HSC_OCP_GPIO2",
  "HSC_OCP_GPIO3",
  "SENSOR_AIR0_ALERT_L",            //50
  "SENSOR_AIR1_ALERT_L",
  "BIC_CPLD_VRD_MUX_SEL",
  "CPU_BIC_PROCHOT_L",
  "CPLD_JTAG_MUX_SEL",
  "JTAG_FPGA_MUX_SEL",
  "BOARD_ID0",
  "BOARD_ID1",
  "BOARD_ID2",
  "BOARD_ID3",
  "BOARD_ID4",                      //60
  "BOARD_ID5",
  "FM_BIOS_MRC_DEBUG_DIS_L",
  "E1S_PRSNT_L",
  "RETIMER_PG",
  "SATMC_READY",
};

/* GPIO Expander gpio table */
gpio_cfg gpio_expander_gpio_table[] = {
  /* shadow_name, pin_name, direction, value */
  // LAST
  {NULL, NULL, GPIO_DIRECTION_INVALID, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2}
};

/* BMC gpio table */
gpio_cfg bmc_gpio_table[] = {
  /* shadow_name, pin_name, direction, value */

  [GPIO_PWRGD_NIC_BMC] =  {"PWRGD_NIC_BMC", "GPIOB0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_OCP_DEBUG_BMC_PRSNT_N] = {"OCP_DEBUG_BMC_PRSNT_N", "GPIOB1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PWROK_STBY_BMC_SLOT1_R] = {"PWROK_STBY_BMC_SLOT1_R", "GPIOB2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PWROK_STBY_BMC_SLOT2] = {"PWROK_STBY_BMC_SLOT2", "GPIOB3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PWROK_STBY_BMC_SLOT3_R] = {"PWROK_STBY_BMC_SLOT3_R", "GPIOB4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PWROK_STBY_BMC_SLOT4] = {"PWROK_STBY_BMC_SLOT4", "GPIOB5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  [GPIO_OCP_NIC_PRSNT_BMC_N] = {"OCP_NIC_PRSNT_BMC_N", "GPIOC0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_FM_NIC_WAKE_BMC_N] = {"FM_NIC_WAKE_BMC_N", "GPIOC1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_FM_PWRBRK_PRIMARY_R] = {"FM_PWRBRK_PRIMARY_R", "GPIOC2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_SMB_RST_PRIMARY_BMC_N_R] = {"SMB_RST_PRIMARY_BMC_N_R", "GPIOC3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_SMB_RST_SECONDARY_BMC_N_R] = {"SMB_RST_SECONDARY_BMC_N_R", "GPIOC4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_USB_MUX_CB_R] = {"USB_MUX_CB_R", "GPIOC5", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},

  [GPIO_FM_RESBTN_SLOT1_BMC_N] = {"FM_RESBTN_SLOT1_BMC_N", "GPIOF0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_FM_RESBTN_SLOT2_N] = {"FM_RESBTN_SLOT2_N", "GPIOF1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_FM_RESBTN_SLOT3_BMC_N] = {"FM_RESBTN_SLOT3_BMC_N", "GPIOF2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_FM_RESBTN_SLOT4_N] = {"FM_RESBTN_SLOT4_N", "GPIOF3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SMB_TEMP_ALERT_BMC_N_R] = {"SMB_TEMP_ALERT_BMC_N_R", "GPIOF4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_FM_DEBUG_UART_MUX_BMC_R] = {"FM_DEBUG_UART_MUX_BMC_R", "GPIOF5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SMB_HOTSWAP_BMC_ALERT_N_R] = {"SMB_HOTSWAP_BMC_ALERT_N_R", "GPIOF6", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_EMMC_PRESENT_N] = {"EMMC_PRESENT_N", "GPIOF7", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},

  [GPIO_SMB_BMC_SLOT1_ALT_R_N] = {"SMB_BMC_SLOT1_ALT_R_N", "GPIOH0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_SMB_BMC_SLOT2_ALT_R_N] = {"SMB_BMC_SLOT2_ALT_R_N", "GPIOH1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SMB_BMC_SLOT3_ALT_R_N] = {"SMB_BMC_SLOT3_ALT_R_N", "GPIOH2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SMB_BMC_SLOT4_ALT_R_N] = {"SMB_BMC_SLOT4_ALT_R_N", "GPIOH3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PRSNT_MB_BMC_SLOT1_BB_N] = {"PRSNT_MB_BMC_SLOT1_BB_N", "GPIOH4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PRSNT_MB_SLOT2_BB_N] = {"PRSNT_MB_SLOT2_BB_N", "GPIOH5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PRSNT_MB_BMC_SLOT3_BB_N] = {"PRSNT_MB_BMC_SLOT3_BB_N", "GPIOH6", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_PRSNT_MB_SLOT4_BB_N] = {"PRSNT_MB_SLOT4_BB_N", "GPIOH7", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  [GPIO_FM_SPI_WP_DISABLE_STATUS_R_N] = {"FM_SPI_WP_DISABLE_STATUS_R_N", "GPIOI5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_BB_BUTTON_BMC_CO_N_R] = {"BB_BUTTON_BMC_CO_N_R", "GPIOI6", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_FM_PWRBRK_SECONDARY_R] = {"FM_PWRBRK_SECONDARY_R", "GPIOI7", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  [GPIO_P3V3_NIC_FAULT_N] = {"P3V3_NIC_FAULT_N", "GPIOM0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_NIC_POWER_BMC_EN_R] = {"NIC_POWER_BMC_EN_R", "GPIOM1", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_P12V_NIC_FAULT_N] = {"P12V_NIC_FAULT_N", "GPIOM2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_USB_BMC_EN_R] = {"USB_BMC_EN_R", "GPIOM3", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1},
  [GPIO_FAST_PROCHOT_BMC_N_R] = {"FAST_PROCHOT_BMC_N_R", "GPIOM4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_RST_BMC_USB_HUB_N_R] = {"RST_BMC_USB_HUB_N_R", "GPIOM5", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1},

  [GPIO_HSC_FAULT_BMC_SLOT1_N_R] = {"HSC_FAULT_BMC_SLOT1_N_R", "GPION0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_HSC_FAULT_SLOT2_N] = {"HSC_FAULT_SLOT2_N", "GPION1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_HSC_FAULT_BMC_SLOT3_N_R] = {"HSC_FAULT_BMC_SLOT3_N_R", "GPION2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_HSC_FAULT_SLOT4_N] = {"HSC_FAULT_SLOT4_N", "GPION3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_BMC_READY_R] = {"BMC_READY_R", "GPION4", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_FM_HSC_BMC_FAULT_N_R] = {"FM_HSC_BMC_FAULT_N_R", "GPION5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  [GPIO_DUAL_FAN0_DETECT_BMC_N_R] = {"DUAL_FAN0_DETECT_BMC_N_R", "GPIOO4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_DUAL_FAN1_DETECT_BMC_N_R] = {"DUAL_FAN1_DETECT_BMC_N_R", "GPIOO5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_FAN0_BMC_CPLD_EN_R] = {"FAN0_BMC_CPLD_EN_R", "GPIOO6", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1},
  [GPIO_FAN1_BMC_CPLD_EN_R] = {"FAN1_BMC_CPLD_EN_R", "GPIOO7", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1},

  [GPIO_FAN2_BMC_CPLD_EN_R] = {"FAN2_BMC_CPLD_EN_R", "GPIOP0", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1},
  [GPIO_FAN3_BMC_CPLD_EN_R] = {"FAN3_BMC_CPLD_EN_R", "GPIOP1", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1},
  [GPIO_RST_PCIE_RESET_SLOT1_N] = {"RST_PCIE_RESET_SLOT1_N", "GPIOP2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_RST_PCIE_RESET_SLOT2_N] = {"RST_PCIE_RESET_SLOT2_N", "GPIOP3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_RST_PCIE_RESET_SLOT3_N] = {"RST_PCIE_RESET_SLOT3_N", "GPIOP4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_RST_PCIE_RESET_SLOT4_N] = {"RST_PCIE_RESET_SLOT4_N", "GPIOP5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_USB_MUX_EN_BMC_N_R] = {"USB_MUX_EN_BMC_N_R", "GPIOP6", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1},

  [GPIO_SLOT1_ID1_DETECT_BMC_N] = {"SLOT1_ID1_DETECT_BMC_N", "GPIOR0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT1_ID0_DETECT_BMC_N] = {"SLOT1_ID0_DETECT_BMC_N", "GPIOR1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT2_ID1_DETECT_BMC_N] = {"SLOT2_ID1_DETECT_BMC_N", "GPIOR2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT2_ID0_DETECT_BMC_N] = {"SLOT2_ID0_DETECT_BMC_N", "GPIOR3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT3_ID1_DETECT_BMC_N] = {"SLOT3_ID1_DETECT_BMC_N", "GPIOR4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT3_ID0_DETECT_BMC_N] = {"SLOT3_ID0_DETECT_BMC_N", "GPIOR5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT4_ID1_DETECT_BMC_N] = {"SLOT4_ID1_DETECT_BMC_N", "GPIOR6", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_SLOT4_ID0_DETECT_BMC_N] = {"SLOT4_ID0_DETECT_BMC_N", "GPIOR7", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  [GPIO_P5V_USB_PG_BMC] = {"P5V_USB_PG_BMC", "GPIOS2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_FM_BMC_TPM_PRSNT_N] = {"FM_BMC_TPM_PRSNT_N", "GPIOS3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_FM_BMC_SLOT1_ISOLATED_EN_R] = {"FM_BMC_SLOT1_ISOLATED_EN_R", "GPIOS4", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1},
  [GPIO_FM_SMB_ISOLATED_EN_R] = {"FM_SMB_ISOLATED_EN_R", "GPIOS4", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS2},
  [GPIO_FM_BMC_SLOT2_ISOLATED_EN_R] = {"FM_BMC_SLOT2_ISOLATED_EN_R", "GPIOS5", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1},
  [GPIO_FM_BMC_SLOT3_ISOLATED_EN_R] = {"FM_BMC_SLOT3_ISOLATED_EN_R", "GPIOS6", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1},
  [GPIO_FM_BMC_SLOT4_ISOLATED_EN_R] = {"FM_BMC_SLOT4_ISOLATED_EN_R", "GPIOS7", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1},

  [GPIO_P12V_EFUSE_DETECT_N] = {"P12V_EFUSE_DETECT_N", "GPIOU1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},

  [GPIO_AC_ON_OFF_BTN_BMC_SLOT1_N_R] = {"AC_ON_OFF_BTN_BMC_SLOT1_N_R", "GPIOV0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_AC_ON_OFF_BTN_SLOT2_N] = {"AC_ON_OFF_BTN_SLOT2_N", "GPIOV1", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_AC_ON_OFF_BTN_BMC_SLOT3_N_R] = {"AC_ON_OFF_BTN_BMC_SLOT3_N_R", "GPIOV2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_AC_ON_OFF_BTN_SLOT4_N] = {"AC_ON_OFF_BTN_SLOT4_N", "GPIOV3", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},
  [GPIO_BOARD_ID0] = {"BOARD_ID0", "GPIOV4", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_BOARD_ID1] = {"BOARD_ID1", "GPIOV5", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_BOARD_ID2] = {"BOARD_ID2", "GPIOV6", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_BOARD_ID3] = {"BOARD_ID3", "GPIOV7", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},

  [GPIO_HSC_BB_BMC_DETECT0] = {"HSC_BB_BMC_DETECT0", "GPIOX2", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  // [GPIO_RST_BMC_WDRST1_R] = {"RST_BMC_WDRST1_R", "GPIOY0", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_RST_BMC_WDRST2_R] = {"RST_BMC_WDRST2_R", "GPIOY1", GPIO_DIRECTION_OUT, GPIO_VALUE_LOW, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_SPI_LOCK_REQ_BMC_N] = {"SPI_LOCK_REQ_BMC_N", "GPIOY2", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},
  [GPIO_EMMC_RST_N_R] = {"EMMC_RST_N_R", "GPIOY3", GPIO_DIRECTION_OUT, GPIO_VALUE_HIGH, SYS_CFG_CLASS1 | SYS_CFG_CLASS2},

  [GPIO_HSC_BB_BMC_DETECT1] = {"HSC_BB_BMC_DETECT1", "GPIOZ0", GPIO_DIRECTION_IN, GPIO_VALUE_INVALID, SYS_CFG_CLASS1},

  // LAST
  {NULL, NULL, GPIO_DIRECTION_INVALID, GPIO_VALUE_INVALID, SYS_CFG_CLASS1 | SYS_CFG_CLASS2}
};

const uint8_t cl_gpio_pin_size = sizeof(cl_gpio_pin_name)/sizeof(cl_gpio_pin_name[0]);
const uint8_t hd_gpio_pin_size = sizeof(hd_gpio_pin_name)/sizeof(hd_gpio_pin_name[0]);
const uint8_t gl_gpio_pin_size = sizeof(gl_gpio_pin_name)/sizeof(gl_gpio_pin_name[0]);
const uint8_t ji_gpio_pin_size = sizeof(ji_gpio_pin_name)/sizeof(ji_gpio_pin_name[0]);

//Great Lakes BIC virtual gpio
const uint8_t gl_virtual_gpio_pin_size = sizeof(gl_virtual_gpio_pin_name)/sizeof(gl_virtual_gpio_pin_name[0]);

const uint8_t vf_gpio_pin_size = sizeof(vf_gpio_pin_name)/sizeof(vf_gpio_pin_name[0]);
const uint8_t rf_gpio_pin_size = sizeof(rf_gpio_pin_name)/sizeof(rf_gpio_pin_name[0]);

uint8_t
y35_get_gpio_list_size(uint8_t fru, bool is_virtual_gpio, uint8_t intf) {
  uint8_t gpio_list_size = 0, exp_type = TYPE_1OU_UNKNOWN;
  int slot_type = SERVER_TYPE_NONE;

  switch (intf) {
    case NONE_INTF:
      slot_type = fby35_common_get_slot_type(fru);
      switch (slot_type) {
        case SERVER_TYPE_CL:
        case SERVER_TYPE_CL_EMR:
          gpio_list_size = cl_gpio_pin_size;
          break;
        case SERVER_TYPE_HD:
          gpio_list_size = hd_gpio_pin_size;
          break;
        case SERVER_TYPE_GL:
          if (is_virtual_gpio == true) {
            gpio_list_size = gl_virtual_gpio_pin_size;
          } else {
            gpio_list_size = gl_gpio_pin_size;
          }
          break;
        case SERVER_TYPE_JI:
          gpio_list_size = ji_gpio_pin_size;
          break;
        default:
          gpio_list_size = cl_gpio_pin_size;
          break;
      }
      break;
    case FEXP_BIC_INTF:
      if (bic_get_1ou_type(fru, &exp_type) != 0) {
        break;
      }
      switch (exp_type) {
        case TYPE_1OU_VERNAL_FALLS_WITH_AST:
          gpio_list_size = vf_gpio_pin_size;
          break;
        case TYPE_1OU_RAINBOW_FALLS:
          gpio_list_size = rf_gpio_pin_size;
          break;
      }
      break;
  }

  return gpio_list_size;
}

int
y35_get_gpio_name(uint8_t fru, uint8_t gpio, char *name, bool is_virtual_gpio, uint8_t intf) {
  uint8_t gpio_pin_size = 0, exp_type = TYPE_1OU_UNKNOWN;
  int slot_type = SERVER_TYPE_NONE;
  const char **gpio_pin_name = NULL;

  //TODO: Add support for BMC GPIO pins
  if (fru < 1 || fru > 4) {
#ifdef DEBUG
    syslog(LOG_WARNING, "y35_get_gpio_name: Wrong fru %u", fru);
#endif
    return -1;
  }

  switch (intf) {
    case NONE_INTF:
      slot_type = fby35_common_get_slot_type(fru);
      switch (slot_type) {
        case SERVER_TYPE_CL:
        case SERVER_TYPE_CL_EMR:
          gpio_pin_size = cl_gpio_pin_size;
          gpio_pin_name = cl_gpio_pin_name;
          break;
        case SERVER_TYPE_HD:
          gpio_pin_size = hd_gpio_pin_size;
          gpio_pin_name = hd_gpio_pin_name;
          break;
        case SERVER_TYPE_GL:
          if (is_virtual_gpio == true) {
            gpio_pin_size = gl_virtual_gpio_pin_size;
            gpio_pin_name = gl_virtual_gpio_pin_name;
          } else {
            gpio_pin_size = gl_gpio_pin_size;
            gpio_pin_name = gl_gpio_pin_name;
          }
          break;
        case SERVER_TYPE_JI:
          gpio_pin_size = ji_gpio_pin_size;
          gpio_pin_name = ji_gpio_pin_name;
          break;
        default:
          gpio_pin_size = cl_gpio_pin_size;
          gpio_pin_name = cl_gpio_pin_name;
          break;
      }
      break;
    case FEXP_BIC_INTF:
      if (bic_get_1ou_type(fru, &exp_type) != 0) {
        break;
      }
      switch (exp_type) {
        case TYPE_1OU_VERNAL_FALLS_WITH_AST:
          gpio_pin_size = vf_gpio_pin_size;
          gpio_pin_name = vf_gpio_pin_name;
          break;
        case TYPE_1OU_RAINBOW_FALLS:
          gpio_pin_size = rf_gpio_pin_size;
          gpio_pin_name = rf_gpio_pin_name;
          break;
      }
      break;
  }

  if ((gpio >= gpio_pin_size) || (gpio_pin_size == 0)) {
#ifdef DEBUG
    syslog(LOG_WARNING, "y35_get_gpio_name: Wrong gpio pin %u, gpio pin size %d", gpio, gpio_pin_size);
#endif
    return -1;
  }

  if (strcmp(gpio_pin_name[gpio], "") == 0) {
    return -1;
  }

  sprintf(name, "%s", gpio_pin_name[gpio]);
  return 0;
}
