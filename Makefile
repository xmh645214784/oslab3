QEMU = qemu-system-i386

os.img:
	@cd bootloader; make
	@cd kernel; make
	@cd app; make
	objdump -d kernel/kMain.elf > kernel/kernel.asm
	objdump -d app/uMain.elf 	> app/uMain.asm
	cat bootloader/bootloader.bin kernel/kMain.elf app/uMain.elf > os.img

play:os.img 
	$(QEMU) -serial stdio os.img

debug: os.img
	$(QEMU) -serial stdio -s -S os.img

clean:
	@cd bootloader; make clean
	@cd kernel; make clean
	@cd app; make clean
	rm -f os.img
