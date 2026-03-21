# Golddrive Improvement Plan

## Overview
This plan addresses critical safety issues, resource leaks, and code quality problems in the Golddrive SSHFS implementation for Windows.

---

## Phase 1: Critical Safety Fixes (Priority: IMMEDIATE)

### 1.1 CLI Memory Safety - Add NULL checks after malloc
**Files:** `src/cli/gd.c`, `src/cli/main.c`, `src/cli/util.c`

| Location | Fix |
|----------|-----|
| gd.c:280 | Add NULL check after `g_ssh = malloc()`, return NULL on failure |
| gd.c:349 | Add NULL check for `inode_hash = malloc()` |
| gd.c:407, 432 | Add NULL checks for target/output malloc in `gd_readlink()` |
| gd.c:652, 920 | Replace `assert(sh)` with proper NULL check and error return |
| gd.c:1656 | Add NULL check for jsmntok_t malloc |
| gd.c:1800, 1804 | Add NULL checks for queue malloc |
| main.c:574, 644, 787 | Add NULL checks for g_logfile, g_conf.pkey, g_conf.home |

### 1.2 Buffer Overflow Fix
**File:** `src/cli/gd.c:1779`
- Replace `vsprintf(message, fmt, args)` with `vsnprintf(message, sizeof(message), fmt, args)`

### 1.3 File Handle NULL Check
**File:** `src/cli/gd.c:1639-1646`
- Add NULL check after `fopen()` before using fp

### 1.4 Socket Leak Fixes
**File:** `src/cli/gd.c:150-270`
- Add `closesocket(sock)` on all error paths in `gd_init_ssh()`
- Use goto cleanup pattern for proper resource release

### 1.5 Command Injection Mitigation
**File:** `src/cli/gd.c:1062`
- Sanitize path before use in shell commands or use SFTP protocol instead

---

## Phase 2: Thread Safety Fixes (Priority: HIGH)

### 2.1 Cache Race Condition
**File:** `src/cli/cache.c:24-40`
- Move `cache_inode_lock()` BEFORE `HASH_FIND_STR()` check in `cache_inode_add()`

### 2.2 Atomic Counters
**File:** `src/cli/main.c`, `src/cli/gd.c`
- Replace `g_sftp_calls++` with `InterlockedIncrement64(&g_sftp_calls)`
- Same for `g_cache_calls`

---

## Phase 3: WPF Application Fixes (Priority: HIGH)

### 3.1 Async Exception Handling
**File:** `src/app/ViewModel/MainWindowViewModel.cs`

Wrap each async void method in try-catch:
- Line 240: `LoadDrivesAsync()`
- Line 321: `ConnectAsync()`
- Line 332: `CheckDriveStatusAsync()`
- Line 342: `GetVersionsAsync()`
- Line 351: `OnConnect()`
- Line 395: `OnConnectPassword()`
- Line 420: `OnSettingsSave()`
- Line 480: `OnSettingsDelete()`

### 3.2 Add Exception Logging to Catch Blocks
**File:** `src/app/Service/MountService.cs`

Add `Logger.Log()` to empty catch blocks at lines: 124, 302, 304, 380, 490, 507, 521, 532, 595, 637, 707, 743, 826, 855

### 3.3 Add IDisposable/using Patterns
**File:** `src/app/Service/MountService.cs`
- Line 169: Wrap Process in `using` statement
- Line 589: Wrap SshClient in `using` in `TestPassword()`
- Line 628: Wrap SshClient in `using` in `TestSsh()`
- Line 683: Wrap SshClient in `using` in `SetupSsh()`

### 3.4 Fix Logic Bug
**File:** `src/app/ViewModel/MainWindowViewModel.cs:438-439`
- Remove duplicate regex check (same condition checked twice with `&&`)

---

## Phase 4: Code Refactoring (Priority: MEDIUM)

### 4.1 Split MountService.cs (916 lines → 4 files)
- `DriveService.cs` - Drive management (~250 lines)
- `SshService.cs` - SSH operations (~200 lines)
- `MountService.cs` - Core mount/unmount (~300 lines)
- `SettingsService.cs` - Config persistence (~100 lines)

### 4.2 Split MainWindowViewModel.cs (723 lines)
- Keep core ViewModel logic (~400 lines)
- Extract command handlers to separate file

### 4.3 Remove Dead Code
- Delete commented-out code blocks in gd.c, main.c, MountService.cs
- Remove unused methods and debug code

---

## Phase 5: Build Modernization (Priority: LOW)

### 5.1 Update Dependencies
**File:** `src/app/packages.config`
- SSH.NET: 2020.0.2 → latest maintained version
- NLog: 4.7.15 → 5.x
- Newtonsoft.Json: 13.0.1 → 13.0.3

### 5.2 Migrate to PackageReference
- Convert packages.config to PackageReference format in .csproj files

### 5.3 Improve Test Coverage
**File:** `src/test/`
- Add unit tests for Settings, Drive model
- Add mocked tests for MountService
- Current coverage: ~5-10%, Target: 50%+

---

## Verification Steps

1. **Build verification:**
   ```cmd
   msbuild src\golddrive.sln -t:rebuild -p:Configuration=Release
   ```

2. **Run existing tests:**
   ```cmd
   vstest.console src\.build\Release\x64\golddrive-test.dll /Settings:src\test\test.runsettings
   ```

3. **Manual testing:**
   - Launch app, connect to SSH server
   - Mount/unmount drives
   - Verify no crashes on connection failures

4. **Memory testing (CLI):**
   - Run with Application Verifier or similar tool
   - Verify no memory leaks on mount/unmount cycles

---

## Implementation Order

| Step | Phase | Est. Time | Risk |
|------|-------|-----------|------|
| 1 | 1.1-1.3 (malloc/buffer fixes) | 2 days | Low |
| 2 | 1.4-1.5 (socket/injection) | 1 day | Medium |
| 3 | 2.1-2.2 (thread safety) | 1 day | Medium |
| 4 | 3.1-3.4 (WPF fixes) | 2 days | Low |
| 5 | 4.1-4.3 (refactoring) | 3-5 days | High |
| 6 | 5.1-5.3 (modernization) | 2-3 days | Medium |
