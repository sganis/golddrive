# Production Readiness Plan — Status

Audit identified 21 issues across CLI (C), WPF app (C#), installer, and build config.
16 fixed, 1 reverted (not a bug), 1 deferred (next session), 3 deferred (separate PRs).

## Completed (16 fixes)

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

## Reverted (not a bug)

| # | Issue | Reason |
|---|-------|--------|
| 16 | Drive status: split `free` vs `disconnected` branches | Both free and disconnected drives return `DISCONNECTED` by design — golddrive treats them identically (both available for mounting). Test confirms this. |

## Deferred — Next Session

| # | Issue | Scope |
|---|-------|-------|
| 21 | SSH connection recovery in CLI | Detect dead SSH session in FUSE callbacks, reconnect transparently, retry failed operation. Approach: single reconnect helper protected by existing SRW lock (`g_ssh_lock`), called on connection errors. Needs: new `gd_reconnect()` function in `gd.c`, error detection in `gd_read/gd_write/gd_stat/gd_opendir`, retry wrapper macro or inline check. |

## Deferred — Separate PRs

| # | Issue | Scope |
|---|-------|-------|
| 18 | Migrate `net48` to `net8.0-windows` | Change TargetFramework + LangVersion in `app.csproj` and `test.csproj`, update NuGet packages, test WPF/MaterialDesign/SSH.NET compat, update CI |
| 19 | Update vendored libraries (OpenSSL 1.1.1q is EOL) | Update OpenSSL to 3.x, rebuild libssh2, add VERSION files to `vendor/` dirs, document in CLAUDE.md |

## Build Verification

- CLI build (msbuild Release|x64): **PASS** — warnings are from Level4 on external headers only
- App build (dotnet Release): **PASS** — 0 warnings, 0 errors
- Tests: **39/39 PASS**
