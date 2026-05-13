#include "pctl_ops.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/*
 * Command IDs for IParentalControlService.
 * Reference: https://switchbrew.org/wiki/Parental_Control_services
 *
 * If one of these starts returning errors after a system update, re-check the
 * wiki — Nintendo has shifted command IDs across firmware versions before.
 *
 *   1006 IsRestrictionTemporaryUnlocked  -> bool   (read-only; goes true after a successful 1201)
 *   1031 IsRestrictionEnabled    -> bool   (read-only)
 *   1032 GetSafetyLevel          -> u32    (read-only)
 *   1043 DeleteSettings          (no args) (privileged: pctl:s/pctl:a) -- IRREVERSIBLE
 *   1201 UnlockRestrictionTemporarily  (in: the PIN as an In|HipcPointer buffer ["type 0x9"], NUL-terminated
 *                                       (digits + '\0' == GetPinCodeLength()+1 bytes); no out) -- verified on
 *                                       fw 22.1.0. A bare no-buffer call, or one sent as MapAlias instead of
 *                                       HipcPointer, returns 0xF601 (ResultSessionClosed: the sysmodule drops
 *                                       the session); a buffer of just the digits (no '\0') returns 0xF80E.
 *   1206 GetPinCodeLength        -> u32    (read-only; 0 == no PIN registered)
 *   1208 GetPinCode              -> u32 len + the PIN digits in an Out|HipcPointer buffer ["type 0xA"]  [4.0.0+]
 *   1941 DeletePairing           (no args) (privileged)
 *   (1601/1602/1603 DisableAllFeatures/PostEnableAllFeatures/IsAllFeaturesDisabled all return pctl error
 *    0x00010A8E [module 142 / desc 133] on a normal console — not usable.)
 *   --- play timer (details further down, near pctl_play_timer_query / _dump) ---
 *   1451 StartPlayTimer / 1452 StopPlayTimer  (no args; control the *running* countdown)
 *   1453 IsPlayTimerEnabled -> bool   1454 GetPlayTimerRemainingTime   1455 IsRestrictedByPlayTimer -> bool
 *   145601 GetPlayTimerSettings -> u16[34]   195101 SetPlayTimerSettingsForDebug <- u16[34]
 *   (Never call 1456 / 1951 on fw 22 — they close the pctl session.)
 *
 * Registering/changing the PIN goes through the OS library applet
 * (pctlauthRegisterPasscode), not the service: the direct SetPinCode command is
 * rejected on current firmware.
 *
 * The privileged calls only succeed on a console running CFW (Atmosphere), where
 * libnx's auto-init lands on a `pctl:a`/`pctl:s` session; on a stock console
 * pctl_ops_init() itself fails and the app never gets this far.
 */

Result pctl_ops_init(void) { return pctlInitialize(); }
void   pctl_ops_exit(void) { pctlExit(); }

Result pctl_ops_reinit(void)
{
    // main() holds exactly one pctl reference, so one exit drops the refcount to
    // zero and actually closes the (possibly dead) session; init reopens a fresh one.
    pctlExit();
    return pctlInitialize();
}

void pctl_status_fetch(PctlStatus *out)
{
    memset(out, 0, sizeof(*out));
    Service *srv = pctlGetServiceSession_Service();

    u32 level = 0;
    if (R_SUCCEEDED(serviceDispatchOut(srv, 1032, level))) {
        out->safety_level = level;
        out->safety_level_ok = true;
    }

    u32 pin_len = 0;
    if (R_SUCCEEDED(serviceDispatchOut(srv, 1206, pin_len))) {
        out->pin_length = pin_len;
        out->pin_length_ok = true;
    }

    bool enabled = false;
    if (R_SUCCEEDED(serviceDispatchOut(srv, 1031, enabled))) {
        out->restriction_enabled = enabled;
        out->restriction_enabled_ok = true;
    }
}

Result pctl_set_pin(void)
{
    // The pctlauth applet opens its own privileged pctl session; on some firmware
    // versions that fails while our process still holds one, so drop ours around
    // the call (this exit/init dance is needed).
    pctlExit();
    Result rc = pctlauthRegisterPasscode();
    pctlInitialize();   // re-open our session; a failure here surfaces on the next op
    return rc;
}

Result pctl_delete_parental_controls(void)
{
    return serviceDispatch(pctlGetServiceSession_Service(), 1043);
}

Result pctl_delete_pairing(void)
{
    return serviceDispatch(pctlGetServiceSession_Service(), 1941);
}

Result pctl_unlock_restriction_temporarily(void)
{
    // Temporarily lift the parental-control restriction (cmd 1201). Verified on
    // fw 22.1.0. Two things had to be right (both learned the hard way):
    //  - the buffer descriptors are HIPC *pointer* buffers ("type 0x9"/"0xA" in
    //    SupercellNx's generated pctl IPC stubs == SfBufferAttr_In|HipcPointer /
    //    Out|HipcPointer), not map-alias — sending map-alias makes the sysmodule
    //    drop the session (0xF601);
    //  - the PIN must be passed NUL-terminated (digits + '\0', i.e. GetPinCodeLength
    //    + 1 bytes) — sending just the digits returns 0xF80E.
    // We read the current PIN with GetPinCode (1208) and hand it back, so this works
    // even if the user has forgotten it (we hold a privileged pctl session under CFW).
    pctl_ops_reinit();
    Service *srv = pctlGetServiceSession_Service();

    char pin[32];
    memset(pin, 0, sizeof(pin));
    u32 pin_len = 0;                                                       // GetPinCode (1208): out u32 (length) + out pointer buffer (digits + null-pad)
    Result rc = serviceDispatchOut(srv, 1208, pin_len,
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
        .buffers      = { { pin, sizeof(pin) } });
    if (R_FAILED(rc)) return rc;

    size_t n = (pin_len > 0 && pin_len < (u32)sizeof(pin)) ? ((size_t)pin_len + 1) : sizeof(pin);  // PIN + the trailing '\0'
    return serviceDispatch(srv, 1201,                                      // UnlockRestrictionTemporarily <- PIN pointer buffer
        .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_In },
        .buffers      = { { pin, n } });
}

const char *pctl_safety_level_name(u32 level)
{
    switch (level) {
        case PctlSafetyLevel_None:       return "None";
        case PctlSafetyLevel_Custom:     return "Custom";
        case PctlSafetyLevel_YoungChild: return "Young Child";
        case PctlSafetyLevel_Child:      return "Child";
        case PctlSafetyLevel_Teen:       return "Teen";
        default:                         return "Unknown";
    }
}

/* --------------------------------------------------------------------------
 * Play timer — see CLAUDE.md's "游戏限时" section for the full story; the command
 * IDs are in the table at the top of this file.
 *
 * On fw 22.1.0 the daily play-time limit can't be set in the on-console Settings
 * (only via Nintendo's companion app + sync); the only "set" command in `pctl` is
 * SetPlayTimerSettingsForDebug (195101) — which, helpfully, is callable under
 * Atmosphere. The 0x44-byte (u16[34]) PlayTimerSettings layout is documented just
 * above pctl_play_timer_query() (decoded from a console with a real configured limit).
 * ------------------------------------------------------------------------- */

static void rep(char **p, char *end, const char *fmt, ...)
{
    if (*p >= end) return;
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(*p, (size_t)(end - *p), fmt, ap);
    va_end(ap);
    if (n > 0) { *p += n; if (*p > end) *p = end; }
}

// PlayTimerSettings (fw 22.1.0): u16[34]. Layout, decoded from a console that has
// a real working limit configured via the companion app:
//   [0]   = 0x0101    header magic ("enabled & active"; non-zero == IsPlayTimerEnabled)
//   [1]   = 0x0001    ?
//   [2..6]= 0         (reserved)
//   then 7 per-day groups, group n at indices [7+4n .. 7+4n+3]:
//     [7+4n+0] = 0x0600   ? (constant in the observed config — possibly a bedtime sentinel / format marker)
//     [7+4n+1] = 0x0100   ? ("this day has a configured limit" flag)
//     [7+4n+2] = <minutes>  that day's play-time limit, in minutes (0 == fully blocked that day)
//     [7+4n+3] = 0          (reserved; the last group's [+3] is index 34 — absent, the array stops at 34 u16s)
//   => per-day minutes at indices 9, 13, 17, 21, 25, 29, 33
// A day whose group is left all-zero (in particular [7+4n+1]==0) is unrestricted (no limit).
// 0 minutes != no limit: 0 means that day is fully blocked. Group order is Sun..Sat (confirmed
// by matching a dump against a known configuration). GetPlayTimerRemainingTime (1454) is ns.
void pctl_play_timer_query(PtState *out)
{
    memset(out, 0, sizeof(*out));
    for (int n = 0; n < 7; n++) out->day_min[n] = PT_DAY_NOLIMIT;
    pctl_ops_reinit();
    Service *srv = pctlGetServiceSession_Service();

    bool b = false;
    if (R_SUCCEEDED(serviceDispatchOut(srv, 1453, b))) out->enabled    = b;   // IsPlayTimerEnabled
    b = false;
    if (R_SUCCEEDED(serviceDispatchOut(srv, 1455, b))) out->restricted = b;   // IsRestrictedByPlayTimer
    u64 rem = 0;
    if (R_SUCCEEDED(serviceDispatchOut(srv, 1454, rem))) out->remaining_ns = rem;   // GetPlayTimerRemainingTime (ns)

    u16 c[34]; memset(c, 0, sizeof(c));
    if (R_SUCCEEDED(serviceDispatchOut(srv, 145601, c))) {                    // GetPlayTimerSettings
        out->valid = true;
        for (int n = 0; n < 7; n++)
            out->day_min[n] = c[7 + 4 * n + 1] ? c[7 + 4 * n + 2] : PT_DAY_NOLIMIT;
    }
}

Result pctl_play_timer_set_days(const u16 days_min[7])
{
    pctl_ops_reinit();

    bool any = false;
    for (int n = 0; n < 7; n++) if (days_min[n] != PT_DAY_NOLIMIT) any = true;

    u16 c[34] = {0};
    if (any) {
        c[0] = 0x0101;   // header magic (matches a real configured-via-app limit)
        c[1] = 0x0001;
        // c[2..6] = 0
        for (int n = 0; n < 7; n++) {              // 7 per-day groups, group n at [7+4n..7+4n+3]
            if (days_min[n] == PT_DAY_NOLIMIT) continue;   // leave this day's group all-zero == no limit
            c[7 + 4 * n + 0] = 0x0600;             //   ? (constant in the observed config)
            c[7 + 4 * n + 1] = 0x0100;             //   "this day has a limit" flag
            c[7 + 4 * n + 2] = days_min[n];        //   minutes; 0 == fully blocked that day
            // [7+4n+3] stays 0 (and group 6's would be index 34 — out of bounds)
        }
    }
    // !any: all-zero struct -> the play timer is turned off, IsPlayTimerEnabled becomes false
    return serviceDispatchIn(pctlGetServiceSession_Service(), 195101, c);   // SetPlayTimerSettingsForDebug
}

Result pctl_play_timer_set_uniform(u16 minutes)
{
    u16 d[7];
    for (int i = 0; i < 7; i++) d[i] = minutes;
    return pctl_play_timer_set_days(d);
}

Result pctl_play_timer_clear(void)
{
    u16 d[7];
    for (int i = 0; i < 7; i++) d[i] = PT_DAY_NOLIMIT;   // every day unrestricted -> timer off
    return pctl_play_timer_set_days(d);
}

void pctl_play_timer_dump(char *buf, size_t bufsz)
{
    if (bufsz == 0) return;
    char *p = buf, *e = buf + bufsz;
    buf[0] = '\0';

    Result ir = pctl_ops_reinit();
    if (R_FAILED(ir)) { rep(&p, e, "pctlInitialize failed: 0x%08X\n", (unsigned)ir); return; }
    Service *srv;

    rep(&p, e, "=== Play timer config dump (read-only) ===\n");

    pctl_ops_reinit(); srv = pctlGetServiceSession_Service();
    { bool b = false; Result r = serviceDispatchOut(srv, 1453, b);
      rep(&p, e, "1453 IsPlayTimerEnabled       : rc=0x%08X  %s\n", (unsigned)r, R_SUCCEEDED(r) ? (b ? "true" : "false") : "-"); }
    { bool b = false; Result r = serviceDispatchOut(srv, 1455, b);
      rep(&p, e, "1455 IsRestrictedByPlayTimer  : rc=0x%08X  %s\n", (unsigned)r, R_SUCCEEDED(r) ? (b ? "true" : "false") : "-"); }
    { bool b = false; Result r = serviceDispatchOut(srv, 1458, b);
      rep(&p, e, "1458 IsPlayTimerAlarmDisabled : rc=0x%08X  %s\n", (unsigned)r, R_SUCCEEDED(r) ? (b ? "true" : "false") : "-"); }
    { u64 v = 0; Result r = serviceDispatchOut(srv, 1454, v);
      rep(&p, e, "1454 GetPlayTimerRemainingTime: rc=0x%08X  0x%016llX", (unsigned)r, (unsigned long long)v);
      if (R_SUCCEEDED(r) && v) rep(&p, e, "  (~%llu min if ns)", (unsigned long long)(v / 60000000000ULL));
      rep(&p, e, "\n"); }

    // GetPlayTimerSettings (145601) — the one we care about. 0x44 bytes, buffer prefilled 0xCC.
    u16  cfg[34];
    bool cfg_ok = false;
    pctl_ops_reinit(); srv = pctlGetServiceSession_Service();
    { u8 b[0x44]; memset(b, 0xCC, 0x44);
      Result r = serviceDispatchOut(srv, 145601, b);
      rep(&p, e, "\n145601 GetPlayTimerSettings: rc=0x%08X   (0x44 bytes; CC == server left it)\n", (unsigned)r);
      for (int i = 0; i < 0x44; i++) { rep(&p, e, "%02X ", b[i]); if ((i & 15) == 15) rep(&p, e, "\n"); }
      rep(&p, e, "\n");
      if (R_SUCCEEDED(r)) { memcpy(cfg, b, 0x44); cfg_ok = true; } }
    if (cfg_ok) {
        rep(&p, e, "as u16[34]:");
        for (int i = 0; i < 34; i++) { if ((i & 7) == 0) rep(&p, e, "\n "); rep(&p, e, "[%2d]=%-5u", i, cfg[i]); }
        rep(&p, e, "\n");
        rep(&p, e, "our decode -> per-day min (groups 0..6): %u %u %u %u %u %u %u   header[0..1]=%u %u\n",
            cfg[9], cfg[13], cfg[17], cfg[21], cfg[25], cfg[29], cfg[33], cfg[0], cfg[1]);
    }

    // GetPlayTimerRemainingTimeDisplayInfo (1459, fw 20+) — 0x20 bytes on fw 22.1.0.
    pctl_ops_reinit(); srv = pctlGetServiceSession_Service();
    { u8 b[0x20]; memset(b, 0xCC, sizeof(b));
      Result r = serviceDispatchOut(srv, 1459, b);
      rep(&p, e, "\n1459 GetPlayTimerRemainingTimeDisplayInfo: rc=0x%08X  (0x20 bytes; CC == server left it)\n", (unsigned)r);
      if (R_SUCCEEDED(r))
          for (int i = 0; i < 0x20; i++) { rep(&p, e, "%02X ", b[i]); if ((i & 15) == 15) rep(&p, e, "\n"); }
      rep(&p, e, "\n"); }

    // Restriction state + PIN — for the temporary-unlock path: UnlockRestrictionTemporarily
    // (cmd 1201) takes the PIN as an input pointer buffer, which we read via GetPinCode
    // (cmd 1208, an Out|HipcPointer buffer). All read-only. 1031/1006 show whether there's
    // even a content restriction to unlock and whether it's already temporarily unlocked.
    // The PIN digits are masked ('##' for any ASCII '0'..'9' byte) — only layout/length shows.
    pctl_ops_reinit(); srv = pctlGetServiceSession_Service();
    { bool b = false; Result r = serviceDispatchOut(srv, 1031, b);
      rep(&p, e, "\n1031 IsRestrictionEnabled         : rc=0x%08X  %s\n", (unsigned)r, R_SUCCEEDED(r) ? (b ? "true" : "false") : "-"); }
    { bool b = false; Result r = serviceDispatchOut(srv, 1006, b);
      rep(&p, e, "1006 IsRestrictionTemporaryUnlocked: rc=0x%08X  %s\n", (unsigned)r, R_SUCCEEDED(r) ? (b ? "true" : "false") : "-"); }
    pctl_ops_reinit(); srv = pctlGetServiceSession_Service();
    { u32 len = 0; Result r = serviceDispatchOut(srv, 1206, len);
      rep(&p, e, "1206 GetPinCodeLength             : rc=0x%08X  len=%u\n", (unsigned)r, (unsigned)len); }
    pctl_ops_reinit(); srv = pctlGetServiceSession_Service();
    { u8 b[0x10]; memset(b, 0xCC, sizeof(b)); u32 ret = 0;
      Result r = serviceDispatchOut(srv, 1208, ret,
          .buffer_attrs = { SfBufferAttr_HipcPointer | SfBufferAttr_Out },
          .buffers      = { { b, sizeof(b) } });
      rep(&p, e, "1208 GetPinCode                   : rc=0x%08X  ret=%u   (0x10-byte buf; CC == untouched, ## == ASCII digit)\n ", (unsigned)r, (unsigned)ret);
      for (size_t i = 0; i < sizeof(b); i++) rep(&p, e, (b[i] >= '0' && b[i] <= '9') ? "## " : "%02X ", b[i]);
      rep(&p, e, "\n"); }
}

