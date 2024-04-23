import serial
import uinput

ser = serial.Serial('/dev/ttyACM0', 115200)

# Create new device with both mouse and keyboard events
device = uinput.Device([
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT,
    uinput.REL_X,
    uinput.REL_Y,
    uinput.KEY_W,
    uinput.KEY_A,
    uinput.KEY_S,
    uinput.KEY_D,
    uinput.KEY_SPACE,
    uinput.KEY_UP,
    uinput.KEY_DOWN,
    uinput.KEY_LEFT,
    uinput.KEY_RIGHT
])

def parse_data(data):
    axis = data[0]  # 0 for X, 1 for Y, 2 for keys
    value = int.from_bytes(data[1:3], byteorder='big', signed=True)
    print(f"Received data: {data}")
    print(f"axis: {axis}, value: {value}")
    return axis, value

def move_mouse(axis, value):
    if axis == 0:    # X-axis
        device.emit(uinput.REL_X, value)
    elif axis == 1:  # Y-axis
        device.emit(uinput.REL_Y, value)

def handle_key_event(axis,value):
    # Define key mapping based on value
    key_map = {
        1: uinput.KEY_W,
        2: uinput.KEY_A,
        3: uinput.KEY_S,
        4: uinput.KEY_D,
        5: uinput.KEY_SPACE,
        6: uinput.KEY_UP,
        7: uinput.KEY_DOWN,
        8: uinput.KEY_LEFT,
        9: uinput.KEY_RIGHT,
        10: uinput.BTN_LEFT,   # Mouse left click
        11: uinput.BTN_RIGHT   # Mouse right click
    }
    key = key_map.get(abs(value))
    if key:
        device.emit(key, axis>2)
        

try:
    # Sync package
    while True:
        print('Waiting for sync package...')
        while True:
            data = ser.read(1)
            if data == b'\xff':
                break

        # Read 4 bytes from UART
        data = ser.read(3)
        axis, value = parse_data(data)
        if axis < 2:
            move_mouse(axis, value)
        else:
            handle_key_event(axis,value)

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
