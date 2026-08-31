// xmd - ReXGlue Recompiled Project

#pragma once

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <rex/audio/audio_system.h>
#include <rex/audio/xma/decoder.h>
#include <rex/cvar.h>
#include <rex/graphics/flags.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/input/input_system.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/system/xam/user_profile.h>
#include <rex/system/xio.h>
#include <rex/system/xtypes.h>
#include <rex/system/xthread.h>
#include <rex/ui/keybinds.h>

#include <SDL3/SDL.h>
#include <dbghelp.h>
#include <cstdio>
#include <fstream>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>

#include "version.h"

#define XMD_VERSION_TEXT "X-Men Destiny PC Port v" XMD_VERSION_STRING

static std::atomic<bool> g_minidump_written{false};

static std::filesystem::path GetMinidumpPath() {
  wchar_t path[MAX_PATH] = {};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  std::filesystem::path exe(path);

  // Timestamped dump name so multiple crashes don't overwrite each other
  SYSTEMTIME st;
  GetLocalTime(&st);
  wchar_t buf[64];
  swprintf_s(buf, L"xmd_crash_%04u%02u%02u_%02u%02u%02u.dmp",
             st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
  return exe.parent_path() / buf;
}

static void xmd_write_minidump(PEXCEPTION_POINTERS ep) {
  if (g_minidump_written.exchange(true)) return;

  HMODULE hDbgHelp = LoadLibraryW(L"DbgHelp.dll");
  if (!hDbgHelp) {
    REXCPU_WARN("xmd_write_minidump: failed to load DbgHelp.dll");
    return;
  }

  using MiniDumpWriteDumpFn = BOOL (WINAPI*)(
      HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
      PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
      PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
      PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

  auto pMiniDumpWriteDump = reinterpret_cast<MiniDumpWriteDumpFn>(
      GetProcAddress(hDbgHelp, "MiniDumpWriteDump"));
  if (!pMiniDumpWriteDump) {
    REXCPU_WARN("xmd_write_minidump: MiniDumpWriteDump not found");
    FreeLibrary(hDbgHelp);
    return;
  }

  auto dump_path = GetMinidumpPath();
  HANDLE hFile = CreateFileW(dump_path.wstring().c_str(), GENERIC_WRITE,
                             0, nullptr, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    REXCPU_WARN("xmd_write_minidump: failed to open {}", dump_path.string());
    FreeLibrary(hDbgHelp);
    return;
  }

  MINIDUMP_EXCEPTION_INFORMATION info = {};
  info.ThreadId = GetCurrentThreadId();
  info.ExceptionPointers = ep;
  info.ClientPointers = FALSE;

  BOOL ok = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                               hFile, MiniDumpWithIndirectlyReferencedMemory,
                               &info, nullptr, nullptr);
  CloseHandle(hFile);
  FreeLibrary(hDbgHelp);

  if (ok) {
    REXCPU_WARN("Crash minidump written to: {}", dump_path.string());
  } else {
    DeleteFileW(dump_path.wstring().c_str());
  }
}

static LONG WINAPI xmd_vectored_handler(PEXCEPTION_POINTERS ep) {
  const uint32_t code = ep->ExceptionRecord->ExceptionCode;

  // Guest access violation logging
  if (code == EXCEPTION_ACCESS_VIOLATION) {
    uint64_t fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    if (fault_addr >= 0x100000000ull && fault_addr < 0x1E0000000ull) {
      uint32_t guest_addr = (uint32_t)(fault_addr - 0x100000000ull);
      REXCPU_WARN("VEH: guest access violation at 0x{:08X} (host 0x{:016X})",
                  guest_addr, fault_addr);
      REXCPU_WARN("VEH: RIP=0x{:016X} RSP=0x{:016X} RBP=0x{:016X}",
                  ep->ContextRecord->Rip, ep->ContextRecord->Rsp,
                  ep->ContextRecord->Rbp);
      REXCPU_WARN("VEH: RCX=0x{:016X} RDX=0x{:016X} R8=0x{:016X} R9=0x{:016X}",
                  ep->ContextRecord->Rcx, ep->ContextRecord->Rdx,
                  ep->ContextRecord->R8, ep->ContextRecord->R9);
      REXCPU_WARN("VEH: R10=0x{:016X} R11=0x{:016X} R12=0x{:016X} R15=0x{:016X}",
                  ep->ContextRecord->R10, ep->ContextRecord->R11,
                  ep->ContextRecord->R12, ep->ContextRecord->R15);

      HMODULE hMod = nullptr;
      GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         (LPCSTR)ep->ContextRecord->Rip, &hMod);
      if (hMod) {
        uint64_t base = (uint64_t)hMod;
        uint64_t rva = ep->ContextRecord->Rip - base;
        REXCPU_WARN("VEH: module base=0x{:016X} RVA=0x{:016X}", base, rva);
      }
    }
  }

  // Write minidump for fatal exception types
  if (code == EXCEPTION_ACCESS_VIOLATION ||
      code == EXCEPTION_ILLEGAL_INSTRUCTION ||
      code == EXCEPTION_INT_DIVIDE_BY_ZERO ||
      code == EXCEPTION_STACK_OVERFLOW ||
      code == EXCEPTION_IN_PAGE_ERROR ||
      code == EXCEPTION_BREAKPOINT) {
    REXCPU_WARN("VEH: fatal exception 0x{:08X} at RIP=0x{:016X} - writing minidump",
                code, ep->ContextRecord->Rip);
    xmd_write_minidump(ep);
  }

  return EXCEPTION_CONTINUE_SEARCH;
}

static LONG WINAPI xmd_unhandled_filter(PEXCEPTION_POINTERS ep) {
  REXCPU_WARN("Unhandled exception 0x{:08X} at RIP=0x{:016X} - writing minidump",
              ep->ExceptionRecord->ExceptionCode,
              ep->ContextRecord ? ep->ContextRecord->Rip : 0);
  xmd_write_minidump(ep);
  return EXCEPTION_EXECUTE_HANDLER;
}

static struct XmdCrashHandlerInstaller {
  XmdCrashHandlerInstaller() {
    AddVectoredExceptionHandler(1, xmd_vectored_handler);
    SetUnhandledExceptionFilter(xmd_unhandled_filter);
  }
} xmd_crash_handler_installer;

extern "C" void __imp__KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXKRNL_WARN("KeBugCheckEx: code=0x{:08X} p1=0x{:08X} p2=0x{:08X} p3=0x{:08X} p4=0x{:08X} (suppressed)",
               ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32);
}

extern "C" void __imp__KeBugCheck(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXKRNL_WARN("KeBugCheck: code=0x{:08X} (suppressed) LR=0x{:08X} r3=0x{:08X} r4=0x{:08X} r5=0x{:08X} r6=0x{:08X} r31=0x{:08X}",
               ctx.r3.u32, ctx.lr, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r31.u32);
}

extern "C" void sub_8299AC58(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXCPU_WARN("sub_8299AC58: abort handler suppressed (r3=0x{:08X})", ctx.r3.u32);
}

extern "C" void sub_82766560(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXCPU_WARN("sub_82766560: GFC error handler suppressed (r3=0x{:08X})", ctx.r3.u32);
}

extern "C" void __imp__XamContentGetDefaultDevice(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXKRNL_WARN("XamContentGetDefaultDevice: returning device 0x00000001 (dummy HDD)");
  ctx.r3.u64 = 0x00000001;
}

extern "C" void __imp__XamUserReadProfileSettings(PPCContext& ctx, uint8_t* base) {
  (void)base;

  const uint32_t title_id = ctx.r3.u32;
  const uint32_t user_index = ctx.r4.u32;
  const uint32_t xuid_count = ctx.r5.u32;
  const uint32_t xuids_ptr = ctx.r6.u32;
  const uint32_t setting_count = ctx.r7.u32;
  const uint32_t setting_ids_ptr = ctx.r8.u32;
  const uint32_t unk = ctx.r9.u32;
  const uint32_t buffer_size_ptr = ctx.r10.u32;
  const uint32_t buffer = REX_LOAD_U32(ctx.r1.u32 + 0x30 + 8 * 4);
  const uint32_t overlapped = REX_LOAD_U32(ctx.r1.u32 + 0x30 + 9 * 4);

  REXKRNL_WARN("XamUserReadProfileSettings: title=0x{:08X} user={} xuids={} count={} buf=0x{:08X} ovl=0x{:08X}",
               title_id, user_index, xuid_count, setting_count, buffer, overlapped);

  struct X_USER_READ_PROFILE_SETTINGS {
    rex::be<uint32_t> setting_count;
    rex::be<uint32_t> settings_ptr;
  };

  uint32_t result = 0;

  if (setting_count < 1 || setting_count > 32) {
    result = 0x57;
  } else if (buffer_size_ptr == 0) {
    result = 0x57;
  } else {
    uint32_t needed_size = sizeof(X_USER_READ_PROFILE_SETTINGS) +
                           setting_count * sizeof(rex::system::xam::X_USER_PROFILE_SETTING);
    for (uint32_t i = 0; i < setting_count; ++i) {
      uint32_t id = REX_LOAD_U32(setting_ids_ptr + i * 4);
      uint32_t key_type = (id >> 28) & 0xF;
      uint32_t key_size = (id >> 16) & 0xFFF;
      if (key_type == static_cast<uint32_t>(rex::system::xam::UserProfile::Setting::Type::WSTRING) ||
          key_type == static_cast<uint32_t>(rex::system::xam::UserProfile::Setting::Type::BINARY)) {
        needed_size += key_size;
      }
    }

    uint32_t current_size = REX_LOAD_U32(buffer_size_ptr);
    if (buffer == 0 || current_size < needed_size) {
      *REX_KERNEL_MEMORY()->TranslateVirtual<rex::be<uint32_t>*>(buffer_size_ptr) = needed_size;
      result = 0x7A;
    } else {
      auto mem = REX_KERNEL_MEMORY();
      mem->Zero(buffer, needed_size);

      auto* out_header = mem->TranslateVirtual<X_USER_READ_PROFILE_SETTINGS*>(buffer);
      out_header->setting_count = setting_count;
      out_header->settings_ptr = mem->HostToGuestVirtual(&out_header[1]);

      auto* out_setting = reinterpret_cast<rex::system::xam::X_USER_PROFILE_SETTING*>(out_header + 1);
      for (uint32_t i = 0; i < setting_count; ++i) {
        uint32_t id = REX_LOAD_U32(setting_ids_ptr + i * 4);
        out_setting->from = 0;
        out_setting->unk04 = 0;
        out_setting->user_index = user_index;
        out_setting->setting_id = id;
        out_setting->unk14 = 0;
        std::memset(out_setting->data_bytes, 0, sizeof(out_setting->data));
        ++out_setting;
      }

      result = 0;
    }
  }

  if (overlapped) {
    auto* ovl = REX_KERNEL_MEMORY()->TranslateVirtual<rex::system::XAM_OVERLAPPED*>(overlapped);
    ovl->result = result;
    ovl->extended_error = 0;
    ovl->length = 0;
    result = 0x3E5;
  }

  ctx.r3.u64 = result;
}

REX_EXTERN(__imp__sub_826A1F20);
extern "C" void sub_826A1F20(PPCContext& ctx, uint8_t* base) {
  const uint32_t L = ctx.r3.u32;
  const uint32_t o = ctx.r4.u32;
  const uint32_t t = ctx.r5.u32;

  auto read_u32 = [&](uint32_t addr) -> uint32_t {
    if (!addr) return 0;
    return REX_LOAD_U32(addr);
  };

  const uint32_t ci = read_u32(L + 0x1C);
  const uint32_t func_tvalue = read_u32(ci);
  const uint32_t func_type = func_tvalue ? read_u32(func_tvalue) : 0;
  const uint32_t func_closure = func_tvalue ? read_u32(func_tvalue + 4) : 0;

  const uint32_t prev_ci = ci >= 0x18 ? ci - 0x18 : 0;
  const uint32_t prev_func_tvalue = read_u32(prev_ci);
  const uint32_t prev_func_type = prev_func_tvalue ? read_u32(prev_func_tvalue) : 0;
  const uint32_t prev_func_closure = prev_func_tvalue ? read_u32(prev_func_tvalue + 4) : 0;
  const uint32_t prev_func_f = prev_func_closure ? read_u32(prev_func_closure + 8) : 0;

  REXCPU_WARN(
      "sub_826A1F20: Lua type error! L=0x{:08X} o=0x{:08X} t=0x{:08X} "
      "ci=0x{:08X} func_tv=0x{:08X} func_type=0x{:08X} func_closure=0x{:08X} "
      "prev_ci=0x{:08X} prev_func_tv=0x{:08X} prev_func_type=0x{:08X} "
      "prev_closure=0x{:08X} prev_f=0x{:08X}",
      L, o, t, ci, func_tvalue, func_type, func_closure,
      prev_ci, prev_func_tvalue, prev_func_type, prev_func_closure, prev_func_f);

  __imp__sub_826A1F20(ctx, base);
}

REX_EXTERN(__imp__sub_826A7888);
extern "C" void sub_826A7888_safe(PPCContext& ctx, uint8_t* base) {
  (void)base;
  __imp__sub_826A7888(ctx, base);
}

REX_EXTERN(__imp__sub_826A65D8);
extern "C" void sub_826A65D8(PPCContext& ctx, uint8_t* base) {
  uint32_t obj = ctx.r3.u32;
  if (obj < 0x1000) {
    REXCPU_WARN("sub_826A65D8: null hash table (r4=0x{:08X} r5={} LR=0x{:08X})",
                ctx.r4.u32, ctx.r5.u32, ctx.lr);
    ctx.r3.u64 = 0;
    return;
  }

  uint32_t bucket_hdr = REX_LOAD_U32(obj + 16);
  if (bucket_hdr >= 0x1000 && bucket_hdr < 0xE0000000) {
    uint32_t buckets = REX_LOAD_U32(bucket_hdr + 0);
    if (buckets >= 0x1000 && buckets < 0xE0000000) {
      __imp__sub_826A65D8(ctx, base);
      return;
    }
  }

  REXCPU_WARN("sub_826A65D8: corrupted hash table obj=0x{:08X} (LR=0x{:08X}), returning 0",
              obj, ctx.lr);
  ctx.r3.u64 = 0;
}

REX_EXTERN(__imp__sub_8294B7F8);
extern "C" void sub_8294B7F8(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXCPU_WARN("sub_8294B7F8: fatal error SUPPRESSED LR=0x{:08X} r3=0x{:08X} r4=0x{:08X}",
              ctx.lr, ctx.r3.u32, ctx.r4.u32);
  ctx.r3.u64 = 0;
}

REX_EXTERN(__imp__sub_8294B9C0);
extern "C" void sub_8294B9C0(PPCContext& ctx, uint8_t* base) {
  (void)base;

  uint32_t old_r1 = REX_LOAD_U32(ctx.r1.u32 + 0);
  if (old_r1 < 0x1000 || old_r1 > 0xC0000000) {
    REXCPU_WARN("sub_8294B9C0: bad old_r1=0x{:08X}, just returning r3=0", old_r1);
    ctx.r3.u64 = 0;
    return;
  }

  uint32_t saved_lr = REX_LOAD_U32(old_r1 - 8);
  ctx.r28.u64 = REX_LOAD_U64(old_r1 - 40);
  ctx.r29.u64 = REX_LOAD_U64(old_r1 - 32);
  ctx.r30.u64 = REX_LOAD_U64(old_r1 - 24);
  ctx.r31.u64 = REX_LOAD_U64(old_r1 - 16);

  REXCPU_WARN("sub_8294B9C0: unwinding to LR=0x{:08X} old_r1=0x{:08X}", saved_lr, old_r1);

  ctx.r1.u32 = old_r1;
  ctx.lr = saved_lr;
  ctx.r3.u64 = 1;
}

REX_EXTERN(__imp__sub_82724C10);
extern "C" void sub_82724C10(PPCContext& ctx, uint8_t* base) {
  const uint32_t r5 = ctx.r5.u32;
  char callback_name[128] = {};
  if (r5 >= 0x00100000 && r5 < 0xE0000000) {
    for (int i = 0; i < 120; i++) {
      uint8_t b = base[r5 + i];
      callback_name[i] = b;
      if (b == 0) break;
    }
  }
  REXCPU_WARN("sub_82724C10: callback='{}' r3=0x{:08X} r4=0x{:08X} r5=0x{:08X} LR=0x{:08X}",
              callback_name, ctx.r3.u32, ctx.r4.u32, r5, ctx.lr);
  REXCPU_WARN("sub_82724C10: skipping nil Lua call for '{}'", callback_name);
  return;
}

REX_EXTERN(__imp__sub_826A1F90);
extern "C" void sub_826A1F90(PPCContext& ctx, uint8_t* base) {
  char buf[256] = {};
  uintptr_t r4_host = ctx.r4.u32;
  if (r4_host > 0x10000) {
    for (int off = 0; off <= 32; off += 4) {
      bool found = false;
      for (int i = 0; i < 200; i++) {
        uint8_t* p = (uint8_t*)(r4_host + off + i);
        __try {
          uint8_t b = *p;
          if (b == 0) { if (i > 0) found = true; break; }
          if (i < 255) buf[i] = (b >= 32 && b < 127) ? b : '?';
        } __except(1) {
          break;
        }
      }
      if (found && strlen(buf) > 3) break;
      buf[0] = 0;
    }
  }
  REXCPU_WARN("sub_826A1F90: error reporter! LR=0x{:08X} r3=0x{:08X} r4=0x{:08X} msg='{}'",
              ctx.lr, ctx.r3.u32, ctx.r4.u32, buf);
  __imp__sub_826A1F90(ctx, base);
}

class XmdApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<XmdApp>(new XmdApp(ctx, "xmd",
        PPCImageConfig));
  }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    (void)paths;
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    if (config.gpu_plugin.empty() && !config.graphics) {
      config.gpu_plugin = "xenos";
    }
  }

  void OnPostSetup() override {
    REXGPU_INFO("{}", XMD_VERSION_TEXT);
    auto rs = REXCVAR_QUERY(int32_t, resolution_scale);
    auto msaa = REXCVAR_QUERY(bool, native_2x_msaa);
    REXGPU_INFO("OnPostSetup: resolution_scale={} native_2x_msaa={}", rs, msaa);

    auto* input_sys = runtime()->input_system();
    if (input_sys) {
      auto* win = window();
      auto* typed_sys = static_cast<rex::input::InputSystem*>(input_sys);
      typed_sys->AddDriver(
          std::make_unique<rex::input::mnk::MnkInputDriver>(win, 1));
    }

    SetGuestFrameStats([this]() -> rex::ui::FrameStats {
      auto now = std::chrono::steady_clock::now();
      if (frame_stats_.frame_count > 0) {
        auto dt = std::chrono::duration<double, std::milli>(now - last_frame_time_).count();
        if (frame_stats_.frame_time_ms == 0) {
          frame_stats_.frame_time_ms = dt;
        } else {
          frame_stats_.frame_time_ms = frame_stats_.frame_time_ms * 0.9 + dt * 0.1;
        }
        frame_stats_.fps = 1000.0 / frame_stats_.frame_time_ms;
      }
      last_frame_time_ = now;
      frame_stats_.frame_count++;

      auto* win = window();
      if (win) {
        uint32_t phys_w = win->GetActualPhysicalWidth();
        uint32_t phys_h = win->GetActualPhysicalHeight();
        auto rs = REXCVAR_QUERY(int32_t, resolution_scale);
        auto msaa = REXCVAR_QUERY(bool, native_2x_msaa);
        frame_stats_.render_width = phys_w * (rs > 0 ? rs : 1);
        frame_stats_.render_height = phys_h * (rs > 0 ? rs : 1);
        frame_stats_.resolution_scale = rs > 0 ? rs : 1;
        frame_stats_.msaa_samples = msaa ? 2 : 1;
      }
      return frame_stats_;
    });
  }

  void OnPostLaunchModule(rex::system::XThread* thread) override {
    (void)thread;
    auto* mem = runtime()->virtual_membase();
    if (!mem) return;

    uint8_t* base = mem;
    uint64_t table_addr = (uint64_t)base + REX_IMAGE_BASE + REX_IMAGE_SIZE;
    uint32_t guest_addr = 0x826A7888;
    uint64_t offset = (uint64_t)(guest_addr - REX_CODE_BASE) * 2;
    PPCFunc** slot = (PPCFunc**)(table_addr + offset);

    if (*slot) {
      REXCPU_WARN("OnPostLaunchModule: patching sub_826A7888 table entry");
      *slot = (PPCFunc*)sub_826A7888_safe;
    }
  }

  void OnDestroy() override {
  }

  void OnPostLoadXexImage() override {
    const char* dump_path = std::getenv("XMD_DUMP_CODE_PATH");
    if (!dump_path || !*dump_path) return;

    auto* mem = runtime()->virtual_membase();
    if (!mem) {
      std::fprintf(stderr, "dump_code: virtual_membase is null\n");
      std::exit(1);
    }

    const uint64_t img_base = REX_IMAGE_BASE;
    const uint64_t img_size = REX_IMAGE_SIZE;

    std::filesystem::path out(dump_path);
    std::error_code ec;
    std::filesystem::create_directories(out.parent_path(), ec);

    FILE* f = std::fopen(out.string().c_str(), "wb");
    if (!f) {
      std::fprintf(stderr, "dump_code: cannot open %s for writing\n",
                   out.string().c_str());
      std::exit(1);
    }

    const uint8_t* src = mem + img_base;
    size_t written = std::fwrite(src, 1, img_size, f);
    std::fclose(f);

    std::fprintf(stderr,
                 "dump_code: wrote %zu bytes (0x%zX) from 0x%llX to %s\n",
                 written, written, (unsigned long long)img_base,
                 out.string().c_str());
    std::exit(0);
  }

 private:
  rex::ui::FrameStats frame_stats_{};
  std::chrono::steady_clock::time_point last_frame_time_;
};
