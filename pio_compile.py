from SCons.Script import DefaultEnvironment
import os
import subprocess

env = DefaultEnvironment()
project_dir = env['PROJECT_DIR']
src_dir = os.path.join(project_dir, "src")

# Compile any .pio files in src/
for file in os.listdir(src_dir):
    if file.endswith(".pio"):
        pio_file = os.path.join(src_dir, file)
        header_file = os.path.join(src_dir, file.replace(".pio", ".pio.h"))
        cmd = f"pioasm -o c-header {pio_file} {header_file}"
        print("Compiling PIO:", cmd)
        subprocess.run(cmd, shell=True)
