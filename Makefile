# SPDX-License-Identifier: GPL-3.0-or-later
# PS5-ECAM build. `make help` lists the targets. Missing dependencies are fetched at their pinned
# revisions (tools/deps.json); nothing that already exists is changed. The build runs on Linux with
# clang-18, lld-18 and the LLVM 18 tools, cmake, ninja, git, python3 and make.

SHELL := /bin/bash
.SHELLFLAGS := -eu -o pipefail -c
.DEFAULT_GOAL := help
MAKEFLAGS += --no-print-directory

# The release's version, set here only: port/CMakeLists.txt and tools/package.sh (param.json) read it
VERSION := 6.0.0
APP := build/app/PPSA99360
JOBS ?= $(shell nproc)
export JOBS

# The emulators a build takes (make release CORES=eden,mgba): their names, or all (the default), none (the
# launcher and the Wii U alone: Cemu is the app's own program), libretro (its cores) and optional (the
# programs a release leaves out). Comma or space separated. The app says on a side whose emulator a build
# left out that it is not in this copy (port/CMakeLists.txt, port/libretro/cores.cpp, tools/package.sh).
CORES ?= all
CORES_LIBRETRO := mesen mesens mgba paralleln64 beetlevb pokemini gw
CORES_ALL := azahar melonds eden eglantine porpoise $(CORES_LIBRETRO)
CORES_OPTIONAL := mupen64plus fceumm esde
comma := ,
cores_named := $(subst $(comma), ,$(CORES))
CORES_LIST := $(sort $(if $(filter all,$(cores_named)),$(CORES_ALL)) $(if $(filter libretro,$(cores_named)),$(CORES_LIBRETRO)) \
	$(if $(filter optional,$(cores_named)),$(CORES_OPTIONAL)) $(filter-out all none libretro optional,$(cores_named)))
cores_unknown := $(filter-out $(CORES_ALL) $(CORES_OPTIONAL),$(CORES_LIST))
$(if $(cores_unknown),$(error CORES: no emulator named $(cores_unknown); the names are $(CORES_ALL) $(CORES_OPTIONAL), or all, none, libretro, optional))
# tools/env.sh's ps5cemu_core_wanted reads it
export PS5CEMU_CORES := $(CORES_LIST)
core = $(filter $(1),$(CORES_LIST))

# A debug build (make release DEBUG=1): what helps develop and test the app on a console, kept out of a
# release (port/CMakeLists.txt's PLAYTENDOH_DEBUG). Its ZIP and ELF are named -debug.
DEBUG ?= 0
export PLAYTENDOH_DEBUG := $(if $(filter 1 on ON yes,$(DEBUG)),ON,OFF)
DIST_NAME := v$(VERSION)$(if $(filter ON,$(PLAYTENDOH_DEBUG)),-debug)

.PHONY: mupen64plus fceumm gameboy retro-upgrades retro test-retro help deps deps-status radv azahar melonds libretro eden eglantine porpoise build check package release clean distclean test-contribution esde esde-zip

test-retro: ## Host verification for the additional system integration
	bash tools/tests/test-retro.sh
	python3 -B tools/tests/test-retro-tools.py

test-contribution: ## Host checks for standalone integrations, transfers, Slot-2 and capture (no console)
	python3 -B tools/tests/test-performance.py
	python3 -B tools/tests/test-performance-suite.py
	bash tools/tests/test-eden-extras.sh
	bash tools/tests/test-profile-management.sh
	bash tools/tests/test-save-transfer.sh
	bash tools/tests/test-saves.sh
	bash tools/tests/test-slot2.sh
	bash tools/tests/test-microphone.sh
	bash tools/tests/test-frame-capture.sh

help: ## List the targets
	@echo 'PS5-ECAM build - make [target] [VARIABLE=value]'
	@echo
	@awk 'BEGIN { FS = ":.*## " } /^[a-z][a-z-]*:.*## / { printf "  %-12s %s\n", $$1, $$2 }' $(MAKEFILE_LIST)
	@echo
	@echo 'Variables:'
	@echo '  JOBS=$(JOBS)                parallel compile jobs'
	@echo '  RADV_PATCHES=0006,0007,0008,0009,0010'
	@echo '                              the driver patches (patches/mesa) RADV is built with: their numbers,'
	@echo '                              all (0001-0007), or empty for none (0006: 120 Hz output after the new'
	@echo '                              launcher; 0007: no GPU fault on a buffer freed while the GPU still'
	@echo '                              uses it; 0008: submissions on a thread of their own, asyncSubmit;'
	@echo '                              0009: the GPU waits sleep gpuWaitSleepUs; 0010: 120 Hz kept while'
	@echo '                              the display settles in it)'
	@echo '  RADV_ARCHIVE, RADV_SDK      a RADV built elsewhere, and the SDK fork it was built with'
	@echo '  CORES=$(CORES)'
	@echo '                              the emulators a build takes: all, none (the launcher and the Wii U),'
	@echo '                              libretro, optional, or names: $(CORES_ALL)'
	@echo '                              $(CORES_OPTIONAL) (CORES=eden,mgba; CORES=all,optional)'
	@echo '  DEBUG=$(DEBUG)                     1: a debug build (PS5_DevLoop'"'"'s grab of the launcher'"'"'s frame), dist/*-debug'

deps: ## Fetch the pinned inputs and build the libraries Cemu needs for the PS5
	python3 -B tools/deps.py fetch
	bash tools/build-deps.sh

deps-status: ## Show every dependency and whether it matches its pin
	@python3 -B tools/deps.py status

radv: deps ## Build RADV, the Vulkan driver, with PS5_Vulkan's recipe and patches/mesa (RADV_PATCHES)
	bash tools/build-radv.sh

azahar: deps ## Build Azahar's core (the 3DS side) and its PS5 frontend: build/azahar
	bash tools/build-azahar.sh

melonds: deps ## Build melonDS's core (the DS side's) and its PS5 frontend: build/melonds
	bash tools/build-melonds.sh

eden: deps ## Build ProsperoEdenX, the Switch side's program (Eden), with patches/eden: build/eden
	bash tools/build-eden.sh

eglantine: deps ## Build EglantinePS5 as the Switch 2 child runtime, with patches/eglantine: build/eglantine/app
	bash tools/build-eglantine.sh

porpoise: deps radv ## Build Porpoise X, the GameCube and Wii sides' program (Dolphin), at its pin: build/retro/porpoise
	bash tools/build-porpoise.sh

esde: ## Build EmulationStation (ES-DE), the optional other frontend, with PS5-ECAM's glue: build/esde/es-de
	bash tools/build-esde.sh

# ES-DE as the app downloads it (port/app/esde_get.h): its folder under PPSA99360/, for the GitHub release
# tagged as esde_get.h's kTag, asset ES-DE-PS5.zip
esde-zip: esde ## dist/ES-DE-PS5.zip: EmulationStation, the app's download of it
	@mkdir -p dist
	rm -rf build/esde-zip dist/ES-DE-PS5.zip
	mkdir -p build/esde-zip/PPSA99360
	cp -al build/esde/es-de build/esde-zip/PPSA99360/es-de
	cd build/esde-zip && python3 -m zipfile -c ../../dist/ES-DE-PS5.zip PPSA99360
	rm -rf build/esde-zip
	cd dist && sha256sum ES-DE-PS5.zip

libretro: deps ## Build the libretro cores the other systems play on in the app (NES, SNES, Game Boys, N64, Virtual Boy, Pokemon mini; CORES's): build/libretro
	python3 tools/build-libretro.py $(call core,$(CORES_LIBRETRO))

# the emulators linked into the app's program, as CORES names them
linked := $(call core,azahar melonds) $(if $(call core,$(CORES_LIBRETRO)),libretro)

build: deps $(linked) $(if $(RADV_ARCHIVE),,radv) ## Build PS5-ECAM (Cemu, and CORES's Azahar, melonDS and libretro cores) and link it with RADV: build/cemu/ps5cemu.elf
	bash tools/build-cemu.sh

check: deps $(linked) ## Build and package everything with a stand-in for RADV, to check the build (not an app)
	PS5CEMU_LINK_CHECK=1 bash tools/build-cemu.sh
	bash tools/package.sh --check

gameboy: ## Build source-pinned GB/GBC and GBA runtimes (prepared PS5 dependencies required)
	python3 tools/build-retro.py gbc gba

retro-upgrades: ## Build optional N64/NES alternative runtimes (prepared toolchains required)
	python3 tools/build-retro.py mupen64plus fceumm

# One of them for a package (CORES=mupen64plus, or optional): a runtime staged at its pin, in the same mode
# (DEBUG=1 or not: its .debug beside it), is kept, as tools/build-porpoise.sh keeps Porpoise; otherwise it is
# built again, its objects too (make would not see the define change)
mupen64plus fceumm: deps
	@pin=$$(python3 -c 'import json,sys; print(next(s["commit"] for s in json.load(open("tools/retro/sources.json"))["sources"] if s["name"] == sys.argv[1]))' $@); \
	staged=$$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1])).get("commit", ""))' build/retro/$@/ECAM-SOURCE.json 2>/dev/null || true); \
	mode=$${PLAYTENDOH_DEBUG:-OFF}; \
	if [[ $$staged == "$$pin" && $$(cat build/retro/$@.debug 2>/dev/null || echo OFF) == "$$mode" ]]; then \
		echo "==> [$@] build/retro/$@ is at $${pin:0:12}"; \
	else rm -rf build/retro/$@ build/retro/$@.debug build/retro-source/$@/ps5/build && \
		python3 tools/build-retro.py --reuse-prepared $@ && echo "$$mode" >build/retro/$@.debug; fi

retro: ## Build the pinned Porpoise, N64 and NES runtimes (prepared donor toolchains required)
	python3 tools/build-retro.py porpoise n64 nes

package: build $(call core,eden eglantine porpoise mupen64plus fceumm esde) ## The app folder: build/app/PPSA99360 (with CORES's Eden, Eglantine, Porpoise...)
	bash tools/package.sh

# a release takes the emulators' patches only as they are: a checkout in .deps with commits not exported
# or changes not committed stops it (ps5cemu_patches_apply in tools/env.sh)
release: export PS5CEMU_RELEASE := 1
release: package ## dist/PS5-ECAM-vVERSION.zip (the PPSA99360 folder), SHA256SUMS, and the ELF with its symbols
	@mkdir -p dist
	rm -f dist/PS5-ECAM-$(DIST_NAME).zip
	cd build/app && python3 -m zipfile -c ../../dist/PS5-ECAM-$(DIST_NAME).zip PPSA99360
	cd dist && sha256sum PS5-ECAM-$(DIST_NAME).zip > SHA256SUMS
	@# the unstripped ELF of the eboot, kept to look up this release's crash reports
	@# (tools/symbolize-crash.py dist/ps5cemu-vVERSION.elf boot.log)
	cp build/cemu/ps5cemu.elf dist/ps5cemu-$(DIST_NAME).elf
	@# and the Switch side's program's (ProsperoEdenX's tools/symbolize-crash.py)
	if [[ -f build/eden/eden.elf ]]; then cp build/eden/eden.elf dist/eden-$(DIST_NAME).elf; fi
	@# and the Switch 2's games' program's (EglantinePS5's tools/symbolize-crash.py)
	if [[ -f build/eglantine/eglantine.elf ]]; then cp build/eglantine/eglantine.elf dist/eglantine-$(DIST_NAME).elf; fi
	@cat dist/SHA256SUMS

clean: ## Remove build outputs (build/, dist/); dependencies stay
	rm -rf build dist

distclean: clean ## Also remove the fetched dependencies in .deps
	python3 -B tools/deps.py clean
