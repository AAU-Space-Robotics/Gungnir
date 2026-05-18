import can
from canopen_helpers import enable_motor, set_operation_mode, set_target_speed, fault_reset

bus = can.interface.Bus(channel='vcan0', bustype='socketcan', bitrate=1000000)

NODE_ID1 = 0x68  # 104 in decimal
NODE_ID2 = 0x65
tx_id1 = 0x600 + NODE_ID1  # 0x668
tx_id2 = 0x600 + NODE_ID2  # 0x668
rx_id1 = 0x580 + NODE_ID1  # 0x5E8
rx_id2 = 0x580 + NODE_ID2  # 0x5E8

# Enable motor
enable_motor(bus, tx_id1)
enable_motor(bus, tx_id2)
