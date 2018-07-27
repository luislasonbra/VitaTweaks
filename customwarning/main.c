/*
  VitaTweaks: Custom Warning
  Copyright (C) 2018, TheFloW

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/io/fcntl.h>
#include <psp2/sysmodule.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

#define MAX_CUSTOM_WARNING_LENGTH 512

static tai_hook_ref_t sceSysmoduleLoadModuleInternalWithArgRef;
static tai_hook_ref_t scePafToplevelGetTextRef;

static SceUID hooks[2];

static wchar_t custom_warning_buf[MAX_CUSTOM_WARNING_LENGTH];
static wchar_t *custom_warning = custom_warning_buf;

static wchar_t *scePafToplevelGetTextPatched(void *a0, void *a1) {
  uint32_t id = *(uint32_t *)(a1 + 0xC);

  if (id == 0x76A1B071 || id == 0x72411882)
    return custom_warning;

  return TAI_CONTINUE(wchar_t *, scePafToplevelGetTextRef, a0, a1);
}

static int sceSysmoduleLoadModuleInternalWithArgPatched(SceUInt32 id, SceSize args, void *argp, void *unk) {
  int res = TAI_CONTINUE(int, sceSysmoduleLoadModuleInternalWithArgRef, id, args, argp, unk);

  if (res >= 0 && id == SCE_SYSMODULE_INTERNAL_PAF) {
    hooks[1] = taiHookFunctionExport(&scePafToplevelGetTextRef, "ScePaf", 0x4D9A9DD0, 0x19CEFDA7,
                                     scePafToplevelGetTextPatched);
  }

  return res;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  SceUID fd = sceIoOpen("ux0:tai/custom_warning.txt", SCE_O_RDONLY, 0);
  if (fd < 0)
    fd = sceIoOpen("ur0:tai/custom_warning.txt", SCE_O_RDONLY, 0);

  if (fd >= 0) {
    sceClibMemset(custom_warning, 0, 2 * MAX_CUSTOM_WARNING_LENGTH);
    sceIoRead(fd, custom_warning, 2 * MAX_CUSTOM_WARNING_LENGTH);
    sceIoClose(fd);

    if (custom_warning[0] == 0xFEFF) {
      custom_warning++;

      hooks[0] = taiHookFunctionImport(&sceSysmoduleLoadModuleInternalWithArgRef, "SceShell", 0x03FCF19D, 0xC3C26339,
                                        sceSysmoduleLoadModuleInternalWithArgPatched);
    }
  }

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hooks[1] >= 0)
    taiHookRelease(hooks[1], scePafToplevelGetTextRef);
  if (hooks[0] >= 0)
    taiHookRelease(hooks[0], sceSysmoduleLoadModuleInternalWithArgRef);

  return SCE_KERNEL_STOP_SUCCESS;
}
