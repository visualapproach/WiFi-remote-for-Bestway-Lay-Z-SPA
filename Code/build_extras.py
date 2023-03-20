from datetime import datetime
import re

def set_version(version):
    # Read in the file
    with open('src/config.h', 'r') as file :
        filedata = file.read()
    # Replace version
    filedata = re.sub(r'#define FW_VERSION ".*"', f'#define FW_VERSION "{version}"', filedata)
    # Write the file out again
    with open('src/config.h', 'w') as file:
        file.write(filedata)

now = datetime.now()
set_version(now.strftime("%Y-%m-%d_%H:%M:%S"))