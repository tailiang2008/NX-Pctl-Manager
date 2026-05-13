# Thin shim over CMake — the real build is in CMakeLists.txt.
# This keeps the existing `make` / `./run.sh` interfaces working on the
# borealis-ui branch (v3.0.0+); the old devkitPro template Makefile is in
# the v2.0.0 tag if you ever need it.

TARGET := nx_pctl_manager
BUILD  := build

# Always pass PCTL_PROBE explicitly so toggling `PROBE=1` ↔ no-PROBE between
# builds correctly updates the CMake cache (cmake is a no-op when nothing
# changed, so re-running it every time is cheap).
CMAKE_FLAGS := -DPLATFORM_SWITCH=ON
ifneq ($(strip $(PROBE)),)
	CMAKE_FLAGS += -DPCTL_PROBE=ON
else
	CMAKE_FLAGS += -DPCTL_PROBE=OFF
endif

.PHONY: all clean dist nxlink

all:
	@cmake -B $(BUILD) -S . $(CMAKE_FLAGS)
	@cmake --build $(BUILD) --target $(TARGET).nro
	@cp $(BUILD)/$(TARGET).nro  $(TARGET).nro
	@cp $(BUILD)/$(TARGET).nacp $(TARGET).nacp

clean:
	@echo clean ...
	@rm -rf $(BUILD) out $(TARGET).zip $(TARGET).nro $(TARGET).nacp $(TARGET).elf

dist: all
	@echo making dist ...
	@rm -rf out/
	@rm -f $(TARGET).zip
	@mkdir -p out/switch
	@cp $(BUILD)/$(TARGET).nro out/switch/
	@cd out && zip -r ../$(TARGET).zip ./*

nxlink: all
	nxlink $(BUILD)/$(TARGET).nro
