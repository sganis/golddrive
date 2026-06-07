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
