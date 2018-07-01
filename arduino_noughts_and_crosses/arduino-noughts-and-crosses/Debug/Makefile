################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL := cmd.exe
RM := rm -rf

USER_OBJS :=

LIBS := 
PROJ := 

O_SRCS := 
C_SRCS := 
S_SRCS := 
S_UPPER_SRCS := 
OBJ_SRCS := 
ASM_SRCS := 
PREPROCESSING_SRCS := 
OBJS := 
OBJS_AS_ARGS := 
C_DEPS := 
C_DEPS_AS_ARGS := 
EXECUTABLES := 
OUTPUT_FILE_PATH :=
OUTPUT_FILE_PATH_AS_ARGS :=
AVR_APP_PATH :=$$$AVR_APP_PATH$$$
QUOTE := "
ADDITIONAL_DEPENDENCIES:=
OUTPUT_FILE_DEP:=
LIB_DEP:=
LINKER_SCRIPT_DEP:=

# Every subdirectory with source files must be described here
SUBDIRS :=  \
../linker_scripts/ \
../linker_scripts/linker_scripts/ \
../linker_scripts/linker_scripts/gcc/


# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS +=  \
../Sketch.cpp


PREPROCESSING_SRCS += 


ASM_SRCS += 


OBJS +=  \
Sketch.o

OBJS_AS_ARGS +=  \
Sketch.o

C_DEPS +=  \
Sketch.d

C_DEPS_AS_ARGS +=  \
Sketch.d

OUTPUT_FILE_PATH +=arduino-noughts-and-crosses.elf

OUTPUT_FILE_PATH_AS_ARGS +=arduino-noughts-and-crosses.elf

ADDITIONAL_DEPENDENCIES:=

OUTPUT_FILE_DEP:= ./makedep.mk

LIB_DEP+=  \
C:/Users/Sam/Desktop/Arduino-Noughts-and-Crosses/arduino_noughts_and_crosses/ArduinoCore/Debug/libArduinoCore.a

LINKER_SCRIPT_DEP+=  \
../linker_scripts/linker_scripts/gcc/flash_without_bootloader.ld \
../linker_scripts/linker_scripts/gcc/flash_with_bootloader.ld


# AVR32/GNU C Compiler
./Sketch.o: .././Sketch.cpp
	@echo Building file: $<
	@echo Invoking: ARM/GNU C Compiler : 6.3.1
	$(QUOTE)C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-g++.exe$(QUOTE) -mthumb -D__SAMD21G18A__ -DDEBUG -DF_CPU=48000000L -DARDUINO=10805 -DARDUINO_SAM_ZERO -DARDUINO_ARCH_SAM -DUSB_VID=0x2a03 -DUSB_PID=0x804f -DUSB_PRODUCT="\"Arduino M0 Pro\"" -DUSB_MANUFACTURER="\"Arduino LLC\"" -DUSBCON -DSAMD_SERIES  -I"C:\Program Files (x86)\Atmel\Studio\7.0\Packs\arm\cmsis\5.0.1\CMSIS\Include" -I"..\\..\ArduinoCore\include\core" -I"..\\..\ArduinoCore\include\variants" -I"..\\..\ArduinoCore\include\core\avr" -I"..\\..\ArduinoCore\include\core\samd21" -I"..\\..\ArduinoCore\include\core\USB" -I"..\\..\ArduinoCore\include\core\samd21\include" -I"..\\..\ArduinoCore\include\core\samd21\include\component" -I"..\\..\ArduinoCore\include\core\samd21\include\instance" -I"..\\..\ArduinoCore\include\core\samd21\include\pio" -I"..\\..\ArduinoCore\include\libraries\adafruit_ft6206_library" -I"..\\..\ArduinoCore\include\libraries\adafruit_gfx_library" -I"..\\..\ArduinoCore\include\libraries\adafruit_ili9341" -I"..\\..\ArduinoCore\include\libraries\SD" -I"..\\..\ArduinoCore\include\libraries\spi" -I"..\\..\ArduinoCore\include\libraries\wire" -I"..\\..\ArduinoCore\include\libraries\adafruit_gfx_library\Fonts" -I"..\\..\ArduinoCore\include\libraries\adafruit_ili9341\examples\pictureEmbed" -I"..\\..\ArduinoCore\include\libraries\SD\utility"  -Os -fno-threadsafe-statics -fno-exceptions -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -g3 -w -mcpu=cortex-m0plus -c -std=gnu++11 --param max-inline-insns-single=500 -nostdlib -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"   -o "$@" "$<" 
	@echo Finished building: $<
	




# AVR32/GNU Preprocessing Assembler



# AVR32/GNU Assembler




ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: $(OUTPUT_FILE_PATH) $(ADDITIONAL_DEPENDENCIES)

$(OUTPUT_FILE_PATH): $(OBJS) $(USER_OBJS) $(OUTPUT_FILE_DEP) $(LIB_DEP) $(LINKER_SCRIPT_DEP)
	@echo Building target: $@
	@echo Invoking: ARM/GNU Linker : 6.3.1
	$(QUOTE)C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-g++.exe$(QUOTE) -o$(OUTPUT_FILE_PATH_AS_ARGS) $(OBJS_AS_ARGS) $(USER_OBJS) $(LIBS) -mthumb -Wl,-Map="arduino-noughts-and-crosses.map" --specs=nano.specs --specs=nosys.specs -Wl,--start-group -lm -lArduinoCore  -Wl,--end-group -L"..\linker_scripts\linker_scripts\gcc" -L"C:\Users\Sam\Desktop\Arduino-Noughts-and-Crosses\arduino_noughts_and_crosses\ArduinoCore\Debug"  -Wl,--gc-sections -mcpu=cortex-m0plus -Tflash_without_bootloader.ld -Wl,--cref -Os -Wl,--check-sections -Wl,--unresolved-symbols=report-all -Wl,--warn-common -Wl,--warn-section-align  
	@echo Finished building target: $@
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objcopy.exe" -O binary "arduino-noughts-and-crosses.elf" "arduino-noughts-and-crosses.bin"
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objcopy.exe" -O ihex -R .eeprom -R .fuse -R .lock -R .signature  "arduino-noughts-and-crosses.elf" "arduino-noughts-and-crosses.hex"
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objcopy.exe" -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O binary "arduino-noughts-and-crosses.elf" "arduino-noughts-and-crosses.eep" || exit 0
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objdump.exe" -h -S "arduino-noughts-and-crosses.elf" > "arduino-noughts-and-crosses.lss"
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-objcopy.exe" -O srec -R .eeprom -R .fuse -R .lock -R .signature  "arduino-noughts-and-crosses.elf" "arduino-noughts-and-crosses.srec"
	"C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\arm\arm-gnu-toolchain\bin\arm-none-eabi-size.exe" "arduino-noughts-and-crosses.elf"
	
	





# Other Targets
clean:
	-$(RM) $(OBJS_AS_ARGS) $(EXECUTABLES)  
	-$(RM) $(C_DEPS_AS_ARGS)   
	rm -rf "arduino-noughts-and-crosses.elf" "arduino-noughts-and-crosses.a" "arduino-noughts-and-crosses.hex" "arduino-noughts-and-crosses.bin" "arduino-noughts-and-crosses.lss" "arduino-noughts-and-crosses.eep" "arduino-noughts-and-crosses.map" "arduino-noughts-and-crosses.srec"
	