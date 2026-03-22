# Production Readiness Plan — Status

Original audit identified 21 issues. All actionable items now resolved except #18 (net48 migration, intentionally deferred).

## Completed (16 fixes from original audit)

| # | Issue | File(s) | Status |
|---|-------|---------|--------|
| 1 | NULL deref on missing USERNAME env var | `src/cli/main.c` | Done |
| 2 | Cache expiry doubled (48h instead of 24h) | `src/cli/cache.c` | Done |
| 3 | `str_replace()` buffer overflow — no bounds check | `src/cli/util.c`, `src/cli/util.h` | Done |
| 4 | Infinite loops in `gd_finalize()` — no timeout | `src/cli/gd.c` | Done |
| 5 | Installer type mismatch + comment about drive range | `installer/setup.iss` | Done |
| 6 | `RunLocal()` timeout not checked + deadlock risk | `src/app/Service/MountService.cs`, `src/app/Service/SshService.cs` | Done |
| 7 | Unchecked `strdup()` returns in `parse_remote()` | `src/cli/main.c` | Done |
| 8 | SFTP handle leak in `gd_opendir()` on malloc failure | `src/cli/gd.c` | Done |
| 9 | `FreeDriveList.First()` crash when empty | `src/app/ViewModel/MainWindowViewModel.cs` | Done |
| 10 | SSH resources (`SshClient`/`SftpClient`) never disposed | `src/app/Service/MountService.cs` | Done |
| 11 | `ObservableCollection` race condition from async context | `src/app/ViewModel/MainWindowViewModel.cs` | Done |
| 12 | Password stored as plain string, not cleared after use | `src/app/ViewModel/MainWindowViewModel.cs` | Done |
| 13 | `rand()` not seeded / not cryptographically secure | `src/cli/util.c` | Done |
| 14 | Hostname buffer overflow (50 bytes, should be 256) | `src/cli/gd.c` | Done |
| 15 | Unchecked `strdup()` on `fs->root` | `src/cli/main.c` | Done |
| 17 | Settings backup exception silently swallowed | `src/app/Common/Settings.cs` | Done |
| 20 | Compiler hardening: Level4 warnings, CFG, no profiling | `src/cli/cli.vcxproj` | Done |

## Completed separately

| # | Issue | Status |
|---|-------|--------|
| 19 | Update vendored libraries (OpenSSL was EOL 1.1.1q) | Done — OpenSSL 3.6.0, libssh2 1.11.1, OpenSSH v10 |
| 21 | SSH connection recovery in CLI | Done — `gd_reconnect()` in `gd.c`, `RETRY_ON_DISCONNECT` macro in `main.c` for getattr/statfs/opendir, connection-error logging in read/write |

## Additional production fixes (second audit)

| Issue | File(s) | Status |
|-------|---------|--------|
| WinHttp NULL deref on `hRequest` | `src/cli/gd.c` | Done |
| Command injection in `gd_check_hlink()` path | `src/cli/gd.c` | Done |
| Channel null check in `run_command_channel_exec()` | `src/cli/gd.c` | Done |
| WSAStartup not cleaned up on `libssh2_init()` failure | `src/cli/gd.c` | Done |
| Orphaned x86 vcxproj configs with hardcoded v142 | `src/cli/cli.vcxproj` | Done — removed |
| MountService not IDisposable | `src/app/Service/MountService.cs` | Done |
| ViewModel doesn't dispose MountService on close | `src/app/ViewModel/MainWindowViewModel.cs` | Done |
| Config file permissions too open | `src/app/Service/MountService.cs` | Done — ACLs restrict to current user |
| PrivateKeyFile not disposed in TestSsh() | `src/app/Service/SshService.cs` | Done |
| Async void methods lack exception handling | `src/app/ViewModel/MainWindowViewModel.cs` | Done |
| Event handler leaks in UserControls | `src/app/Controls/HostControl.xaml.cs`, `PasswordControl.xaml.cs` | Done — Unloaded handlers unsubscribe |
| SanitizeShellArg() untested | `src/test/Unit/SanitizeShellArgTest.cs` | Done — 14 tests |
| No code coverage tooling | `src/test/test.csproj` | Done — coverlet.collector added |
| AppVeyor installs old WinFsp 1.12 | `appveyor.yml` | Done — updated to 2.1 |
| Newtonsoft.Json 13.0.3 outdated | `src/app/app.csproj` | Done — updated to 13.0.4 |

## Reverted (not a bug)

| # | Issue | Reason |
|---|-------|--------|
| 16 | Drive status: split `free` vs `disconnected` branches | Both free and disconnected drives return `DISCONNECTED` by design — golddrive treats them identically (both available for mounting). Test confirms this. |

## Intentionally deferred

| # | Issue | Reason |
|---|-------|--------|
| 18 | Migrate `net48` to `net8.0-windows` | .NET Framework 4.8 ships smaller, all target clients have it pre-installed |

## Build Verification

- CLI build (msbuild Release|x64): **PASS** — 0 errors
- App build (dotnet Release): **PASS** — 0 warnings, 0 errors
- Tests: **46/46 PASS** (32 unit + 14 SanitizeShellArg, 2 SSH-dependent skipped)
