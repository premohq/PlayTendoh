#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Apply the ECAM handoff overlay to a disposable pinned donor worktree (AI-assisted)."""
import argparse
from pathlib import Path
import shutil


def replace(path, before, after):
    text = path.read_text()
    if after in text:
        return
    if text.count(before) != 1:
        raise ValueError(f'{path}: expected one integration anchor, found {text.count(before)}')
    path.write_text(text.replace(before, after))


def prepare(source, kind, project):
    if kind in ('gbc', 'gba'):
        folder = source / kind / 'src'
        shutil.copyfile(project / 'port/retro/runtime/ecam_bridge.h', folder / 'ecam_bridge.h')
        main = folder / 'main_shelf.cpp'
        replace(main, '#include "fe_ps5.h"', '#include "fe_ps5.h"\n#include "ecam_bridge.h"')
        replace(main, 'int main() {', 'int main(int argc, char** argv) {')
        # Persistent request files need filesystem access before they can be consumed.
        replace(main, 'const auto access=elevation::request(elevation::Capability::filesystem);',
                f'const auto access=elevation::request(elevation::Capability::filesystem);\n    ecam_bridge::Init(argc, argv, "{kind}");')
        replace(main, 'sceSystemServiceLoadExec("exit",nullptr)', 'sceSystemServiceLoadExec(ecam_bridge::ReturnProgram(),nullptr)')
        frontend = source / 'ps5/frontend/fe_ps5.cpp'
        anchor = f'{kind}::ShelfRuntime nes_game(paths.top_dir,fonts);'
        replace(frontend, anchor, anchor + '\n    const char* ecam_rom = std::getenv("PS5ECAM_ROM");\n    const bool ecam_forwarded = ecam_rom && *ecam_rom;\n    if (ecam_forwarded) { if (audio) audio->Stop(); ok = nes_game.Start(ecam_rom, renderer); if (!ok) std::printf("[ecam] Game start failed: %s\\n", nes_game.error.c_str()); }')
        replace(frontend, 'orbis_web_now_playing("");', 'orbis_web_now_playing("");\n                if (ecam_forwarded) break;')
        # The standalone binary helper targets a different title. ECAM must supply filesystem access.
        build = source / kind / 'tools/build_shelf.py'
        text = build.read_text()
        marker = "baseline=os.environ.get('PS5" + kind.upper() + "_LAPY_BASELINE')"
        if marker in text:
            start = text.index(marker)
            stop = text.index("licenses=app/'licenses'", start)
            text = text[:start] + '# ECAM bundle: no standalone-title Lapy binary.\n' + text[stop:]
        text = text.replace("(out/'filesystem-helper/Lapy-MIT.txt','Lapy-MIT.txt'),", '')
        build.write_text(text)
        # UI assets stay embedded and are extracted to the donor's writable native data folder.
        return
    if kind in ('mupen64plus', 'fceumm'):
        folder = source / 'ps5/coreorbis'
        shutil.copyfile(project / 'port/retro/runtime/ecam_bridge.h', folder / 'ecam_bridge.h')
        main = folder / 'main-boot.cpp'
        replace(main, '#include "OrbisPaths.h"', '#include "OrbisPaths.h"\n#include "ecam_bridge.h"')
        replace(main, 'int main(int argc, char** argv)\n{', f'int main(int argc, char** argv)\n{{\n    ecam_bridge::Init(argc, argv, "{kind}");')
        replace(main, 'sceSystemServiceLoadExec("exit", nullptr)', 'sceSystemServiceLoadExec(ecam_bridge::ReturnProgram(), nullptr)')
        # Existing native teardown runs after one forwarded game, including a failed boot.
        replace(main, 'if (!again)\n', 'if (!again || (ecam_bridge::request.valid && !ecam_bridge::request.library))\n')
        if kind == 'mupen64plus':
            replace(main, 'std::string rom = (argc > 1 && argv[1] && OrbisIsFile(argv[1])) ? argv[1] : "";',
                    'std::string rom = ecam_bridge::request.valid ? ecam_bridge::request.rom : ((argc > 1 && argv[1] && OrbisIsFile(argv[1])) ? argv[1] : "");')
            # Bundled runtimes are updated with ECAM only, never through a standalone installer.
            update = source / 'ps5/frontend/fe_update.cpp'
            replace(update, 'UpdateOffer CheckForUpdate()\n{', 'UpdateOffer CheckForUpdate()\n{\n    return {}; // ECAM bundle: standalone self-updater disabled.\n')
            # RADV's dladdr brings in the SDK libc's dlfcn wrappers, whose optional hooks nothing on the
            # console provides: as null they stay out of the imports, as tools/link.sh links the app.
            replace(source / 'ps5/Makefile', '\t\t--as-needed $(APP_STUBS) -L $(SDK_LIB) \\\n',
                    '\t\t--defsym=__dlopen=0 --defsym=__dlsym=0 --defsym=__dladdr=0 --defsym=__dlclose=0 --defsym=__dlerror=0 \\\n'
                    '\t\t--as-needed $(APP_STUBS) -L $(SDK_LIB) \\\n')
        else:
            replace(main, 'const bool from_command_line = argc > 1 && argv[1] && OrbisIsFile(argv[1]);',
                    'const bool from_command_line = (ecam_bridge::request.valid && !ecam_bridge::request.library) || (argc > 1 && argv[1] && OrbisIsFile(argv[1]));')
            replace(main, 'std::string rom = from_command_line ? argv[1] : "";',
                    'std::string rom = ecam_bridge::request.valid ? ecam_bridge::request.rom : (from_command_line ? argv[1] : "");')
        # Dedicated ECAM helper ports avoid colliding with running standalone helpers.
        helper = folder / 'orbis-shims/ProsperoJailbreak.h'
        port = 9064 if kind == 'mupen64plus' else 9085
        replace(helper, f'constexpr int kHelperPort = {port};', f'constexpr int kHelperPort = {9136 if kind == "mupen64plus" else 9137}; // ECAM bundle')
        # A debug build (PLAYTENDOH_DEBUG): PS5_DevLoop's grab of the frame shown, taken from the surface the
        # frontend draws into (menus over the game included) as it is presented, before it is tiled for the
        # display (A8B8G8R8: its bytes are RGBA). No Vulkan here: this checkout's grab.h without it.
        shutil.copyfile(project / 'port/devloop/grab.h', folder / 'orbis-shims/devloop_grab.h')
        video = folder / 'orbis-shims/ProsperoVideo.cpp'
        replace(video, '#include "ProsperoVideo.h"\n', '#include "ProsperoVideo.h"\n#ifdef PLAYTENDOH_DEBUG\n#define DEVLOOP_NO_VULKAN\n#include "devloop_grab.h"\n#endif\n')
        replace(video, 'void Present(int x, int y, int w, int h, bool wait_vsync)\n{\n\tif (!g.ok)\n\t\treturn;\n',
                'void Present(int x, int y, int w, int h, bool wait_vsync)\n{\n\tif (!g.ok)\n\t\treturn;\n'
                '#ifdef PLAYTENDOH_DEBUG\n\tstatic uint64_t devloop_calls = 0; // ECAM bundle: PS5_DevLoop\'s grab, a debug build\n'
                '\tif (devloop::Asked("/app0", devloop_calls))\n'
                '\t\tdevloop::SaveRgba("/app0", reinterpret_cast<const uint8_t*>(g.surface), kWidth, kHeight, false, {});\n'
                '#endif\n')
        makefile = source / 'ps5/Makefile'
        if 'PLAYTENDOH_DEBUG' not in makefile.read_text():
            makefile.write_text(makefile.read_text() + "\n# ECAM bundle: a debug build (PLAYTENDOH_DEBUG=ON) answers PS5_DevLoop's grab\n"
                                "ifeq ($(PLAYTENDOH_DEBUG),ON)\nDEFS += -DPLAYTENDOH_DEBUG=1\nendif\n")
        return
    if kind == 'porpoise':
        folder = source / 'src'
        shutil.copyfile(project / 'port/retro/runtime/ecam_bridge.h', folder / 'ecam_bridge.h')
        main = folder / 'porpoise_main.cpp'
        replace(main, '#include "porpoise_core.hpp"', '#include "porpoise_core.hpp"\n#include "ecam_bridge.h"')
        replace(main, 'int main(int argc, char **argv)\n{', 'int main(int argc, char **argv)\n{\n    ecam_bridge::Init(argc, argv, "porpoise");')
        replace(main, '[[noreturn]] void leave(int status)\n{', '[[noreturn]] void leave(int status)\n{\n    const char* ecam_destination = ecam_bridge::ReturnProgram();')
        replace(main, '[[noreturn]] void restart_porpoise()\n{', '[[noreturn]] void restart_porpoise()\n{\n    if (ecam_bridge::request.valid) leave(0);')
        # The bridge augments Porpoise's existing forwarding protocol, including the argc=1 case.
        replace(main, 'const porpoise::forward::Args forwarded = porpoise::forward::parse(argc, argv);',
                'porpoise::forward::Args forwarded = porpoise::forward::parse(argc, argv);\n    if (ecam_bridge::request.valid && !ecam_bridge::request.library) { forwarded.rom = ecam_bridge::request.rom; forwarded.exit_after_game = true; }')
        replace(main, 'const int result = sceSystemServiceLoadExec("exit", nullptr);',
                'const int result = sceSystemServiceLoadExec(ecam_destination, nullptr);')
        # Return after a failed forwarded boot too, rather than trapping ECAM users in another library.
        replace(main, 'if (exit == porpoise::core::Exit::Home)', 'if (ecam_bridge::request.valid && exit == porpoise::core::Exit::Failed) leave(2);\n        if (exit == porpoise::core::Exit::Home)')
        replace(main, '    g_app.init(&g_gfx, &g_library, &g_settings, g_settings_path, g_options_path, g_saves_path);',
                '    if (ecam_bridge::request.valid && !ecam_bridge::request.library && !forwarded_game) leave(2);\n    g_app.init(&g_gfx, &g_library, &g_settings, g_settings_path, g_options_path, g_saves_path);')
        replace(folder / 'porpoise_paths.hpp', '#define PORPOISE_APP "/app0"', '#define PORPOISE_APP "/app0/retro/porpoise"')
        # A debug build (PLAYTENDOH_DEBUG, given to Porpoise's build): PS5_DevLoop's grab of the frame shown,
        # at Porpoise's one present, where its in-game menu is drawn over the game in the same pass.
        (folder / 'devloop').mkdir(exist_ok=True)
        shutil.copyfile(project / 'port/devloop/grab.h', folder / 'devloop/grab.h')
        vk = folder / 'porpoise_vk.cpp'
        replace(vk, '#include "porpoise_vk.hpp"\n', '#include "porpoise_vk.hpp"\n#ifdef PLAYTENDOH_DEBUG\n#include "devloop/grab.h" // ECAM bundle: PS5_DevLoop\'s grab\n#endif\n')
        replace(vk, '\nState s;\n', '\nState s;\n#ifdef PLAYTENDOH_DEBUG\ndevloop::Grab devloop_grab;\n#endif\n')
        replace(vk, '    if (!info.imageUsage)\n        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;\n',
                '    if (!info.imageUsage)\n        info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;\n'
                '#ifdef PLAYTENDOH_DEBUG\n    info.imageUsage = devloop::SwapchainUsage(info.imageUsage, caps.supportedUsageFlags);\n#endif\n')
        replace(vk, '    CHECK(vkGetSwapchainImagesKHR(s.device, s.swapchain, &s.image_count, s.images), "images");\n',
                '    CHECK(vkGetSwapchainImagesKHR(s.device, s.swapchain, &s.image_count, s.images), "images");\n'
                '#ifdef PLAYTENDOH_DEBUG\n    {\n        VkPhysicalDeviceMemoryProperties memory{};\n'
                '        vkGetPhysicalDeviceMemoryProperties(s.gpu, &memory);\n'
                '        if (devloop_grab.Init(s.device, vkGetDeviceProcAddr, memory, s.queue_family, "/app0",\n'
                '                              [](const std::string& line) { ps5::debug::mark(line.c_str()); }))\n'
                '            devloop_grab.SetSwapchain(s.swapchain, std::vector<VkImage>(s.images, s.images + s.image_count),\n'
                '                                      s.format, s.extent, info.imageUsage);\n    }\n#endif\n')
        replace(vk, '        (void)vkQueuePresentKHR(s.queue, &present_info);',
                '#ifdef PLAYTENDOH_DEBUG\n        (void)devloop_grab.QueuePresent(s.queue, present_info, vkQueuePresentKHR);\n#else\n'
                '        (void)vkQueuePresentKHR(s.queue, &present_info);\n#endif')
        replace(vk, '    if (s.device_is_ours)\n        vkDestroyDevice(s.device, nullptr);',
                '#ifdef PLAYTENDOH_DEBUG\n    devloop_grab.Destroy();\n#endif\n    if (s.device_is_ours)\n        vkDestroyDevice(s.device, nullptr);')
        replace(source / 'tools/build-porpoise.sh', 'APP_DEFINITIONS="PS5_RETROARCH_RADV PORPOISE" \\\n',
                'APP_DEFINITIONS="PS5_RETROARCH_RADV PORPOISE$([[ ${PLAYTENDOH_DEBUG:-OFF} == ON ]] && echo " PLAYTENDOH_DEBUG")" \\\n')
    else:
        folder = source / kind / 'src'
        shutil.copyfile(project / 'port/retro/runtime/ecam_bridge.h', folder / 'ecam_bridge.h')
        main = folder / 'main_shelf.cpp'
        replace(main, '#include "fe_ps5.h"', '#include "fe_ps5.h"\n#include "ecam_bridge.h"')
        replace(main, 'int main() {', f'int main(int argc, char** argv) {{\n    ecam_bridge::Init(argc, argv, "{kind}");')
        before = 'sceSystemServiceLoadExec("exit",nullptr)' if kind == 'nes' else 'sceSystemServiceLoadExec("exit",nullptr)'
        replace(main, before, 'sceSystemServiceLoadExec(ecam_bridge::request.valid ? ecam_bridge::request.returnProgram.c_str() : "exit",nullptr)')
        frontend = source / 'ps5/frontend/fe_ps5.cpp'
        anchor = f'{kind}::ShelfRuntime nes_game(paths.top_dir);'
        replace(frontend, anchor, anchor + '\n    const char* ecam_rom = std::getenv("PS5ECAM_ROM");\n    const bool ecam_forwarded = ecam_rom && *ecam_rom;\n    if (ecam_forwarded) { if (audio) audio->Stop(); ok = nes_game.Start(ecam_rom, renderer); }')
        replace(frontend, 'orbis_web_now_playing("");', 'orbis_web_now_playing("");\n                if (ecam_forwarded) break;')
        # The standalone's old signed helper targets another title. ECAM supplies its
        # own elevation path; do not require or ship another title's opaque helper.
        build = source / kind / 'tools/build_shelf.py'
        text = build.read_text()
        marker = "baseline=Path(os.environ['PS5NES_LAPY_BASELINE'])" if kind == 'nes' else "baseline=os.environ.get('PS5N64_LAPY_BASELINE')"
        if marker in text:
            start = text.index(marker)
            stop = text.index("licenses=app/'licenses'", start)
            text = text[:start] + '# ECAM runtime: no title-specific standalone Lapy helper.\n' + text[stop:]
            build.write_text(text)
        # Donor resources remain embedded; no external standalone installation is required.
        replace(frontend, 'original_ui.asset_dir=paths.top_dir+"/porpoise-ui";', f'original_ui.asset_dir="/app0/retro/{kind}/porpoise-ui";')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('kind', choices=['porpoise', 'n64', 'nes', 'mupen64plus', 'fceumm', 'gbc', 'gba'])
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    prepare(args.source.resolve(), args.kind, Path(__file__).resolve().parents[2])
