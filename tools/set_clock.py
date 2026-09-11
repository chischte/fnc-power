"""Set the PLC RTC from this computer's local wall clock over USB."""
import argparse
from datetime import datetime
import time
import serial

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('port', help='Serial port, e.g. COM5')
args = parser.parse_args()
try:
    with serial.Serial(args.port, 115200, timeout=0.5) as port:
        time.sleep(3)  # Opening an AVR USB serial port can reset the controller.
        port.reset_input_buffer()
        command = datetime.now().strftime('TIME %Y-%m-%d %H:%M:%S\n')
        port.write(command.encode('ascii'))
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            line = port.readline().decode('ascii', errors='replace').strip()
            if line:
                print(line)
            if line == 'OK TIME':
                print('RTC set to local PC time: ' + command[5:].strip())
                break
            if line.startswith('ERR'):
                raise SystemExit('RTC rejected time: ' + line)
        else:
            raise SystemExit('No confirmation from firmware; check port and upload.')
except serial.SerialException as error:
    raise SystemExit(f'Serial connection failed: {error}') from error
