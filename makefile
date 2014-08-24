# Project configuration:

OBJS  = main.o usbdrv/usbdrv.o usbdrv/usbdrvasm.o
BIN   = main

#Hardware
MCU     = atmega8
ROM_START = 0x000
F_CPU   = 12000000

#Compiler flags
CCFLAGS = -g -Os -Wall -Wstrict-prototypes -mmcu=$(MCU) -Iusbdrv -I. -DDEBUG_LEVEL=0 -mcall-prologues -fomit-frame-pointer -DF_CPU=$(F_CPU)

#Linker flags
LDFLAGS = -Wl,-Map=$(BIN).map,--cref,--section-start=.text=$(ROM_START) -DF_CPU=$(F_CPU) -mmcu=$(MCU) -gstabs,-ahlms=$(<:.c=.lst)

# Assembler config
ASTFLAG = -x assembler-with-cpp

# PROTEUS = E:/my/shemes/Proteus7/Designs/led_cube/

# Compiler configuration:
CC      = avr-gcc
LD      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
UPLOAD  = avrdude

# Convert ELF to COFF for use in debugging / simulating in
# AVR Studio or VMLAB.
COFFCONVERT=$(OBJCOPY) --debugging \
	--change-section-address .data-0x800000 \
	--change-section-address .bss-0x800000 \
	--change-section-address .noinit-0x800000 \
	--change-section-address .eeprom-0x810000

#Do all
all: image extcoff
#extcoff

#create ROM-Image
image: $(BIN)
	avr-size $(BIN).elf

#Upload
upload: $(BIN).hex
	$(UPLOAD) -p$(MCU) -cstk500 -P COM3 -e -U flash:w:$(BIN).hex

#asm
asm: $(ASMS)

#clean source directory
.phony:	clean
clean:
	rm -f *.o *.S *.bak *.hex *.cof *.elf *.map *.lst usbdrv/*.o usbdrv/oddebug.S usbdrv/usbdrv.S

#create docs
doc:
	doxygen doxy.dxy

# Build rules:
%o: %c
	$(CC) $(CCFLAGS) -c $< -o $@
	@echo

%o: %S
	$(CC) $(CCFLAGS) $(ASTFLAG) -c $< -o $@
	@echo

%S: %c
	$(CC) $(CCFLAGS) -S $< -o $@
	@echo

$(BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $(BIN).elf $^
	@echo
	$(OBJCOPY) -O ihex -R .eeprom $(BIN).elf $(BIN).hex
	@echo
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma .eeprom=0 -O ihex $(BIN).elf $(BIN)_eeprom.hex
	@echo
	$(OBJDUMP) -t -h -S $(BIN).elf >$(BIN).lst
	@echo
#	@cp main.hex $(PROTEUS)

coff: $(BIN).elf
	$(COFFCONVERT) -O coff-avr $< $(BIN).cof

extcoff: $(BIN).elf
	$(COFFCONVERT) -O coff-ext-avr $< $(BIN).cof
