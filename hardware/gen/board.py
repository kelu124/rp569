"""rp569 board netlist: single source of truth for the schematic and PCB generators.

Pin assignments follow docs/pinmap.md, power follows docs/power.md.
Each part: ref, lib_id, value, footprint, {pin_number: net}, block, extra flags.
Pins of the symbol not listed in the map get a no-connect flag.
"""
import os
from collections import defaultdict

SYMLIB = '/usr/share/kicad/symbols/'
CH569_LIB = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'lib', 'rp569.kicad_sym')

FP_R = 'Resistor_SMD:R_0402_1005Metric'
FP_C = 'Capacitor_SMD:C_0402_1005Metric'
FP_C_BIG = 'Capacitor_SMD:C_0805_2012Metric'
FP_RN = 'Resistor_SMD:R_Array_Convex_4x0402'
FP_L_BUCK = 'Inductor_SMD:L_Changjiang_FTC303015D'
FP_L_RP = 'Inductor_SMD:L_Cenker_CKCS201610'
FP_XTAL = 'Crystal:Crystal_SMD_3225-4Pin_3.2x2.5mm'
FP_LED = 'LED_SMD:LED_LiteOn_LTST-C19HE1WT'   # pad 4 = common anode, 1/2/3 = R/G/B cathodes
FP_SW = 'Button_Switch_SMD:SW_SPST_PTS810'
FP_HOLE = 'MountingHole:MountingHole_3.2mm_M3'


def hdr(n):
    return f'Connector_PinHeader_2.54mm:PinHeader_1x{n:02d}_P2.54mm_Vertical'


parts = []
_count = defaultdict(int)


def add(prefix, lib_id, value, fp, pins, block, dnp=False):
    _count[prefix] += 1
    ref = f'{prefix}{_count[prefix]}'
    parts.append(dict(ref=ref, lib_id=lib_id, value=value, fp=fp, pins=pins, block=block, dnp=dnp))
    return ref


def R(value, a, b, block, fp=FP_R, dnp=False):
    return add('R', 'Device:R', value, fp, {'1': a, '2': b}, block, dnp)


def C(value, a, block, b='GND', fp=None):
    big = value in ('10uF', '22uF')
    return add('C', 'Device:C', value, fp or (FP_C_BIG if big else FP_C), {'1': a, '2': b}, block)


def RN(value, pairs, block):
    """4-resistor array; pairs = [(side1, side2)] x4 -> R1:1-8, R2:2-7, R3:3-6, R4:4-5."""
    m = {}
    for (a, b), (p1, p2) in zip(pairs, [('1', '8'), ('2', '7'), ('3', '6'), ('4', '5')]):
        m[p1], m[p2] = a, b
    return add('RN', 'Device:R_Pack04', value, FP_RN, m, block)


def pwr_flag(net, block):
    _count['#FLG'] += 1
    parts.append(dict(ref=f'#FLG{_count["#FLG"]:02d}', lib_id='power:PWR_FLAG', value='PWR_FLAG', fp='',
                      pins={'1': net}, block=block, dnp=False))


# --------------------------------------------------------------------------- power
B = 'Power'
add('J', 'Connector:USB_C_Receptacle_USB2.0_16P', 'USB-C (RP2350)',
    'Connector_USB:USB_C_Receptacle_HRO_TYPE-C-31-M-12',
    {'A1': 'GND', 'A12': 'GND', 'B1': 'GND', 'B12': 'GND', 'SH': 'GND',
     'A4': 'VBUS_C', 'A9': 'VBUS_C', 'B4': 'VBUS_C', 'B9': 'VBUS_C',
     'A5': 'USBC_CC1', 'B5': 'USBC_CC2', 'A6': 'USBC_DP', 'B6': 'USBC_DP', 'A7': 'USBC_DM', 'B7': 'USBC_DM'},
    B)
R('5.1k', 'USBC_CC1', 'GND', B)
R('5.1k', 'USBC_CC2', 'GND', B)
add('U', 'Power_Protection:TPD2EUSB30', 'TPD2EUSB30', 'Package_TO_SOT_SMD:Texas_DRT-3',
    {'1': 'USBC_DP', '2': 'USBC_DM', '3': 'GND'}, B)
C('4.7uF', 'VBUS_C', B)
# ideal-diode OR -> 5V_SYS (no back-feed between the two connectors)
for vin in ('VBUS_C', 'VBUS_3'):
    add('U', 'Power_Management:LM66100DCK', 'LM66100', 'Package_TO_SOT_SMD:SOT-363_SC-70-6',
        {'1': vin, '2': 'GND', '3': 'GND', '6': '5V_SYS'}, B)
C('10uF', '5V_SYS', B)
# CH569 supply: VBUS_3 (or 5V_SYS, DNP) -> 5V_CH_IN -> load switch -> 5V_CH
R('0R', 'VBUS_3', '5V_CH_IN', B)
R('0R', '5V_SYS', '5V_CH_IN', B, dnp=True)
add('U', 'Power_Management:AP2171W', 'AP2171W', 'Package_TO_SOT_SMD:SOT-23-5',
    {'5': '5V_CH_IN', '1': '5V_CH', '2': 'GND', '4': 'CH569_PWR_EN', '3': 'CH569_PWR_FAULT_N'}, B)
C('1uF', '5V_CH_IN', B)
C('10uF', '5V_CH', B)
R('100k', 'CH569_PWR_EN', 'GND', B)
R('10k', 'CH569_PWR_FAULT_N', '+3V3', B)


def buck(vin, vout, r_top, r_bot, block):
    sw, fb = f'{vout}_SW', f'{vout}_FB'
    add('U', 'Regulator_Switching:TLV62569DBV', 'TLV62569', 'Package_TO_SOT_SMD:SOT-23-5',
        {'1': vin, '2': 'GND', '3': sw, '4': vin, '5': fb}, block)
    add('L', 'Device:L', '2.2uH', FP_L_BUCK, {'1': sw, '2': vout}, block)
    R(r_top, vout, fb, block)
    R(r_bot, fb, 'GND', block)
    C('10uF', vin, block)
    C('22uF', vout, block)
    C('22uF', vout, block)


buck('5V_SYS', '+3V3', '453k', '100k', B)        # RP2350 side 3.3 V
buck('5V_CH', '3V3_CH', '453k', '100k', B)       # CH569 3.3 V (switched)
buck('5V_CH', '1V2_CH', '100k', '100k', B)       # CH569 1.2 V (switched)
# nets without a power_out pin (VBUS_3/GND are driven by the USB3 symbol, 5V_SYS/5V_CH by the switches)
for n in ('VBUS_C', '5V_CH_IN', '+3V3', '3V3_CH', '1V2_CH', '1V1', 'VREG_AVDD'):
    pwr_flag(n, B)
# power monitoring dividers
R('100k', 'VBUS_3', 'VBUS3_SENSE', B)
R('47k', 'VBUS3_SENSE', 'GND', B)
R('100k', '3V3_CH', 'CH3V3_SENSE', B)
R('100k', 'CH3V3_SENSE', 'GND', B)

# --------------------------------------------------------------------------- RP2350B
B = 'RP2350B'
rp = {
    '81': 'GND', '30': 'XIN_RP', '31': 'XOUT_RP', '33': 'SWCLK', '34': 'SWDIO', '35': 'RUN',
    '59': '+3V3', '61': 'VREG_AVDD', '62': 'GND', '63': 'VREG_LX', '64': '+3V3', '65': '1V1',
    '66': 'RP_USB_DM', '67': 'RP_USB_DP', '68': '+3V3', '69': '+3V3',
    '70': 'QSPI_SD3', '71': 'QSPI_SCLK', '72': 'QSPI_SD0', '73': 'QSPI_SD2', '74': 'QSPI_SD1', '75': 'QSPI_SS',
}
IOVDD = ['5', '15', '24', '29', '41', '50', '60', '76']
DVDD = ['10', '32', '51']
for p in IOVDD:
    rp[p] = '+3V3'
for p in DVDD:
    rp[p] = '1V1'
gpio_pin = {0: '77', 1: '78', 2: '79', 3: '80', 4: '1', 5: '2', 6: '3', 7: '4', 8: '6', 9: '7', 10: '8', 11: '9',
            12: '11', 13: '12', 14: '13', 15: '14', 16: '16', 17: '17', 18: '18', 19: '19', 20: '20', 21: '21',
            22: '22', 23: '23', 24: '25', 25: '26', 26: '27', 27: '28', 28: '36', 29: '37', 30: '38', 31: '39',
            32: '40', 33: '42', 34: '43', 35: '44', 36: '45', 37: '46', 38: '47', 39: '48', 40: '49', 41: '52',
            42: '53', 43: '54', 44: '55', 45: '56', 46: '57', 47: '58'}
HSPI = ['HSPI_D0', 'HSPI_D1', 'HSPI_D2', 'HSPI_D3', 'HSPI_D4', 'HSPI_D5', 'HSPI_D6', 'HSPI_D7',
        'HSPI_RCLK', 'HSPI_RACT', 'HSPI_RVLD', 'HSPI_RACK', 'HSPI_TCLK', 'HSPI_TREQ', 'HSPI_TVLD', 'HSPI_TRDY']
gpio = {}
for i in range(8):
    gpio[i] = f'LA{i}_R'
for i, n in enumerate(HSPI):
    gpio[8 + i] = n + '_RP'
gpio.update({24: 'UART_RP2CH', 25: 'UART_CH2RP', 26: 'I2C_LINK_SDA', 27: 'I2C_LINK_SCL',
             28: 'RP_I2C_SDA', 29: 'RP_I2C_SCL', 30: 'RP_BTN1', 31: 'RP_BTN2',
             32: 'RP_DBG_TX', 33: 'RP_DBG_RX', 34: 'CH569_RST_N', 35: 'CH569_PWR_EN',
             36: 'RP_LED_R', 37: 'RP_LED_G', 38: 'RP_LED_B', 39: 'CH569_PWR_FAULT_N',
             46: 'VBUS3_SENSE', 47: 'CH3V3_SENSE'})
for i in range(40, 46):
    gpio[i] = f'RP_ADC{i - 40}'
for g, n in gpio.items():
    rp[gpio_pin[g]] = n
add('U', 'MCU_RaspberryPi:RP2350B', 'RP2350B', 'Package_DFN_QFN:QFN-80-1EP_10x10mm_P0.4mm_EP3.4x3.4mm', rp, B)
for p in IOVDD + ['59', '68', '69']:
    C('100nF', '+3V3', B)
for p in DVDD:
    C('100nF', '1V1', B)
# core regulator (hardware-design-with-rp2350 §2.1)
add('L', 'Device:L', '3.3uH', FP_L_RP, {'1': 'VREG_LX', '2': '1V1'}, B)
C('4.7uF', '+3V3', B)          # VREG_VIN
C('4.7uF', '1V1', B)           # DVDD output
R('33R', '+3V3', 'VREG_AVDD', B)
C('4.7uF', 'VREG_AVDD', B)
C('10uF', '+3V3', B)
# crystal
add('Y', 'Device:Crystal_GND24', '12MHz', FP_XTAL, {'1': 'XIN_RP', '3': 'XOUT_RP_X', '2': 'GND', '4': 'GND'}, B)
R('1k', 'XOUT_RP', 'XOUT_RP_X', B)
C('15pF', 'XIN_RP', B)
C('15pF', 'XOUT_RP_X', B)
# USB
R('27R', 'USBC_DP', 'RP_USB_DP', B)
R('27R', 'USBC_DM', 'RP_USB_DM', B)
# flash + BOOTSEL + RUN
add('U', 'Memory_Flash:W25Q128JVS', 'W25Q128JVS', 'Package_SO:SOIC-8_5.3x5.3mm_P1.27mm',
    {'1': 'QSPI_SS', '2': 'QSPI_SD1', '3': 'QSPI_SD2', '4': 'GND', '5': 'QSPI_SD0', '6': 'QSPI_SCLK',
     '7': 'QSPI_SD3', '8': '+3V3'}, B)
C('100nF', '+3V3', B)
R('1k', 'QSPI_SS', 'BOOTSEL_SW', B)
add('SW', 'Switch:SW_Push', 'BOOTSEL', FP_SW, {'1': 'BOOTSEL_SW', '2': 'GND'}, B)
R('10k', 'RUN', '+3V3', B)
add('SW', 'Switch:SW_Push', 'RUN', FP_SW, {'1': 'RUN', '2': 'GND'}, B)
add('J', 'Connector_Generic:Conn_01x03', 'SWD', hdr(3), {'1': 'SWCLK', '2': 'GND', '3': 'SWDIO'}, B)
# RGB LED (common anode, GPIO sinks)
add('D', 'Device:LED_RGBA', 'RGB', FP_LED, {'4': '+3V3', '1': 'RP_LED_RK', '2': 'RP_LED_GK', '3': 'RP_LED_BK'}, B)
for c in 'RGB':   # G/B Vf up to 3.9 V on a 3.3 V rail: smaller resistors
    R('330R' if c == 'R' else '100R', f'RP_LED_{c}', f'RP_LED_{c}K', B)
# headers
add('J', 'Connector_Generic:Conn_01x04', 'RP I2C', hdr(4), {'1': '+3V3', '2': 'RP_I2C_SDA', '3': 'RP_I2C_SCL', '4': 'GND'}, B)
R('4.7k', 'RP_I2C_SDA', '+3V3', B)
R('4.7k', 'RP_I2C_SCL', '+3V3', B)
# debug UART shares the EXT header (no room for a separate one on 50x70 mm)
add('J', 'Connector_Generic:Conn_01x10', 'RP EXT', hdr(10),
    {'1': '+3V3', '2': 'RP_DBG_TX', '3': 'RP_DBG_RX', '4': 'RP_ADC0', '5': 'RP_ADC1', '6': 'RP_ADC2',
     '7': 'RP_ADC3', '8': 'RP_ADC4', '9': 'RP_ADC5', '10': 'GND'}, B)
# user buttons (active low, external pull-ups)
for i in (1, 2):
    add('SW', 'Switch:SW_Push', f'RP BTN{i}', FP_SW, {'1': f'RP_BTN{i}', '2': 'GND'}, B)
    R('10k', f'RP_BTN{i}', '+3V3', B)

# --------------------------------------------------------------------------- logic analyser
B = 'Logic analyser'
add('J', 'Connector_Generic:Conn_01x10', 'LA IN', hdr(10),
    {**{str(i + 1): f'LA{i}' for i in range(8)}, '9': 'GND', '10': 'GND'}, B)
RN('100R', [(f'LA{i}', f'LA{i}_R') for i in range(4)], B)
RN('100R', [(f'LA{i}', f'LA{i}_R') for i in range(4, 8)], B)

# --------------------------------------------------------------------------- inter-chip link
B = 'Inter-chip'
for k in range(4):
    RN('33R', [(n + '_RP', n) for n in HSPI[4 * k:4 * k + 4]], B)
R('4.7k', 'I2C_LINK_SDA', '+3V3', B)
R('4.7k', 'I2C_LINK_SCL', '+3V3', B)
R('10k', 'CH569_RST_N', '3V3_CH', B)

# --------------------------------------------------------------------------- CH569W
B = 'CH569W'
ch = {
    '1': 'USB3_DP', '2': 'USB3_DM', '3': '3V3_CH', '4': 'CH_SSTX_P', '5': 'CH_SSTX_N', '6': '1V2_CH',
    '7': 'USB3_SSRX_P', '8': 'USB3_SSRX_N', '9': '1V2_CH',
    '10': 'HSPI_RCLK', '11': 'HSPI_RACT', '12': 'UART_CH2RP', '13': 'UART_RP2CH', '14': 'HSPI_RVLD',
    '15': 'HSPI_D0', '16': '3V3_CH', '17': 'HSPI_D1', '18': 'HSPI_TRDY', '19': 'HSPI_D3', '20': 'HSPI_D4',
    '21': 'HSPI_D5', '22': 'HSPI_D6', '23': 'HSPI_D7', '24': 'SD_D3', '25': 'SD_D2', '26': 'SD_D1',
    '27': 'SD_D0', '28': 'CH_PA17', '29': 'SD_CMD', '30': 'CH569_RST_N', '31': 'SD_CLK',
    '32': 'CH_LED_R', '33': 'CH_LED_G', '34': 'CH_LED_B', '35': 'CH_PA20', '36': 'CH_BTN1', '37': 'CH_BTN2',
    '38': 'CH_DBG_RX', '39': 'CH_DBG_TX', '40': 'CH_PA16', '41': 'CH_PB7', '42': '3V3_CH',
    '43': 'CH_I2C_SDA', '44': 'CH_I2C_SCL', '45': 'I2C_LINK_SDA', '46': 'CH_PB10', '47': 'I2C_LINK_SCL',
    '48': 'CH_PB11', '49': 'CH_PB24', '50': 'CH_PB12', '51': 'CH_PB13', '52': 'HSPI_TVLD', '53': 'HSPI_D2',
    '54': 'HSPI_TREQ', '55': 'HSPI_RACK', '56': 'HSPI_TCLK', '57': 'CH_PA12', '58': 'CH_PA13',
    '59': 'CH_PA14', '60': 'SD_DET', '61': '3V3_CH', '62': '3V3_CH', '63': '1V2_CH', '64': '3V3_CH',
    '67': 'XO_CH', '68': 'XI_CH', '69': 'GND',
}
add('U', 'rp569:CH569W', 'CH569W', 'Package_DFN_QFN:QFN-68-1EP_8x8mm_P0.4mm_EP5.2x5.2mm', ch, B)
for p, n in ch.items():
    if n in ('3V3_CH', '1V2_CH'):
        C('100nF', n, B)
add('Y', 'Device:Crystal_GND24', '30MHz', FP_XTAL, {'1': 'XO_CH', '3': 'XI_CH', '2': 'GND', '4': 'GND'}, B)
C('15pF', 'XI_CH', B)
C('15pF', 'XO_CH', B)
R('1k', 'HSPI_D0', 'CH_BOOT_SW', B)
add('SW', 'Switch:SW_Push', 'CH569 BOOT', FP_SW, {'1': 'CH_BOOT_SW', '2': 'GND'}, B)
# jumper fitted = PA5 held low at every power-up -> USB bootloader (blank-chip / recovery flashing)
add('J', 'Connector_Generic:Conn_01x02', 'CH BOOT JP', hdr(2), {'1': 'CH_BOOT_SW', '2': 'GND'}, B)
# user buttons (active low, pull-ups on the CH569 rail)
for i in (1, 2):
    add('SW', 'Switch:SW_Push', f'CH BTN{i}', FP_SW, {'1': f'CH_BTN{i}', '2': 'GND'}, B)
    R('10k', f'CH_BTN{i}', '3V3_CH', B)
add('D', 'Device:LED_RGBA', 'RGB', FP_LED, {'4': '3V3_CH', '1': 'CH_LED_RK', '2': 'CH_LED_GK', '3': 'CH_LED_BK'}, B)
for c in 'RGB':   # G/B Vf up to 3.9 V on a 3.3 V rail: smaller resistors
    R('330R' if c == 'R' else '100R', f'CH_LED_{c}', f'CH_LED_{c}K', B)
add('J', 'Connector_Generic:Conn_01x04', 'CH I2C', hdr(4), {'1': '3V3_CH', '2': 'CH_I2C_SDA', '3': 'CH_I2C_SCL', '4': 'GND'}, B)
R('4.7k', 'CH_I2C_SDA', '3V3_CH', B)
R('4.7k', 'CH_I2C_SCL', '3V3_CH', B)
add('J', 'Connector_Generic:Conn_01x03', 'CH UART', hdr(3), {'1': 'CH_DBG_TX', '2': 'CH_DBG_RX', '3': 'GND'}, B)
ext = ['CH_PA12', 'CH_PA13', 'CH_PA14', 'CH_PA16', 'CH_PA17', 'CH_PA20', 'CH_PB7',
       'CH_PB10', 'CH_PB11', 'CH_PB12', 'CH_PB13', 'CH_PB24']
add('J', 'Connector_Generic:Conn_02x07_Odd_Even', 'CH EXT',
    'Connector_PinHeader_2.54mm:PinHeader_2x07_P2.54mm_Vertical',
    {**{str(i + 1): n for i, n in enumerate(ext)}, '13': '3V3_CH', '14': 'GND'}, B)

# --------------------------------------------------------------------------- USB3 + SD
B = 'USB3 / SD'
add('J', 'Connector:USB3_B_Micro', 'USB3 Micro-B (CH569)', 'rp569:USB3_Micro-B_Amphenol_GSB343K33HR',   # THT shell legs (D-013)
    {'1': 'VBUS_3', '2': 'USB3_DM', '3': 'USB3_DP', '5': 'GND', '6': 'USB3_SSTX_N', '7': 'USB3_SSTX_P',
     '8': 'GND', '9': 'USB3_SSRX_N', '10': 'USB3_SSRX_P', 'SH': 'GND'}, B)
C('100nF', 'CH_SSTX_P', B, b='USB3_SSTX_P')
C('100nF', 'CH_SSTX_N', B, b='USB3_SSTX_N')
for a, b in (('USB3_DP', 'USB3_DM'), ('USB3_SSRX_P', 'USB3_SSRX_N'), ('USB3_SSTX_P', 'USB3_SSTX_N')):
    add('U', 'Power_Protection:TPD2EUSB30', 'TPD2EUSB30', 'Package_TO_SOT_SMD:Texas_DRT-3',
        {'1': a, '2': b, '3': 'GND'}, B)
C('4.7uF', 'VBUS_3', B)
add('J', 'Connector:Micro_SD_Card_Det_Hirose_DM3AT', 'microSD', 'Connector_Card:microSD_HC_Hirose_DM3AT-SF-PEJM5',
    {'1': 'SD_D2', '2': 'SD_D3', '3': 'SD_CMD', '4': '3V3_CH', '5': 'SD_CLK', '6': 'GND', '7': 'SD_D0',
     '8': 'SD_D1', '9': 'SD_DET', '10': 'GND', 'SH': 'GND'}, B)
for n in ('SD_CMD', 'SD_D0', 'SD_D1', 'SD_D2', 'SD_D3'):
    R('47k', n, '3V3_CH', B)
R('100k', 'SD_DET', '3V3_CH', B)
C('100nF', '3V3_CH', B)
C('4.7uF', '3V3_CH', B)

# --------------------------------------------------------------------------- mechanical
for _ in range(4):
    add('H', 'Mechanical:MountingHole', 'M3', FP_HOLE, {}, 'Mechanical')

# --------------------------------------------------------------------------- JLCPCB / LCSC part numbers
# Checked against the jlcparts snapshot of the JLCPCB catalogue, 2026-10-23 (stock, basic/extended): see
# docs/bom.md. '' = not stocked by JLCPCB -> consigned part (bought elsewhere, sent to JLCPCB) or hand-soldered.
LCSC_BY_VALUE = {
    # passives (0402 unless noted); Basic parts where JLCPCB has one
    ('Device:C', '100nF'): ('C1525', 'CL05B104KO5NNNC'), ('Device:C', '1uF'): ('C52923', 'CL05A105KA5NQNC'),
    ('Device:C', '4.7uF'): ('C23733', 'CL05A475MP5NRNC'), ('Device:C', '15pF'): ('C1548', '0402CG150J500NT'),
    ('Device:C', '10uF'): ('C15850', 'CL21A106KAYNNNE'), ('Device:C', '22uF'): ('C45783', 'CL21A226MAQNNNE'),
    ('Device:R', '0R'): ('C17168', '0402WGF0000TCE'), ('Device:R', '27R'): ('C138021', 'RC0402FR-0727RL'),
    ('Device:R', '33R'): ('C25105', '0402WGF330JTCE'), ('Device:R', '100R'): ('C25076', '0402WGF1000TCE'),
    ('Device:R', '330R'): ('C25104', '0402WGF3300TCE'), ('Device:R', '1k'): ('C11702', '0402WGF1001TCE'),
    ('Device:R', '4.7k'): ('C25900', '0402WGF4701TCE'), ('Device:R', '5.1k'): ('C25905', '0402WGF5101TCE'),
    ('Device:R', '10k'): ('C25744', '0402WGF1002TCE'), ('Device:R', '47k'): ('C25792', '0402WGF4702TCE'),
    ('Device:R', '100k'): ('C25741', '0402WGF1003TCE'), ('Device:R', '453k'): ('C27009', '0402WGF4533TCE'),
    ('Device:R_Pack04', '33R'): ('C25501', '4D02WGJ0330TCE'), ('Device:R_Pack04', '100R'): ('C48442', '4D02WGJ0101TCE'),
    ('Device:L', '2.2uH'): ('C7423318', 'FTC303020D2R2MBCA'),     # FTC303015D2R2MBCA: only 9 in stock; same 3x3 family, 2.0 mm high
    ('Device:L', '3.3uH'): ('C42411119', 'AOTA-B201610S3R3-101-T'),   # the part named in the RP2350 HW design guide
    ('Device:Crystal_GND24', '12MHz'): ('C20625731', 'ABM8-272-T3'),  # RP2350 HW design guide part, CL 10 pF
    ('Device:Crystal_GND24', '30MHz'): ('C70592', 'X322530MOB4SI'),   # CL 12 pF
    ('Device:LED_RGBA', 'RGB'): ('C458749', 'LTST-C19HE1WT'),
    ('Switch:SW_Push', None): ('C221895', 'PTS810SJG250SMTRLFS'),
    # ICs and connectors
    ('MCU_RaspberryPi:RP2350B', 'RP2350B'): ('C42415655', 'RP2350B'),
    ('Memory_Flash:W25Q128JVS', 'W25Q128JVS'): ('C97521', 'W25Q128JVSIQ'),
    ('Power_Management:AP2171W', 'AP2171W'): ('C110466', 'AP2171WG-7'),
    ('Regulator_Switching:TLV62569DBV', 'TLV62569'): ('C141836', 'TLV62569DBVR'),
    ('Power_Protection:TPD2EUSB30', 'TPD2EUSB30'): ('C97502', 'TPD2EUSB30DRTR'),
    ('Connector:USB_C_Receptacle_USB2.0_16P', None): ('C165948', 'TYPE-C-31-M-12'),
    ('Connector:Micro_SD_Card_Det_Hirose_DM3AT', None): ('C114218', 'DM3AT-SF-PEJM5'),
    ('rp569:CH569W', 'CH569W'): ('', 'CH569W'),                         # not at JLCPCB: consign
    ('Power_Management:LM66100DCK', 'LM66100'): ('', 'LM66100DCKR'),    # not at JLCPCB: consign
    ('Connector:USB3_B_Micro', None): ('', 'GSB343K33HR'),              # not at JLCPCB: consign
    # 2.54 mm headers (THT): assembled by JLCPCB only if THT assembly is ordered
    ('Connector_Generic:Conn_01x02', None): ('C492401', 'PZ254V-11-02P'),
    ('Connector_Generic:Conn_01x03', None): ('C49257', '2.54-1*3P'),
    ('Connector_Generic:Conn_01x04', None): ('C492403', 'PZ254V-11-04P'),
    ('Connector_Generic:Conn_01x10', None): ('C492409', 'PZ254V-11-10P'),
    ('Connector_Generic:Conn_02x07_Odd_Even', None): ('C492424', 'PZ254V-12-14P'),
}
for _p in parts:
    if _p['ref'][0] in '#H':
        continue
    _k = (_p['lib_id'], _p['value'])
    _p['lcsc'], _p['mpn'] = LCSC_BY_VALUE.get(_k) or LCSC_BY_VALUE[(_p['lib_id'], None)]
