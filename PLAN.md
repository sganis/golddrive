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
| WinFsp 2.x MSI removed from vendor | `vendor/winfsp/` | Done — 2.x incompatible with Windows 10, keeping 1.12 |
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

---

# Round 2 — Hardening & Improvement Plan (proposed 2026-06-07)

The original audit closed the obvious memory-safety and resource bugs. This round
targets what an audit pass misses: latent correctness in the SSH trust boundary,
fuzzing of the hand-written parsers, build/CI mitigations not yet enabled, the one
architectural limit (single global connection), and the platform move to Windows 11.

## Implementation status (first slice — 2026-06-07)

| Item | Status | Verification |
|---|---|---|
| T0 — native unit-test harness | **Done** — `src/clitest/{clitest,fuzz}.c`, `tools/build_clitest.bat`, gated in `tools/test.bat` | Builds + runs locally: **77 checks + 200k property iters, 0 failures** |
| T0c — pure extractions | **Done** — `gd_knownhost_keytype`, `normalize_link_path`, `extract_rcode`, `parse_remote_str`, `parse_json_buffer` all in `src/cli/parse.{c,h}` | Unit-tested |
| F1 — parse_remote | **Done** — extracted to pure `parse_remote_str()`; `main.c` wiring keeps the in-place `fs->remote` mutation (VolumePrefix) intact | 22 unit checks (service/user/host/port/locuser/root/backslash); link CI-gated |
| F2 — load_json | **Done** — extracted to pure `parse_json_buffer()`; `gd.c` keeps file I/O and calls it; orphaned `jsoneq` removed. **Found+fixed a shipped bug:** the token-walk skipped non-first drives with an off-by-one (`i+3` vs `i+2`), so `AppKey`/`Args` were silently unread for any drive not first in `config.json`. | 14 unit checks (multi-drive, missing drive, empty, malformed); link CI-gated |
| I2 — getaddrinfo / IPv6 | **Done** — new `src/cli/net.{c,h}` `gd_tcp_connect()` (resolves IPv4+IPv6, tries each address in turn); `gd_init_ssh` drops the deprecated IPv4-only `gethostbyname` + the `**(int**)h_addr_list` cast | Tested against live loopback listeners — **IPv4 and IPv6** — plus the connection-refused path; `gd.c` wiring CI-gated |
| I1 — connection pool | **Done** — new `src/cli/pool.{c,h}`: N independent SSH connections (`-o connections=N`, default 4, clamped 1–16). `g_ssh` is now thread-local; `gd_lock()` round-robins a pooled connection into it for stateless ops, `gd_lock_conn(sh->conn)` pins file/dir handles to the connection that opened them. Per-connection `SRWLOCK` (replaces the single global lock), per-connection in-place `gd_reconnect`, keepalive iterates the pool, one-time `gd_global_init`. `connections=1` reproduces today's behavior exactly. | Pool math unit-tested (clamp/round-robin, 12 checks); full CLI builds (shipping config) + `--version`/help/suite green; **concurrent I/O correctness rides on CI fsx/fsbench** (default-on-4 ensures the stress suite exercises it) |
| B2 — /CETCOMPAT | **Done** — CET shadow-stack opt-in on Release x64 (`CETCompat` in `cli.vcxproj`, added as an x64-only `ItemDefinitionGroup`). | Verified via `dumpbin /headers`: extended DLL characteristics now report **"CET compatible"** alongside CFG/ASLR/HighEntropyVA/DEP; build + `--version` green |
| B1 — /Qspectre | **Deferred** — the Spectre-mitigated CRT libs are not installed in the local toolchain (confirmed) and are almost certainly absent on the default AppVeyor VS2022 image, so enabling `/Qspectre` would break the CI link. Requires adding the "C++ Spectre-mitigated libs" component to the CI image first. | — |
| I3 — reconnect backoff | **Done** — `gd_reconnect` now retries connection establishment up to 5× with full-jitter exponential backoff (200 ms base, 5 s cap) via pure `backoff_ms()` in `parse.c`. (Mount-lost app-surfacing deferred — the WPF app already polls drive status.) | `backoff_ms` unit-tested for bounds/cap/jitter; full build + suite green |
| B3 — un-suppress C4996 | **Done** — removed the project-wide `<DisableSpecificWarnings>4996>` from every `cli.vcxproj` config; `strdup`→`_strdup`, `localtime`→`localtime_s`, and the 5 read-only `getenv` calls scoped with per-line `#pragma warning(suppress: 4996)`. Build is now **clean with no blanket suppression**, so genuinely-unsafe CRT calls (strcpy/sprintf/gets) surface as `/sdl` errors instead of being hidden. | Build clean (0 warnings/errors) + suite green |
| W1 — WinFsp 1.12 → 2.1 | **Done** — vendored `winfsp-2.1.25156.msi` (latest **stable** release; Authenticode signature **verified** — NAVIMATICS LLC) replaces the 1.12 MSI; `appveyor.yml` installs it; `verify_cli.bat` updated for the 2.x `DYNAMIC` administrative-extract layout. Code builds + runs against 2.1 (FUSE 3.2). | Compiles vs vendored 2.1 SDK (`verify_cli`) **and** installed 2.1 SDK (`build_cli`); `--version`/suite green |
| H1 — host-key keytype | **Done** — `gd.c` host-key block calls `gd_knownhost_keytype()` | Mapping unit-tested (incl. ed25519≠dss regression); full link CI-gated |
| F4 — readlink normalizer | **Done** — `gd_readlink` calls `normalize_link_path()` (also removes a latent `output[rc-1]` OOB at rc==0) | Logic unit-tested; link CI-gated |
| I4 — RCODE sentinel | **Done** — `run_command_channel_exec` calls `extract_rcode()` with last-occurrence hardening | Logic unit-tested; link CI-gated |
| F3 — str_replace bounds | **Done** — regression + canary OOB test (links `util.c`); guards the audit-#3 overflow | Unit-tested locally |
| P1 — randomized property tests | **Partial** — `src/clitest/fuzz.c` drives 200k inputs through `normalize_link_path`/`extract_rcode` with guard-byte OOB detection; libFuzzer/ASan still deferred to CI | Runs locally: 0 failures |
| B4 — ASan | **Blocked locally** — VS "C++ AddressSanitizer" component not installed; stays a CI item | — |
| W2 — Windows 11 target | **Done** — `_WIN32_WINNT=0x0A00`/`NTDDI=0x0A000000` (`version.xml`), installer `MinVersion=10.0.22000` | Build/installer CI-gated |
| H2 — `gd_init_ssh` cleanup ladder | **Done** — all failure paths route through one `goto fail:` that frees each resource only if acquired, plus `libssh2_exit`/`WSACleanup`; fixes the session/socket leaks that accumulated on repeated `gd_reconnect()` failures | Compile-verified vs WinFsp SDK (`tools/verify_cli.bat`); runtime behavior CI-gated |
| W1 — WinFsp 1.12 → 2.x | **Deferred** — needs vendored-binary swap + WinFsp build/runtime | — |

> **Local verification (full build + test, WinFsp SDK installed):** the real msbuild
> build (`tools/build_cli.bat`, shipping config — `/sdl /W4`, CFG, delayload, real libs)
> produces `golddrive.exe` — **BUILD SUCCESS** — and the binary runs correctly
> (`--version`/`--help`/arg errors). Full suite green locally: native unit tests
> (**77 checks + 200k property iters**) and `dotnet test` → **47 passed / 16 skipped /
> 0 failed** (the 16 skips are all SSH-mount tests with no live server). Only the
> mount-level fsx/fsbench/iozone stress runs remain CI-only (need an SSH server + the
> running driver). `tools/verify_cli.bat` still offers a compile-only check from the
> vendored MSI for machines without the WinFsp SDK.

## Guiding rules (apply to every item)

- **R1 — Never break the suite.** Every change keeps the current tests green:
  46 app unit tests + `CliTest` CLI tests + the fsx/fsbench/iozone integration run.
  CI is the gate; a red suite blocks the change.
- **R2 — Every change ships with tests.** Pure C logic → native unit test (T0 harness);
  CLI-observable behavior → C# `CliTest`; parsers → fuzz corpus + crash-regression seeds;
  mount/WinFsp → integration test. No item is "done" without its test column satisfied.
- **R3 — Extract to test.** Logic is pulled into small, linkable, network-free units so it
  can be asserted without a live server or a mount. The same extracted functions back both
  the unit tests (T0) and the fuzz harnesses (P1) — one refactor, two payoffs.
- **R4 — Target Windows 11.** The Windows 10 compatibility constraint is dropped. WinFsp
  upgrades to 2.x and the minimum supported OS becomes Windows 11 (workstream W).

## Verified current build-hardening status (Release|x64)

| Mitigation | State | Source |
|---|---|---|
| `/W4` warnings | On | `cli.vcxproj:271` |
| `/sdl` (additional security checks) | On | `cli.vcxproj:278` |
| `/guard:cf` (Control Flow Guard) | On | `cli.vcxproj:282` |
| `/GS`, ASLR (`/DYNAMICBASE`), DEP (`/NXCOMPAT`), HighEntropyVA | On (MSVC defaults) | — |
| `/Qspectre` (Spectre v1) | **Missing** | — |
| `/CETCOMPAT` (CET shadow stack) | **Missing** | — |
| C4996 (unsafe-CRT / deprecated-API warning) | **Globally suppressed** | `cli.vcxproj:280` |
| ASan / static analysis / fuzzing in CI | **None** | `appveyor.yml` |

## T0 — Enabling: native C unit-test harness (do first; unblocks R2)

The CLI currently has **no unit tests** — its C is only exercised black-box by spawning
`golddrive.exe` (`CliTest`) and by the integration stress run. Pure logic (parsers,
mappers) cannot be asserted directly. T0 fixes that and is a prerequisite for testing
H/F/I items at the unit level.

| # | Item | File(s) | Tests to add |
|---|------|---------|--------------|
| T0a | Split `cli.vcxproj` into a static lib `cli.lib` (gd.c, util.c, cache.c, jsmn.c, new parse units) + thin `golddrive.exe` (main.c) that links it. Behavior-preserving. | `cli.vcxproj`, new `cli.lib` project | `CliTest` stays green (proves no behavior change) |
| T0b | Add native `clitest.exe` (tiny assert runner) linking `cli.lib`; register it in `tools\test.bat` + `appveyor.yml` test_script. | new `src/clitest/`, `appveyor.yml` | The harness itself runs in CI and gates the build |
| T0c | Extract pure, linkable functions used by later items: `gd_knownhost_keytype()` (H1), non-static `parse_remote()` (F1), `parse_json_buffer()` from `load_json` (F2), `normalize_link_path()` from `gd_readlink` (F4), `resolve_host()` (I2), `extract_rcode()` (I4). | `gd.c`, `main.c`, new `parse.c` | Each extraction lands with its unit test (see below) |

## P0 — Security correctness (highest priority)

| # | Item | File(s) | Why | Tests to add | Effort |
|---|------|---------|-----|--------------|--------|
| H1 | **Host-key type only handles RSA/DSS** — every non-RSA key maps to `SSHDSS`, so ed25519/ecdsa hosts are checked under the wrong known_hosts bucket. A changed key returns `NOTFOUND` (TOFU auto-accepts) instead of `MISMATCH` → MITM gap on modern servers. Map ed25519 + ecdsa-256/384/521 via extracted `gd_knownhost_keytype()`. | `gd.c:114-155` | Native unit test: RSA→SSHRSA, ED25519→ED25519, ECDSA-256/384/521→matching bits, unknown→0/fail. Optional CI integration vs an ed25519 test server. | S |
| H2 | **`gd_init_ssh` failure paths leak** the libssh2 session/socket and skip `WSACleanup`/`libssh2_exit`. Accumulates on every failed `gd_reconnect()` attempt. Extract a NULL-safe, idempotent `gd_ssh_free(GDSSH*)` and route all error exits through one `goto fail:`. | `gd.c:42-86, 258-299` | Native unit test: `gd_ssh_free(NULL)` no-ops, double-free guarded, partial struct freed cleanly. ASan stress run (B4) backstops the live leak. | S |

## P1 — Fuzzing the parsers (isolated, high security value)

The four hand-written parsers handle server- or user-controlled input and have **no
libssh2 dependency**. After T0c they link straight into libFuzzer/clang-cl harnesses,
and each harness doubles as a regression test (the corpus runs in CI).

| # | Target | File(s) | Tests to add | Effort |
|---|--------|---------|--------------|--------|
| F1 | `parse_remote()` — UNC/instance string surgery | `main.c:423-513` → `parse.c` | Unit tests for known-good UNC forms + fuzz target + seed corpus | M |
| F2 | `parse_json_buffer()` + jsmn token walk | `gd.c:1406-1542`, `jsmn.c` | Unit tests for valid/truncated/oversized configs + fuzz target | M |
| F3 | `str_replace()` bounds | `util.c:143-168` | Unit tests at/over `result_size` boundary + fuzz target | S |
| F4 | `normalize_link_path()` (server-controlled symlink target) | `gd.c:464-491` → `parse.c` | Unit tests: double-slash, trailing-slash, max-length + fuzz target | S |
| F5 | Seed corpus + 60s/target smoke run wired into AppVeyor; failing seeds become permanent unit cases | `appveyor.yml`, `src/clitest/` | Crash-regression seeds checked into the corpus | S |

## P2 — Build / CI mitigations (low effort, high leverage)

| # | Item | Tests to add | Effort |
|---|------|--------------|--------|
| B1 | Add `/Qspectre` to Release `x64`/`ARM64`. | CI build passes + `CliTest --version` smoke | S |
| B2 | Add `/CETCOMPAT` linker flag (shadow stack). | CI build + mount/unmount integration passes | S |
| B3 | Stop globally suppressing C4996; audit raw `strncpy`/`fopen`/`localtime`/`gethostbyname` callsites, then scope the suppression per-line. | Clean build with no new warnings (treat new warnings as CI failure) | M |
| B4 | Add an ASan build variant (`/fsanitize=address`) run against the fsx/fsbench/iozone stress suite in CI. | ASan run is itself the test; must finish clean | M |
| B5 | Enable MSVC `/analyze` (or GitHub CodeQL cpp+csharp) in CI; the analysis ruleset currently exists only on Debug configs and never runs. | Analysis job gates the build | M |

## P3 — Reliability & throughput improvements ("improve")

| # | Item | File(s) | Why | Tests to add | Effort |
|---|------|---------|-----|--------------|--------|
| I1 | **Single global SSH connection + one SRWLock** serializes every SFTP op, so WinFsp's multi-threaded dispatch can't do parallel I/O. Introduce a small connection pool / per-thread channels. Largest real perf win. | `gd.c` (`g_ssh`, `gd_lock`), `main.c` | Native unit test of the pool (acquire/release/cap/contention); fsx/fsbench show no regression + throughput delta recorded | L |
| I2 | Replace deprecated IPv4-only blocking `gethostbyname` with `getaddrinfo` via extracted `resolve_host()` → IPv6 + cleaner resolution. | `gd.c:70-80` → `parse.c` | Native unit test: IPv4 literal, IPv6 literal, `localhost`, bad-host failure path | S |
| I3 | Reconnect backoff: cap attempts + add jitter; surface mount-lost state to the WPF app. | `gd.c:258`, `main.c:61` | Native unit test of backoff schedule (cap, jitter bounds); C# test for app status surfacing | M |
| I4 | Harden `run_command_channel_exec` `RCODE=` sentinel parsing against output that itself contains the sentinel, via extracted `extract_rcode()`. | `gd.c:1240-1252` → `parse.c` | Native unit test: payloads embedding `RCODE=`, multi-line, no-sentinel | S |

## Workstream W — Windows 11 / WinFsp 2.x upgrade (R4)

Drops the Windows 10 pin and moves the platform forward.

| # | Item | File(s) | Tests to add | Effort |
|---|------|---------|--------------|--------|
| W1 | Upgrade vendored WinFsp 1.12 → 2.x; verify the fuse3 headers/ABI against current callbacks (`fs_ops`). Update the CI MSI install + installer bundle. | `vendor/winfsp/`, `appveyor.yml:22`, `installer/setup.iss` | `CliTest` mount/unmount + create/delete + fsx/fsbench/iozone must pass on 2.x; `CliTest --version` asserts the WinFsp/FUSE 2.x banner | M |
| W2 | Set minimum supported OS to Windows 11: installer `MinVersion`, prereqs in `readme.md`/`CLAUDE.md`. | `installer/setup.iss`, `readme.md`, `CLAUDE.md` | Installer build + existing silent-install smoke step in CI | S |
| W3 | net48 stays (ships in-box on Win11, so audit #18 deferral still holds). Win11-only would *permit* a future net8 move if a runtime is bundled — noted, no action now. | — | n/a | — |

## Dependency hygiene

| # | Item | Tests to add |
|---|------|--------------|
| D2 | Automate a periodic vendored-lib freshness check (OpenSSL/libssh2/OpenSSH/WinFsp) + emit an SBOM. | CI freshness job; SBOM artifact diffed per build |

## Suggested order

1. **T0** — stand up the native test harness; without it R2 can't be met for the C changes.
2. **H1, H2** — small, security-correct, each lands with its T0 unit test.
3. **F1–F5** — fuzz harnesses reuse T0c extractions; corpus runs in CI.
4. **W1, W2** — Windows 11 / WinFsp 2.x, gated by the integration suite on 2.x.
5. **B1–B2** (trivial), then **B4** (ASan) to backstop fuzzing + the WinFsp move.
6. **I2, I4** (small, unit-tested), then **I1** (connection pool) as a deliberate project.
7. **B3, B5, I3, D2** as cleanup.

---

# Round 3 — Connection loss with open file handles (proposed 2026-09-10)

**Status: proposed, nothing implemented.** Findings below are from code inspection on
the current `master` (`627ed30`), not from a live repro.

## Symptom

An editor that holds a file open across an SSH connection drop (VSCode, Sublime) can no
longer save it. The drive letter does **not** look disconnected — `net use` reports OK,
Windows shows no red X — but every write on the open handle fails. Listing the folder in
Explorer restores the drive, and only then can the editor save.

## Why Explorer "fixes" it

Explorer's directory listing goes through `f_getattr` / `f_opendir`, which are two of the
only three callbacks wired to `RETRY_ON_DISCONNECT` (`main.c:64`) — they reconnect **and
retry**, so they succeed and the mount looks healthy again. The editor's already-open
handle is never repaired by that path, so the save keeps failing until the file is
reopened.

## Root cause

| # | Finding | Evidence |
|---|---------|----------|
| C1 | **Open SFTP handles do not survive a reconnect, and nothing reopens them.** `gd_reconnect` calls `libssh2_sftp_shutdown` + `libssh2_session_free` on the dead session; the `LIBSSH2_SFTP_HANDLE`s belong to that session and do not outlive it. There is **no registry of open handles anywhere in the CLI**, so `sh->file_handle` / `sh->dir_handle` are left dangling. The next `gd_read`/`gd_write`/`gd_fstat`/`gd_fsync`/`gd_close` on that fd is a **use-after-free**. | `gd.c:287-346` (teardown at `296-318`), handles stored at `gd.c:736-737`, `gd.c:946-947`, struct `config.h:237-246` |
| C2 | **`read`/`write` reconnect but never retry.** Both detect the connection error, kick a reconnect, then return the original error straight to the application. Even without C1 the save would still fail. | `main.c:173-205` |
| C3 | **Most callbacks have no recovery at all.** Only `f_statfs` (`97`), `f_getattr` (`118`) and `f_opendir` (`232`) use `RETRY_ON_DISCONNECT`. Untouched: `f_open`, `f_create`, `f_truncate`, `f_release`, `f_flush`, `f_fsync`, `f_rename`, `f_unlink`, `f_mkdir`, `f_rmdir`, `f_readlink`, `f_utimens`, `f_readdir`, `f_releasedir`. | `main.c:124-311` |
| C4 | **The pool makes the split visible.** A handle is pinned to the connection that opened it (`sh->conn`), while stateless ops round-robin the pool. With `connections=4`, one dead connection leaves the editor's file unusable while Explorer's listing lands on a healthy connection and works — exactly the "looks fine, won't save" report. | `gd.c:737`, `gd.c:947`, `pool.c` (`gd_lock` vs `gd_lock_conn`) |
| C5 | **Double reconnect race.** `RETRY_ON_DISCONNECT` reads `g_ssh` *after* the failed call and reconnects it. `gd_reconnect` is serialized by the per-connection `SRWLOCK`, so when N dispatcher threads fail on the same dead connection, thread A rebuilds the session and threads B..N then tear down and rebuild the *healthy* session A just created — destroying any handles reopened by C6 in the process. | `main.c:64-77`, `pool.c` (`gd_lock_conn`) |
| C6 | **No session timeout.** The session is blocking (`libssh2_session_set_blocking(ssh, 1)`) and `libssh2_session_set_timeout()` is never called, so a black-holed TCP connection (laptop sleep, VPN drop, NAT timeout) parks a WinFsp dispatcher thread in a socket read **indefinitely** — the error that would trigger recovery never arrives. This is a second, distinct path to "connected but frozen". Keepalive (`libssh2_keepalive_config(ssh, 1, 60)`, thread ping every 30 s) only helps once the socket actually errors. | `gd.c:69`, `gd.c:149`, `main.c:24-54` |

## Work items

| # | Item | File(s) | Tests to add | Effort |
|---|------|---------|--------------|--------|
| R1 | **Per-connection handle registry.** Add an intrusive list of live `GDHANDLE`s to `GDSSH` plus `next`/`prev` + `stale` on `GDHANDLE`. Register in `gd_open`/`gd_opendir`, unregister in `gd_close`/`gd_closedir` — all four already hold the connection lock at that point, so no new locking. `GDHANDLE` already carries `path`, `flags`, `mode`, `dir`, so **no new state is needed to reopen**; offsets need no tracking either, since FUSE passes an absolute offset on every call and `gd_read`/`gd_write` seek before each op (`gd.c:763`, `gd.c:808`). | `config.h:229-246`, `gd.c:665-744`, `869-899`, `900-959`, `1005-1034` | Native (`clitest`): register/unregister ordering, double-unregister, list integrity under interleaved open/close | M |
| R2 | **Reopen handles inside `gd_reconnect`.** Before teardown, NULL every registered handle pointer so no racing thread dereferences freed memory. After the new session is adopted (`gd.c:336-344`), walk the list and `libssh2_sftp_open_ex` each entry from its stored path/flags/mode; on success swap the pointer in, on failure mark the handle `stale`. A stale handle returns `-EIO` once and is freed by `gd_close` without touching the dead pointer. | `gd.c:287-346` | Native: reopen-all / partial-failure / all-fail paths against a faked handle list; integration: drop the server mid-write and assert the write completes | L |
| R3 | **Handle-aware retry for handle ops.** Replace the fire-and-forget reconnect in `f_read`/`f_write` with reconnect → reopen (R2) → retry once → only then return the error. Same treatment for `f_flush`, `f_fsync`, `f_release`, and `f_truncate` when `fi != 0`. Give the macro a handle-op sibling that pins `sh->conn` instead of reading `g_ssh`. | `main.c:64-77`, `149-212`, `291-305` | `CliTest`: write a file across a forced server restart; integration: fsx/fsbench must stay green | M |
| R4 | **Recovery for the remaining path ops**, with explicit idempotency rules — a mutating op may have completed server-side before the socket died, so a blind retry can return a bogus error. On retry only: `mkdir` → `EEXIST` counts as success, `unlink`/`rmdir` → `ENOENT` counts as success, `rename` is **not** retried blindly (probe the target first, else surface the error). `open`/`create`/`readlink`/`utimens`/`truncate(path)` are safe to retry as-is. | `main.c:124-171`, `214-222`, `269-290` | Native: idempotency decision table; `CliTest` for each op across a reconnect | M |
| R5 | **Fix the double-reconnect race (C5).** Add a `generation` counter to `GDSSH`, bumped by `gd_reconnect`. Recovery paths capture the generation before the op and, under the lock, reconnect **only if it is unchanged** — otherwise another thread already healed the connection, so just retry. Prevents B..N from destroying the handles A just reopened. | `config.h:229-235`, `gd.c:287`, `main.c:64-77` | Native: generation monotonicity + skip-reconnect decision; stress: N threads on one killed connection ⇒ exactly one reconnect | S |
| R6 | **Bound the blocking case (C6).** Call `libssh2_session_set_timeout()` on every session (init and reconnect) so a black-holed socket fails with a real error instead of parking a dispatcher thread forever; enable `SO_KEEPALIVE` on the socket in `gd_tcp_connect`. Timeout must exceed the largest legitimate SFTP round trip — start at 30 s, make it `-o timeout=N`. | `gd.c:69`, `gd.c:149`, `net.c` | Native: option parse/clamp; integration: block the port with a firewall rule mid-read and assert the op errors within the timeout instead of hanging | M |
| R7 | **Log + surface degraded state.** One log line per reconnect with connection index, handles reopened, handles lost. If reopen fails permanently, the WPF app's existing status poll should show the drive as degraded rather than healthy. | `gd.c`, `src/app/Service/MountService.cs` | C# test for the status mapping | S |

## Test strategy

Under R2 of the Round-2 guiding rules, every item lands with its test:

- **Native (`src/clitest/`)** — registry bookkeeping, generation logic, idempotency table,
  timeout option parsing. All network-free; the handle list is exercised against a fake
  connection struct, so no server is needed.
- **`CliTest` (`src/test/Cli/CliTest.cs`)** — black-box: open a file, restart/kill the SSH
  server, write, assert success. This is the regression test for the reported bug and the
  one that must fail before R1–R3 and pass after.
- **Integration** — the existing fsx/fsbench/iozone stress run must stay green; add a
  connection-drop variant if CI can bounce sshd mid-run.
- **ASan (B4, still open)** would catch C1 directly; worth revisiting once the registry
  exists, since the use-after-free is now a known reproducible target.

## Order

1. **R5** first — it is small, independent, and without it R2's reopened handles can be
   destroyed by a second thread.
2. **R1** — registry, behavior-preserving on its own.
3. **R2** — reopen on reconnect. This is the fix; C1's use-after-free dies here.
4. **R3** — retry read/write/flush/fsync/release. This is what makes the editor save work.
5. **R6** — session timeout, so the *hang* variant reaches the recovery path at all.
6. **R4**, then **R7** — coverage for the remaining ops, then observability.

## Risks

- **R2 is the sharp edge.** Reopening on a connection whose lock is held by the caller,
  while other threads hold pointers to the old handles, is where a mistake turns a
  recoverable drop into a crash. NULL-first-then-rebuild ordering is load-bearing.
- **Silent data loss on reopen.** A reopened handle is a *new* server-side file
  description. If the file was replaced/truncated remotely in between, a retried write at
  the old offset writes to the wrong content. Reopen should re-stat and refuse (`-EIO`)
  when size/mtime moved in a way inconsistent with the handle's own writes.
- **`O_TRUNC`/`O_EXCL` must not be replayed.** Reopen has to mask creation flags off
  `sh->flags`, or reconnecting mid-session truncates the user's file.
- The blocking-mode session means the `LIBSSH2_ERROR_EAGAIN` loops and `waitsocket`
  (`gd.c:1295`) are largely vestigial today; R6's timeout changes which errors actually
  surface, so those loops need a re-read rather than a trust.

## Implementation status (2026-09-10)

Implemented on `plan/handle-reconnect`. Verification after every item: real msbuild
release build (`tools/build_cli.bat`, `/sdl /W4`, CFG, CET) + native suite
(`tools/build_clitest.bat`).

| Item | Status | Verification |
|---|---|---|
| R1 — handle registry | **Done** — `gd_link` intrusive node in `parse.{c,h}`, embedded as the FIRST member of `GDHANDLE`; `handles` head on `GDSSH`. Register in `gd_open`/`gd_opendir`, unregister in `gd_close`/`gd_closedir`, all under the connection lock. | 16 native checks: walk reaches every handle, middle/head/tail unregister, double unregister no-op, interleaved add/remove, NULL-safety |
| R2 — reopen on reconnect | **Done** — `gd_reconnect` NULLs every registered handle before teardown, then reopens each from stored path/flags/mode via `gd_handle_reopen`. `GD_CREAT\|GD_TRUNC\|GD_EXCL` masked off. Reopen failure marks `stale`. | Build + suite green; live drop behaviour is CI/manual |
| R3 — handle-op retry | **Done** — new `RETRY_HANDLE_OP` pins `sh->conn` (never reads `g_ssh`) and skips the retry when the handle is stale. Applied to `f_read`, `f_write`, `f_flush`, `f_fsync`. | Build + suite green |
| R4 — path-op recovery | **Done** — `RETRY_PATH_OP(op, call)` + pure `retry_result()`. `f_open`/`f_create`/`f_readlink`/`f_utimens`/`f_truncate` plain; `f_mkdir` EEXIST→success; `f_unlink`/`f_rmdir` ENOENT→success. | 11 native checks on the decision table |
| R5 — double-reconnect race | **Done** — `generation` on `GDSSH` bumped by `gd_reconnect`; thread-local `g_gen` captured at lock time; `gd_heal(c, seen_gen)` reconnects only if the generation is unchanged. | Build + suite green |
| R6 — session timeout | **Done** — `libssh2_session_set_timeout()` on every session (init and reconnect), `SO_KEEPALIVE` in `gd_tcp_connect`, `-o timeout=N` (default 30 s, clamped 5–600) via pure `timeout_ms()`. | 8 native checks on parse/clamp/boundaries |
| R7 — log degraded state | **Partial** — CLI side done: one line per reconnect with generation, handles reopened and handles lost, plus a line per handle that could not be reopened. **WPF status surfacing not done** (scoped to the C layer for this pass). | Build + suite green |

### Additional findings from this pass (not in the original Round 3 write-up)

| # | Finding | Fix |
|---|---------|-----|
| A | **Round-robin defeated the retry.** `RETRY_ON_DISCONNECT` healed one connection, then re-ran an impl calling `gd_lock()` → `gd_pool_pick()` → `InterlockedIncrement`, so the retry ran on a *different* connection. After a network drop every connection is dead, so at the default `connections=4` recovery succeeded only by luck. This, not C1, is why the mount "never comes back". | Thread-local pin honoured by `gd_lock()`; recovery pins the healed connection for the retry. NULL outside a retry, so healthy I/O is byte-identical. |
| C | **Handle pointers were read before the lock** in `gd_read`/`gd_write`/`gd_close`/`gd_fstat`/`gd_fsync`/`gd_readdir`/`gd_rewinddir`/`gd_closedir`. A thread parked on the lock resumed with a freed pointer, so R2's reopen alone could not have fixed them. | All reads (and the `!handle` EBADF guards) moved inside the lock. |
| D | **`gd_error()` ran outside the lock** in 8 places (`gd_stat`, `gd_fstat`, `gd_readlink`, `gd_mkdir`, `gd_unlink`, `gd_rmdir`, `_gd_rename`, `gd_check_hlink`). It dereferences `g_ssh->ssh`/`->sftp`, so a concurrent reconnect made it a second use-after-free independent of file handles. It also reassigns `rc`, so a stale `SFTP_OK` could turn a failed `gd_rename` into a `0` return. | Brought inside the lock where an op exists; replaced with a plain log where no lock applies. |
| E | **Keepalive parked while holding the lock.** `gd_keepalive_thread` takes `gd_lock_conn(c)` then calls `libssh2_keepalive_send`; with no session timeout a black-holed socket froze *every* op pinned to that connection, not just one thread. | Bounded by R6's session timeout. |

### Deliberate deviations from the plan

- **`f_release`/`f_releasedir` are not retried.** `gd_close`/`gd_closedir` free the
  `GDHANDLE` on every path, so a second attempt would pass freed memory — reintroducing
  the exact use-after-free this work removes. FUSE calls release once and the fd must be
  freed once with it.
- **`f_truncate` uses the path retry, not the handle retry.** Both branches bottom out in
  `gd_truncate()`, which is a path op on a round-robin connection; `RETRY_HANDLE_OP`'s
  premise (the op ran on `sh->conn`) does not hold for it.
- **`f_rename` is not retried at all.** Not idempotent: if the first attempt succeeded
  before the socket died, the retry reports ENOENT for an operation that worked.
  Distinguishing the cases needs a probe of the target, which is its own race.

### Residual risk

- **Reopen does no consistency check.** A reopened handle is a new server-side file
  description. If a third party replaced or truncated the file during the drop, a retried
  write at the old offset lands in the wrong content. Offsets themselves are safe (FUSE
  passes an absolute offset and `gd_read`/`gd_write` seek before every op). The re-stat
  guard from the Risks section above is **not** implemented.
### Live verification (2026-09-10, san@192.168.100.73)

Tested through a local TCP proxy (127.0.0.1:2222 -> host:22) so connections could be
killed on demand without touching port 22 -- a firewall rule there would have taken down
the user's production `Z:` mount to the same host.

- **Full suite: 63/63 passed.** (Blocked earlier by a WinFsp `VolumePrefix` collision:
  `Z:` already held `\golddrive\san@192.168.100.73`, so the tests' own mount failed with
  `ERROR_FILE_EXISTS`. Not a code fault.)
- **Reopen path confirmed executing end to end.** A 25 MB sustained write through a single
  open handle, with all 4 pooled connections killed mid-write:
  `Handle error (-5) on .../sustained.bin, attempting reconnect` (R3) ->
  `SSH reconnection successful (conn 1, generation 2, 1 handle(s) reopened, 0 lost)`
  (R1+R2) -> write completed, server-side size byte-exact (26214400).
- **A first attempt did not prove anything** and is worth recording: a short
  write/drop/write test passed, but the log showed `0 handle(s) reopened` on every
  reconnect -- background getattr traffic healed the drop during the test's sleep and
  Windows reopened the file, so no handle was ever live on a reconnecting connection. A
  passing test that never enters the code under test is not evidence.

### Soak test (2026-09-10)

4 concurrent writers each holding one handle open, 6 drops of all 4 pooled connections,
with `listdir` traffic throughout so drops also landed mid-readdir:

- 4 x 9600 KiB written, **every file byte-exact** on the server
- 9 reconnects, **9 handles reopened, 0 lost**, 0 reconnect failures
- 8 of 9 reconnects reopened at least one live handle (max 2 in one reconnect)
- 32 directory listings across the drops, no errors; process did not crash

24 connection deaths (6 drops x 4) produced only 9 reconnects -- the R5 generation check
suppressing redundant rebuilds of sessions another thread had already healed.

### Still unverified
- **`gd_rename` across a drop** returns an error by design (see deviations); no test
  covers what an application does with that.
- **Reopen consistency check** remains unimplemented (see above).
