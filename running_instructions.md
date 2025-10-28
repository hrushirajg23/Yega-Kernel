Since the 32bit setup is already done, we just need to set path

1. export PATH="$HOME/opt/cross/bin:$PATH"

2. make

3. For display based qemu-system-i386 -cdrom build/yegaos.iso

    For serial based 

qemu-system-i386 -cdrom build/yegaos.iso -serial stdio

