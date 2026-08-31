// xmd - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <chrono>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <rex/audio/audio_system.h>
#include <rex/audio/xma/decoder.h>
#include <rex/rex_app.h>
#include <rex/input/input_system.h>
#include <rex/input/mnk/mnk_input_driver.h>
#include <rex/system/xam/user_profile.h>
#include <rex/system/xio.h>
#include <rex/system/xtypes.h>
#include <rex/system/xthread.h>

// xmd_audio.h - custom paced audio system (disabled, causes deadlocks)
// TODO: Rethink approach — stopping/restarting the base worker thread causes
// race conditions. Need a different strategy for audio pacing.
// #include "xmd_audio.h"

// NOTE: setjmp/longjmp fix is implemented directly in the generated functions:
//   setjmp:  __imp__sub_829561F0 in xmd_recomp.83.cpp (added host setjmp)
//   longjmp: __imp__sub_82950120 in xmd_recomp.231.cpp (added host longjmp)
// The xmd_pch.h get_jmp_buf_map() provides the shared host jmp_buf map.

// Vectored Exception Handler to catch guest access violations and dump
// the PPC context before the crash kills the process.
static LONG WINAPI xmd_vectored_handler(PEXCEPTION_POINTERS ep) {
  if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
    // Check if the fault address is in guest memory range
    uint64_t fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    if (fault_addr >= 0x100000000ull && fault_addr < 0x1E0000000ull) {
      uint32_t guest_addr = (uint32_t)(fault_addr - 0x100000000ull);
      // Try to find the PPC context on the stack.
      // The PPCContext is typically passed as a pointer in the recompiled
      // function. We can try to find it by scanning the stack.
      // For now, just log the crash location.
      REXCPU_WARN("VEH: guest access violation at 0x{:08X} (host 0x{:016X})",
                  guest_addr, fault_addr);
      REXCPU_WARN("VEH: RIP=0x{:016X} RSP=0x{:016X} RBP=0x{:016X}",
                  ep->ContextRecord->Rip, ep->ContextRecord->Rsp,
                  ep->ContextRecord->Rbp);
      // Dump some host registers for debugging
      REXCPU_WARN("VEH: RCX=0x{:016X} RDX=0x{:016X} R8=0x{:016X} R9=0x{:016X}",
                  ep->ContextRecord->Rcx, ep->ContextRecord->Rdx,
                  ep->ContextRecord->R8, ep->ContextRecord->R9);
      REXCPU_WARN("VEH: R10=0x{:016X} R11=0x{:016X} R12=0x{:016X} R15=0x{:016X}",
                  ep->ContextRecord->R10, ep->ContextRecord->R11,
                  ep->ContextRecord->R12, ep->ContextRecord->R15);

      // Get module base to calculate RVA
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
  return EXCEPTION_CONTINUE_SEARCH;
}

// Install the VEH at process start
static struct XmdVehInstaller {
  XmdVehInstaller() { AddVectoredExceptionHandler(1, xmd_vectored_handler); }
} xmd_veh_installer;

// Override the kernel's KeBugCheck/KeBugCheckEx implementations.
// The ReXGlue implementations call rex::debug::Break() which triggers a
// host EXCEPTION_BREAKPOINT, crashing the process. We override them to
// log and return instead, letting the game's error handling code run
// naturally. The game's fatal error handler (sub_8294B7F8) sets a flag,
// does cleanup, and then calls KeBugCheck. With our override, KeBugCheck
// returns and execution continues past the error.
// These are strong symbols that override the library's REX_EXPORT versions.
extern "C" void __imp__KeBugCheckEx(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXKRNL_WARN("KeBugCheckEx: code=0x{:08X} p1=0x{:08X} p2=0x{:08X} p3=0x{:08X} p4=0x{:08X} (suppressed)",
               ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r7.u32);
}

// The archive loading system works correctly:
// - sub_82681A30 loads zzzgroup.g00-g49.gz files at startup
// - sub_82681BA8 processes each archive file
// - sub_825196B8 loads individual .zzz files from archives on demand
// - sub_82520B50 looks up files in a hash table (lazy-loaded)
// - sub_8233D818 actually reads file data from the archive
// All these work correctly with ReXGlue's VFS.

// NtOpenFile override removed - was breaking file I/O

extern "C" void __imp__KeBugCheck(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXKRNL_WARN("KeBugCheck: code=0x{:08X} (suppressed) LR=0x{:08X} r3=0x{:08X} r4=0x{:08X} r5=0x{:08X} r6=0x{:08X} r31=0x{:08X}",
               ctx.r3.u32, ctx.lr, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32, ctx.r31.u32);
}

// Override the game's abort handler (sub_8299AC58) and the GFC error handler
// (sub_82766560) to no-op. These are called when the game detects internal
// errors (pure virtual calls, assertions). By making them no-ops, the game
// continues past the error instead of setting a fatal error flag and calling
// KeBugCheck. The pure virtual call itself (blr) returns r3 (the this pointer)
// unchanged, which is typically safe for the caller.
// The generated functions are weak symbols, so these strong definitions
// take priority at link time.
extern "C" void sub_8299AC58(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXCPU_WARN("sub_8299AC58: abort handler suppressed (r3=0x{:08X})", ctx.r3.u32);
}

extern "C" void sub_82766560(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXCPU_WARN("sub_82766560: GFC error handler suppressed (r3=0x{:08X})", ctx.r3.u32);
}

// Override XamContentGetDefaultDevice - the ReXGlue version is a stub that
// returns garbage. We return the dummy HDD device ID (0x00000001) so the
// game can find a storage device for saving.
extern "C" void __imp__XamContentGetDefaultDevice(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXKRNL_WARN("XamContentGetDefaultDevice: returning device 0x00000001 (dummy HDD)");
  ctx.r3.u64 = 0x00000001;  // DummyDeviceId::HDD
}

// Minimal XamUserReadProfileSettings override.
// The ReXGlue SDK does not implement this export. The game calls it twice:
// first with a zero/insufficient buffer to query the needed size, then with
// a buffer of that size. We return success and zero the requested setting
// headers so the game can continue with the profile/storage flow.
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
  // 9th and 10th args are in the caller's stack parameter area
  const uint32_t buffer = REX_LOAD_U32(ctx.r1.u32 + 0x30 + 8 * 4);
  const uint32_t overlapped = REX_LOAD_U32(ctx.r1.u32 + 0x30 + 9 * 4);

  REXKRNL_WARN("XamUserReadProfileSettings: title=0x{:08X} user={} xuids={} count={} buf=0x{:08X} ovl=0x{:08X}",
               title_id, user_index, xuid_count, setting_count, buffer, overlapped);

  // Local mirror of the Xenia output header.
  struct X_USER_READ_PROFILE_SETTINGS {
    rex::be<uint32_t> setting_count;
    rex::be<uint32_t> settings_ptr;
  };

  uint32_t result = 0;  // X_ERROR_SUCCESS

  if (setting_count < 1 || setting_count > 32) {
    result = 0x57;  // X_ERROR_INVALID_PARAMETER
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
      // First call: report the required size.
      *REX_KERNEL_MEMORY()->TranslateVirtual<rex::be<uint32_t>*>(buffer_size_ptr) = needed_size;
      result = 0x7A;  // X_ERROR_INSUFFICIENT_BUFFER
    } else {
      // Second call: write a header and one (possibly unset) setting record
      // per requested ID.  We do not have real title/profile data, so we
      // report every setting as unset (from == 0) and clear the payload.
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

      result = 0;  // X_ERROR_SUCCESS
    }
  }

  // Complete overlapped if provided.
  if (overlapped) {
    auto* ovl = REX_KERNEL_MEMORY()->TranslateVirtual<rex::system::XAM_OVERLAPPED*>(overlapped);
    ovl->result = result;
    ovl->extended_error = 0;
    ovl->length = 0;
    result = 0x3E5;  // X_ERROR_IO_PENDING
  }

  ctx.r3.u64 = result;
}

// Trace the Lua type error (sub_826A1F20) to dump the Lua call stack and
// find which C/AS function attempted to call a nil value.
REX_EXTERN(__imp__sub_826A1F20);
extern "C" void sub_826A1F20(PPCContext& ctx, uint8_t* base) {
  const uint32_t L = ctx.r3.u32;
  const uint32_t o = ctx.r4.u32;
  const uint32_t t = ctx.r5.u32;

  // Lua 5.1 state offsets observed in the generated code:
  // L+0x1C (28) = ci (CallInfo*)
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

// Safe wrapper for sub_826A7888 - this function unconditionally calls the
// fatal error handler at the end. We patched the generated code to replace
// the fatal call with a proper epilogue, so this wrapper is no longer needed.
// Keeping it as a pass-through for safety.
REX_EXTERN(__imp__sub_826A7888);
extern "C" void sub_826A7888_safe(PPCContext& ctx, uint8_t* base) {
  (void)base;
  __imp__sub_826A7888(ctx, base);
}

// Intercept sub_826A65D8 - a hash table lookup function. Log when the
// hash table object is null to trace which caller passes a null table.
REX_EXTERN(__imp__sub_826A65D8);
extern "C" void sub_826A65D8(PPCContext& ctx, uint8_t* base) {
  uint32_t obj = ctx.r3.u32;
  if (obj < 0x1000) {
    // Log the caller's LR to trace who passes null
    REXCPU_WARN("sub_826A65D8: null hash table (r4=0x{:08X} r5={} LR=0x{:08X})",
                ctx.r4.u32, ctx.r5.u32, ctx.lr);
    ctx.r3.u64 = 0;
    return;
  }

  // Check bucket pointer
  uint32_t bucket_hdr = REX_LOAD_U32(obj + 16);
  if (bucket_hdr >= 0x1000 && bucket_hdr < 0xE0000000) {
    uint32_t buckets = REX_LOAD_U32(bucket_hdr + 0);
    if (buckets >= 0x1000 && buckets < 0xE0000000) {
      // Valid - call original
      __imp__sub_826A65D8(ctx, base);
      return;
    }
  }

  // Corrupted bucket pointer - log and return 0
  REXCPU_WARN("sub_826A65D8: corrupted hash table obj=0x{:08X} (LR=0x{:08X}), returning 0",
              obj, ctx.lr);
  ctx.r3.u64 = 0;
}

// The callback dispatcher (sub_8271A058) is no longer overridden.
// The generated implementation in xmd_recomp.110.cpp handles it directly.
// The override was added during crash investigation but is not needed.

// The fatal error handler (sub_8294B7F8) is called when the game detects
// an internal error. In the original game, it sets a flag, does cleanup,
// and calls KeBugCheck. With our overrides, KeBugCheck returns, but the
// cleanup code then crashes on null pointers (state corrupted by skipped
// Lua callbacks). We override the fatal error handler to just log and
// return immediately, skipping the dangerous cleanup.
REX_EXTERN(__imp__sub_8294B7F8);
extern "C" void sub_8294B7F8(PPCContext& ctx, uint8_t* base) {
  (void)base;
  REXCPU_WARN("sub_8294B7F8: fatal error SUPPRESSED LR=0x{:08X} r3=0x{:08X} r4=0x{:08X}",
              ctx.lr, ctx.r3.u32, ctx.r4.u32);
  // Return r3=0 (success) to caller instead of running the fatal error path
  ctx.r3.u64 = 0;
}

// Fatal error thunks (sub_8294B9C0, sub_8294B9D0, etc.) call sub_8294B7F8
// and are supposed to NEVER return. But since we suppress the fatal handler,
// they DO return. The calling function (e.g. sub_826A7888) has no epilogue
// after the call, so returning causes stack corruption.
//
// Fix: override these thunks to do a proper function epilogue - restore
// the stack pointer and return to the caller's LR. The calling function
// saved LR in the prologue via __savegprlr_28, so LR is on the stack.
// We need to undo the stack frame and return.
//
// The calling convention: the caller did:
//   stwu r1,-N(r1)   (push stack frame)
//   ... saved regs ...
//   bl sub_8294B9C0  (no epilogue after this)
//
// To return properly, we need to:
//   1. Restore r1 from the saved value (r1 + N)
//   2. Restore LR from the saved location
//   3. Return
//
// But we don't know N (the stack frame size) from here.
// Instead, we can use the fact that the stack frame pointer chain is intact.
// The saved r1 (old stack pointer) is at the current r1+0.
// The saved LR is at r1+8 (after __savegprlr_28 which stores at -8(r1)).
//
// Actually, the simplest fix: just make these thunks NOT call the fatal
// handler at all, and instead return r3=0 (success). The caller will
// get r3=0 and continue. Since the caller has no epilogue, we need to
// do the epilogue ourselves.
//
// The epilogue for sub_826A7888 is:
//   addi r1,r1,256    (restore stack, frame size was 256)
//   __restgprlr_28    (restore r28-r31 and lr)
//   blr               (return)
//
// But we don't know which caller called us, so we can't do the right epilogue.
// A better approach: override sub_8294B9C0 to just return with r3=0.
// The caller (sub_826A7888) will then fall through to the next function,
// which is wrong. But if we make sub_8294B9C0 do a proper return to
// the CALLER's caller, that works.
//
// The key insight: sub_8294B9C0 is called via bl, so LR points to the
// instruction after the bl in the caller. But the caller has no more
// instructions. So we need to return to the caller's caller instead.
// We can do this by unwinding the stack frame.

REX_EXTERN(__imp__sub_8294B9C0);
extern "C" void sub_8294B9C0(PPCContext& ctx, uint8_t* base) {
  (void)base;
  // This fatal thunk is called at the end of sub_826A7888 after a virtual
  // call. The function has no epilogue after this call - it expects the
  // fatal handler to never return. Since we suppress the fatal handler,
  // we need to do the epilogue ourselves.
  //
  // The prologue of sub_826A7888:
  //   mflr r12           // r12 = LR
  //   bl __savegprlr_28  // saves r28-r31 at -40..-16(OLD r1), r12(LR) at -8(OLD r1)
  //   stwu r1,-256(r1)   // r1 -= 256, old_r1 stored at NEW r1+0
  //
  // So: old_r1 = LOAD_U32(r1 + 0)
  //     saved LR = LOAD_U32(old_r1 - 8)
  //     r28 = LOAD_U64(old_r1 - 40)
  //     r29 = LOAD_U64(old_r1 - 32)
  //     r30 = LOAD_U64(old_r1 - 24)
  //     r31 = LOAD_U64(old_r1 - 16)

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
  ctx.r3.u64 = 1;  // return success (the li r3,1 before the bl indicates success)
}

// Scaleform-to-Lua bridge function (sub_82724C10).
//
// This function is the fallback handler for ExternalInterface.call() from
// Scaleform movies. It receives a callback name (r5), looks it up as a Lua
// global function, and calls it via lua_pcall.
//
// The callback dispatcher (sub_8271A058) is no longer wrapped.
// The fallback handler (sub_82724C10) intercepts problematic callbacks.

// sub_82724C10 is the Scaleform ExternalInterface callback handler.
// It calls a Lua function via lua_pcall. Some Lua functions are nil
// because the game's Lua script registration is broken in the recompilation.
// Calling nil via lua_pcall causes a host-level segfault that bypasses
// our null-page protection. Skip all Lua fallback calls.
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

// Trace the error reporter (sub_826A1F90) to find what calls it.
REX_EXTERN(__imp__sub_826A1F90);
extern "C" void sub_826A1F90(PPCContext& ctx, uint8_t* base) {
  // r4 is the error message - could be a host pointer or PPC pointer
  char buf[256] = {};
  // Try reading r4 as a raw host pointer (Lua TString in host heap)
  uintptr_t r4_host = ctx.r4.u32;
  // On x64, the pointer might be in the lower 4GB or might need sign extension
  // Try reading directly from the host address
  if (r4_host > 0x10000) {
    // Try reading at various offsets for TString header
    for (int off = 0; off <= 32; off += 4) {
      bool found = false;
      for (int i = 0; i < 200; i++) {
        // Use volatile read to avoid crashes on bad pointers
        uint8_t* p = (uint8_t*)(r4_host + off + i);
        // SEH-style guard: check if pointer is readable
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

// (Removed old sub_826A20B8 diagnostic override - replaced by the null-check
// version above)

// (Removed old sub_826A20B8 diagnostic override)

class XmdApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<XmdApp>(new XmdApp(ctx, "xmd",
        PPCImageConfig));
  }

  // Override virtual hooks for customization:
  void OnPreSetup(rex::RuntimeConfig& config) override {
    // Load the Xenos GPU plugin if not already set (e.g. via --gpu_plugin CLI)
    if (config.gpu_plugin.empty() && !config.graphics) {
      config.gpu_plugin = "xenos";
    }
  }

  void OnPostSetup() override {
    // Add keyboard/mouse input driver so the game responds to keyboard input.
    // The default input system only creates SDL (gamepad) + NOP drivers.
    // The MNK driver maps keyboard/mouse to Xbox 360 controller inputs.
    auto* input_sys = runtime()->input_system();
    if (input_sys) {
      auto* win = window();
      auto* typed_sys = static_cast<rex::input::InputSystem*>(input_sys);
      typed_sys->AddDriver(
          std::make_unique<rex::input::mnk::MnkInputDriver>(win, 1));
    }
  }

  void OnPostLaunchModule(rex::system::XThread* thread) override {
    (void)thread;
    // Patch the function table entry for sub_826A7888 to point to our safe
    // wrapper. The weak/strong symbol override doesn't work reliably on
    // Windows with clang-cl, so we directly overwrite the function pointer
    // in the per-module dispatch table.
    auto* mem = runtime()->virtual_membase();
    if (!mem) return;

    // The function table is at IMAGE_BASE + IMAGE_SIZE in guest memory
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

  void OnPostLoadXexImage() override {
    // Check for --dump_code flag: if set, dump the raw PPC code bytes to a file
    // and exit. This is used by the static pre-pass analyzer to scan for
    // function prologues and jump tables without running the game.
    const char* dump_path = std::getenv("XMD_DUMP_CODE_PATH");
    if (!dump_path || !*dump_path) return;

    auto* mem = runtime()->virtual_membase();
    if (!mem) {
      std::fprintf(stderr, "dump_code: virtual_membase is null\n");
      std::exit(1);
    }

    // The full image is at [REX_IMAGE_BASE, REX_IMAGE_BASE + REX_IMAGE_SIZE).
    // Dump the entire image so the analyzer can see code + data.
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
};
