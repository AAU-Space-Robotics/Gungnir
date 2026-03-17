import can
import time
import asyncio
import can_motor

# sudo ip link set can0 up type can bitrate 1000000

# My actuator implementation
import myactuator_rmd_py as rmd


async def read_messages(bus):
    """Continuously read and process messages from the CAN buffer."""
    while True:
        message = bus.recv(timeout=0)
        while message:
            #Motor 1
            if message.arbitration_id == j1.rx_id:            
                if message.data[1:3]==b'\x6c\x60': #Speed response
                    j1.speed = int.from_bytes(message.data[4:8], byteorder='little', signed=True)
                    print(f"[{time.time()}] j1 speed: {j1.speed}")
                elif message.data[1:3]==b'\x64\x60': #Position response
                    j1.position = int.from_bytes(message.data[4:8], byteorder='little', signed=True)
                    print(f"[{time.time()}] j1 position: {j1.position}")
            #Motor 2
            if message.arbitration_id == j2.rx_id:
                if message.data[1:3]==b'\x6c\x60': #Speed response
                    j2.speed = int.from_bytes(message.data[4:8], byteorder='little', signed=True)
                    print(f"[{time.time()}] j2 speed: {j2.speed}")
                elif message.data[1:3]==b'\x64\x60': #Position response
                    j2.position = int.from_bytes(message.data[4:8], byteorder='little', signed=True)
                    print(f"[{time.time()}] j  position: {j2.position}")


            message = bus.recv(timeout=0)
        await asyncio.sleep(0.001)

async def send_requests(bus):
    """Periodically send speed requests to both motors."""
    while True:
        j1.read_position(bus)
        j2.read_position(bus)
        j1.read_speed(bus)
        j2.read_speed(bus)
        print(f"[{time.time()}] Sent speed request!")
        await asyncio.sleep(0.01)

async def main(bus):
    await asyncio.gather(send_requests(bus), read_messages(bus))

# Setup CAN bus
bus = can.interface.Bus(channel='can0', interface='socketcan', bitrate=1000000)

driver = rmd.CanDriver("can0")

lol = 1
i = 0

# Initialize motors

j2 = rmd.ActuatorInterface(driver, 2)
print(j2.getVersionDate())
j3 = rmd.ActuatorInterface(driver, 3)
print(j3.getVersionDate())

j2.setCurrentPositionAsEncoderZero()
j3.setCurrentPositionAsEncoderZero()
j2.reset()
j3.reset()


j1 = can_motor.Motor("j1", 101, "small")
j4 = can_motor.Motor("j2", 104, "small")

j1.fault_reset(bus)
j4.fault_reset(bus)
j1.enable_motor(bus)
j4.enable_motor(bus)
j1.set_operation_mode(bus, 1)
j4.set_operation_mode(bus, 1)

j1.set_target_position(bus, 0)
j4.set_target_position(bus, 0)
j2.sendPositionAbsoluteSetpoint(0.0, 200.0)
j3.sendPositionAbsoluteSetpoint(0.0, 200.0)

while lol:
    j1.set_target_position(bus, 10)
    j4.set_target_position(bus, 10)
    j2.sendPositionAbsoluteSetpoint(10.0, 200.0)
    j3.sendPositionAbsoluteSetpoint(10.0, 200.0)

    time.sleep(0.5)

    j1.set_target_position(bus, 0)
    j4.set_target_position(bus, 0)
    j2.sendPositionAbsoluteSetpoint(0.0, 200.0)
    j3.sendPositionAbsoluteSetpoint(0.0, 200.0)

    time.sleep(0.5)
    
    i += 1

    if i > 10:
        lol = 0



# Run both tasks concurrently
asyncio.run(main(bus))