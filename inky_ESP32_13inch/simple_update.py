#!/usr/bin/env python3
"""Trigger an Inky 13.3" display refresh without sending any pixel data."""

import time
import warnings

import gpiod
import gpiodevice
import spidev
from gpiod.line import Bias, Direction, Value

RESET_PIN = 27  # PIN13
BUSY_PIN = 17   # PIN11
DC_PIN = 22     # PIN15
CS0_PIN = 26
CS1_PIN = 16

CS0_SEL = 0b01
CS1_SEL = 0b10
CS_BOTH_SEL = CS0_SEL | CS1_SEL

EL133UF1_POF = 0x02
EL133UF1_PON = 0x04
EL133UF1_DRF = 0x12
EL133UF1_CMD66 = 0xF0
EL133UF1_PSR = 0x00
EL133UF1_PLL = 0x30
EL133UF1_CDI = 0x50
EL133UF1_TCON = 0x60
EL133UF1_AGID = 0x86
EL133UF1_PWS = 0xE3
EL133UF1_CCSET = 0xE0
EL133UF1_TRES = 0x61
EL133UF1_ANTM = 0x74
EL133UF1_PWR = 0x01
EL133UF1_EN_BUF = 0xB6
EL133UF1_BTST_P = 0x06
EL133UF1_BOOST_VDDP_EN = 0xB7
EL133UF1_BTST_N = 0x05
EL133UF1_BUCK_BOOST_VDDN = 0xB0
EL133UF1_TFT_VCOM_POWER = 0xB1

# --- GPIO setup ---

gpiochip = gpiodevice.find_chip_by_platform()
gpiodevice.friendly_errors = True

gpiodevice.check_pins_available(gpiochip, {
    "Chip Select 0": CS0_PIN,
    "Chip Select 1": CS1_PIN,
    "Data/Command": DC_PIN,
    "Reset": RESET_PIN,
    "Busy": BUSY_PIN,
})

cs0_pin = gpiochip.line_offset_from_id(CS0_PIN)
cs1_pin = gpiochip.line_offset_from_id(CS1_PIN)
dc_pin = gpiochip.line_offset_from_id(DC_PIN)
reset_pin = gpiochip.line_offset_from_id(RESET_PIN)
busy_pin = gpiochip.line_offset_from_id(BUSY_PIN)

gpio = gpiochip.request_lines(consumer="inky", config={
    cs0_pin: gpiod.LineSettings(direction=Direction.OUTPUT, output_value=Value.ACTIVE, bias=Bias.DISABLED),
    cs1_pin: gpiod.LineSettings(direction=Direction.OUTPUT, output_value=Value.ACTIVE, bias=Bias.DISABLED),
    dc_pin: gpiod.LineSettings(direction=Direction.OUTPUT, output_value=Value.INACTIVE, bias=Bias.DISABLED),
    reset_pin: gpiod.LineSettings(direction=Direction.OUTPUT, output_value=Value.ACTIVE, bias=Bias.DISABLED),
    busy_pin: gpiod.LineSettings(direction=Direction.INPUT, bias=Bias.PULL_UP),
})

spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 10000000


# --- Helpers ---

def busy_wait(timeout=40.0):
    if gpio.get_value(busy_pin) == Value.ACTIVE:
        print("Sleeping active")
        time.sleep(timeout)
        return
    t_start = time.time()
    while gpio.get_value(busy_pin) == Value.ACTIVE:
        time.sleep(0.1)
        if time.time() - t_start > timeout:
            warnings.warn(f"Busy Wait: Timed out after {timeout:0.2f}s")
            return


def send_command(command, cs_sel=None, data=None):
    if cs_sel is not None:
        if cs_sel & CS0_SEL:
            gpio.set_value(cs0_pin, Value.INACTIVE)
        if cs_sel & CS1_SEL:
            gpio.set_value(cs1_pin, Value.INACTIVE)

        gpio.set_value(dc_pin, Value.INACTIVE)
        time.sleep(0.3)
        spi.xfer3([command])

        if data is not None:
            gpio.set_value(dc_pin, Value.ACTIVE)
            spi.xfer3(data)

        gpio.set_value(cs0_pin, Value.ACTIVE)
        gpio.set_value(cs1_pin, Value.ACTIVE)
        gpio.set_value(dc_pin, Value.INACTIVE)


# --- Reset and init ---

gpio.set_value(reset_pin, Value.INACTIVE)
time.sleep(0.03)
gpio.set_value(reset_pin, Value.ACTIVE)
time.sleep(0.03)

print("sleepy")
busy_wait(0.3)
print("not sleepy")

send_command(EL133UF1_ANTM, CS0_SEL, [0xC0, 0x1C, 0x1C, 0xCC, 0xCC, 0xCC, 0x15, 0x15, 0x55])
send_command(EL133UF1_CMD66, CS_BOTH_SEL, [0x49, 0x55, 0x13, 0x5D, 0x05, 0x10])
send_command(EL133UF1_PSR, CS_BOTH_SEL, [0xDF, 0x69])
send_command(EL133UF1_PLL, CS_BOTH_SEL, [0x08])
send_command(EL133UF1_CDI, CS_BOTH_SEL, [0xF7])
send_command(EL133UF1_TCON, CS_BOTH_SEL, [0x03, 0x03])
send_command(EL133UF1_AGID, CS_BOTH_SEL, [0x10])
send_command(EL133UF1_PWS, CS_BOTH_SEL, [0x22])
send_command(EL133UF1_CCSET, CS_BOTH_SEL, [0x01])
send_command(EL133UF1_TRES, CS_BOTH_SEL, [0x04, 0xB0, 0x03, 0x20])
send_command(EL133UF1_PWR, CS0_SEL, [0x0F, 0x00, 0x28, 0x2C, 0x28, 0x38])
send_command(EL133UF1_EN_BUF, CS0_SEL, [0x07])
send_command(EL133UF1_BTST_P, CS0_SEL, [0xD8, 0x18])
send_command(EL133UF1_BOOST_VDDP_EN, CS0_SEL, [0x01])
send_command(EL133UF1_BTST_N, CS0_SEL, [0xD8, 0x18])
send_command(EL133UF1_BUCK_BOOST_VDDN, CS0_SEL, [0x01])
send_command(EL133UF1_TFT_VCOM_POWER, CS0_SEL, [0x02])

# --- Trigger refresh (no pixel data) ---

send_command(EL133UF1_PON, CS_BOTH_SEL)
busy_wait(0.2)

send_command(EL133UF1_DRF, CS_BOTH_SEL, [0x00])
busy_wait(32.0)

send_command(EL133UF1_POF, CS_BOTH_SEL, [0x00])
busy_wait(0.2)

print("Done.")
