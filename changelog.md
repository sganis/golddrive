# Changelog

## 1. SDK-Style Project Migration

Both `src/app/app.csproj` and `src/test/test.csproj` were rewritten from legacy MSBuild format to modern SDK-style (still targeting `net48`). Key consequences:

- **`packages.config` deleted** — replaced by `<PackageReference>` entries with updated versions (MaterialDesignThemes 4→5, NLog 4→5, SSH.NET 2020→2024, etc.)
- **`AssemblyInfo.cs`** — version changed from `2.29.0.*` to `2.29.0.0` (SDK-style projects don't support wildcard versioning)
- **`NLog.config`** — `${windows-identity}` → `${environment-user}` (NLog 5 compatibility), encoding changed to UTF-8

## 2. C/C++ Build Modernization

All native `.vcxproj` files (cli, runapp, sanssh, sanssh-libssh) were updated:

- **PlatformToolset** changed from hardcoded `v140`/`v142` to `$(DefaultPlatformToolset)` for x64, ARM64, and Win32 configurations; orphaned x86 configs removed
- **WindowsTargetPlatformVersion** bumped from `8.1` to `10.0`
- A **hardcoded user-specific include path** was replaced with a portable relative path
- Removed Whole Program Optimization and profiling flags from Release builds

## 3. Critical Safety Fixes in `src/cli/gd.c`

- **NULL-safe finalization** — `gd_finalize()` now checks every pointer before closing/freeing
- **Buffer overflow prevention** — `strcpy`/`strcat` → `strcpy_s`/`strcat_s` with length checks
- **NULL check after `fopen`** — prevents NULL dereference on config load failure
- **`calloc` bug fix** — `sizeof(char*)` corrected to `sizeof(char)` (was allocating 8x too much)
- **Memory leak fixes** — added `free()` on error paths in JSON parsing
- **`vsprintf` → `vsnprintf`** in logging to prevent buffer overflow
- **Fixed `fclose(NULL)`** in the logging function

## 4. Thread Safety Fix in `src/cli/cache.c`

Moved `cache_inode_lock()` **before** `HASH_FIND_STR()` — fixing a TOCTOU race condition where the hash table lookup was unprotected.

## 5. WPF Application Fixes

- **`Drive.cs`** — Fixed missing `Path` copy in the copy constructor; safe port parsing with `TryParse` instead of `int.Parse`
- **`MountService.cs`** — Added logging to ~10 empty catch blocks; wrapped `Process` in `using` for disposal; simplified assembly path resolution; handled nullable `ExitStatus` from SSH.NET 2024; added shell injection mitigation for public key strings
- **`MainWindowViewModel.cs`** — Removed a duplicated regex check

## 6. Test Improvements

- **`MountManagerTest.cs`** — Wrapped SSH key setup test in `try/finally` so cleanup always runs, even on assertion failure
- **`test.bat`** — switched from `vstest.console` to `dotnet test`

## 7. Build/Environment Scripts

- **`build.bat`** — rewritten to call `vcvars64.bat`, run `dotnet restore`, then build projects individually with error checking
- **`setenv.bat`** / **`terminal.bat`** — updated VS paths from 2019 → 2022, updated test host IP

## 8. `tools/setupssh.py` Rewrite (~340 → ~1,380 lines)

Complete rewrite with proper architecture: dataclass result types, context-managed SSH client, `cryptography`-library key generation (RSA/Ed25519/ECDSA), ~20 integration tests, argparse CLI, and retry logic.

## 9. Production Readiness (Second Audit)

CLI:
- **SSH reconnection** — `gd_reconnect()` tears down and re-establishes SSH/SFTP/channel; `RETRY_ON_DISCONNECT` macro retries getattr/statfs/opendir on connection errors
- **WinHttp NULL deref fix** — null check on `hRequest` before `WinHttpSendRequest`
- **Command injection fix** — path validation in `gd_check_hlink()` rejects shell metacharacters
- **Channel null check** — `run_command_channel_exec()` validates channel before use
- **WSAStartup cleanup** — `WSACleanup()` called if `libssh2_init()` fails

App:
- **MountService IDisposable** — implements `IDisposable`, disposed on window close
- **Config file ACLs** — `config.json` restricted to current user after save
- **PrivateKeyFile disposal** — wrapped in `using` block in `TestSsh()`
- **Async exception handling** — try-catch added to `LoadDrivesAsync`, `CheckDriveStatusAsync`, `GetVersionsAsync`
- **Event handler leaks** — Unloaded handlers unsubscribe `FocusRequested` in HostControl/PasswordControl

Tests/CI:
- **SanitizeShellArg tests** — 14 new tests covering all injection characters
- **Code coverage** — `coverlet.collector` added to test project
- **AppVeyor WinFsp** — updated from 1.12 to 2.1
- **Newtonsoft.Json** — updated from 13.0.3 to 13.0.4

## 10. New Files

- **`CLAUDE.md`** — project documentation for AI assistant context
- **`PLAN.md`** — 5-phase improvement plan (many items above correspond to this plan)
- **`.claude/settings.local.json`** / **`.vscode/settings.json`** — tool/editor config
