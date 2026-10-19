# WSS Tunnel (termita) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Mount a golddrive drive from a network with no SSH access (the VPN) but HTTPS access to termita, by carrying the SSH session through termita's `/tunnel` WebSocket.

**Architecture:** termita already relays raw bytes between a WebSocket and an allowlisted `host:port` (termita repo, `src/tunnel.rs`, `doc/tunnel.md`). In the driver, `gd_init_ssh` gets a second transport: `gd_ws_connect` opens the WebSocket with WinHTTP and hands libssh2 one end of a loopback socket pair, with two pump threads between the other end and the WebSocket. Everything above the socket stays the same: `waitsocket`, timeouts, `gd_reconnect` and `known_hosts`. In the app, SSH.NET connects to a local port opened by `WebSocketTunnel`, ported from SEE (`/Users/san/dev/see/windows/Aramco.SEE.Core/SSH/SshNet/WebSocketTunnel.cs`). The user chooses SSH or WSS per drive, by hand.

**Tech Stack:** C (MSVC, libssh2, WinHTTP WebSocket API, Winsock), C# .NET Framework 4.8 (`System.Net.WebSockets.ClientWebSocket`, SSH.NET 2024.2.0), WPF/MaterialDesign, MSTest.

**Spec:** No separate spec. The decisions were agreed in chat on 2026-10-19 and are recorded in *Decisions* below. The termita protocol is in the termita repo's `doc/tunnel.md`, and SEE's client behavior in SEE's `doc/websocket-jump.md`.

## Decisions

- **Manual route, no automatic fallback** (user's choice). Each drive has an **SSH / WSS** choice. With WSS, golddrive never connects directly. With SSH, behavior is exactly what it is today.
- **Per-drive tunnel URL** in config.json, e.g. `wss://termita.apps.<cluster>/tunnel`. The WSS choice appears only when the drive has a URL. *(My call; the user wasn't asked.)*
- **config.json keys** per drive: `"TunnelUrl": "<url>"` and `"UseTunnel": true|false`. The driver uses the tunnel only when `UseTunnel` is `true` **and** `TunnelUrl` is non-empty. *(Finding: the existing per-drive `Args` value goes to WinFsp, not to golddrive's option parser — `src/cli/main.c:686` inserts it after `fuse_opt_parse` ran at `:598` — so the URL cannot travel as `-o tunnel=` inside `Args`.)*
- **Command-line option** `-o tunnel=URL` for direct CLI use and tests. config.json overrides it when the drive has the tunnel set, the same precedence `AppKey` has over `-k`.
- **Credentials never reach termita.** SSH auth, keys and `known_hosts` run end to end against the real host name, exactly as today.
- **The choice is locked while the drive is mounted.** You disconnect, switch, and connect again.

## Global Constraints

- Windows 10 or later (WinHTTP WebSocket, `WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY`); .NET Framework 4.8; C# `LangVersion` 7.3 (no newer syntax).
- No new dependencies: no NuGet packages, nothing in `vendor/`. `winhttp.lib` is already linked by `src/cli/cli.vcxproj`.
- termita protocol, copied from termita `doc/tunnel.md` / `src/tunnel.rs`: `GET <TunnelUrl>?host=<host>&port=<port>` upgraded to a WebSocket. Use `&` instead of `?` when the URL already has a query. Binary frames carry the raw bytes, and text frames are not part of the stream. Refusals arrive as HTTP statuses before the upgrade: **403** host not in `TUNNEL_TARGETS`, **502** termita cannot reach the host, **503** at `TUNNEL_MAX`, **404** no `/tunnel`. termita pings every 30 s and drops a client that is silent for 90 s.
- The `host` sent to termita is the drive's host exactly as configured. termita does not resolve aliases.
- Both directions of every pipe copy concurrently. A single loop that waits on a send can deadlock SFTP while acks flow the other way.
- Proxy: use the Windows system proxy (WPAD/PAC included) with the logged-on user's NTLM/Kerberos credentials, in both the driver and the app.
- Wording of termita refusals in the app (reused from SEE): 403 `termita does not allow <host> (it is not in termita's TUNNEL_TARGETS).` · 502 `termita could not reach <host>.` · 503 `termita is at its connection limit, please try again shortly.` · 404 `There is no tunnel at <url> (is termita up to date?).` · other `termita answered HTTP <n>.` · timeout `Timed out connecting to termita at <urlhost>.` · no answer `Cannot reach termita at <urlhost>: <reason>`
- Builds and tests run only on Windows: a VS 2022 Developer Prompt with WinFsp installed, sshd reachable, and `GOLDDRIVE_HOST/USER/PASS` set (see `tools\test.bat`). From the Mac, push the branch and read AppVeyor's result.
- Work on branch `feat/wss-tunnel`. Commit messages follow the repo style (`feat(cli): …`, `test(app): …`) and carry **no** `Co-Authored-By` trailer or AI attribution (user's global CLAUDE.md).
- Test host alias: tests that must prove no direct connection happened point the drive at host `gd-tunnel-test`. That name resolves only inside the test relay, which maps it to `GOLDDRIVE_HOST`. The first run adds a `gd-tunnel-test` entry to the test user's `~/.ssh/known_hosts`, which is expected.

## Review Focus

1. **Idle mount longer than WinHTTP's default receive timeout.** An idle tunnel sees traffic only from the 60 s ssh keepalive (`gd.c:157`). The WebSocket receive in the driver must never time out on its own, and an idle mount must not reconnect. Test: `Cli_TunnelIdle_DoesNotReconnect` (Task 3).
2. **termita pod restart, or a laptop that sleeps while mounted.** Every tunnel drops at once. The drive must reconnect through `gd_reconnect` and keep working, and a 503 during the outage must not kill the process. Test: `Cli_TunnelDropped_Reconnects` (Task 3).
3. **Leaks per reconnect.** Every dropped connection must end both pump threads and close all WinHTTP handles. After repeated drops the process thread count must stay flat. Checked inside `Cli_TunnelDropped_Reconnects` (Task 3).
4. **Bulk transfer both ways at once** (copying to and from the drive at the same time). Neither direction may stall. Test: `Cli_MountThroughTunnel_Works` writes one file while reading another (Task 3).
5. **The corporate proxy on a real VPN laptop** (authenticating proxy, PAC, TLS inspection). The tunnel must work wherever Edge reaches termita. CI can't simulate this, so it's a manual check in Task 6, Step 7.

---

### Task 1: Native pure helpers (URL, refusal reason, socket pair)

**Files:**
- Modify: `src/cli/parse.h`, `src/cli/parse.c` (two helpers next to `parse_remote_str`)
- Modify: `src/cli/net.h`, `src/cli/net.c` (`gd_socketpair` next to `gd_tcp_connect`)
- Test: `src/clitest/clitest.c` (new `test_tunnel_url`, `test_tunnel_reason`, called from `main`), `src/clitest/nettest.c` (socket pair checks inside `run_net_tests`)

**Interfaces:**
- Produces:
  - `int gd_tunnel_url(const char* base, const char* host, int port, char* out, size_t outlen)` — 0 on success. -1 on NULL input, on a scheme other than `ws://`/`wss://` (case-insensitive), or when the result doesn't fit `outlen`. `host` is percent-encoded (unreserved `A-Za-z0-9-._~` pass through, everything else becomes `%XX` uppercase).
  - `const char* gd_tunnel_reason(int status)` — static text for an HTTP status, never NULL.
  - `int gd_socketpair(SOCKET pair[2])` — 0 and two connected loopback TCP sockets with `TCP_NODELAY` on both, or -1 with no socket left open. WSAStartup must already have been called.

- [ ] **Step 1: Write the failing tests**

```c
/* clitest.c */
static void test_tunnel_url(void)
{
	printf("gd_tunnel_url...\n");
	char u[256];
	CHECK(gd_tunnel_url("wss://t/tunnel", "node1", 22, u, sizeof u) == 0
		&& strcmp(u, "wss://t/tunnel?host=node1&port=22") == 0, "plain");
	CHECK(gd_tunnel_url("ws://t/tunnel?x=1", "node1", 2222, u, sizeof u) == 0
		&& strcmp(u, "ws://t/tunnel?x=1&host=node1&port=2222") == 0, "existing query -> &");
	CHECK(gd_tunnel_url("WSS://t/tunnel", "fe80::1", 22, u, sizeof u) == 0
		&& strcmp(u, "WSS://t/tunnel?host=fe80%3A%3A1&port=22") == 0, "encodes host, scheme any case");
	CHECK(gd_tunnel_url("https://t/tunnel", "h", 22, u, sizeof u) == -1, "https rejected");
	CHECK(gd_tunnel_url(NULL, "h", 22, u, sizeof u) == -1, "null base");
	CHECK(gd_tunnel_url("wss://t/tunnel", "node1", 22, u, 10) == -1, "too small");
}

static void test_tunnel_reason(void)
{
	printf("gd_tunnel_reason...\n");
	CHECK(strstr(gd_tunnel_reason(403), "TUNNEL_TARGETS") != NULL, "403");
	CHECK(strstr(gd_tunnel_reason(502), "could not reach") != NULL, "502");
	CHECK(strstr(gd_tunnel_reason(503), "connection limit") != NULL, "503");
	CHECK(strstr(gd_tunnel_reason(404), "no tunnel") != NULL, "404");
	CHECK(strstr(gd_tunnel_reason(407), "proxy") != NULL, "407");
	CHECK(gd_tunnel_reason(418) != NULL && strlen(gd_tunnel_reason(418)) > 0, "other");
}
```

In `nettest.c`, inside `run_net_tests`: `gd_socketpair(p)` returns 0. Bytes `"abc"` sent on `p[0]` arrive on `p[1]`, and `"xyz"` sent on `p[1]` arrive on `p[0]`. After `closesocket(p[0])`, `recv(p[1])` returns 0. Use `NCHECK` messages `"socketpair ok"`, `"pair 0->1"`, `"pair 1->0"`, `"close -> eof"`.

- [ ] **Step 2: Run the tests to verify they fail**

Run: `tools\build_clitest.bat`
Expected: compile/link errors for `gd_tunnel_url`, `gd_tunnel_reason`, `gd_socketpair`.

- [ ] **Step 3: Implement the three helpers**

The reason strings are the CLI's log wording, not the app's: 403 `termita does not allow this host (it is not in TUNNEL_TARGETS)`, 502 `termita could not reach the host`, 503 `termita is at its connection limit`, 404 `there is no tunnel at this URL (is termita up to date?)`, 407 `the proxy requires authentication`, other `termita answered an unexpected HTTP status`.

`gd_socketpair` binds a listener to `127.0.0.1:0`, connects to it, accepts once, and closes the listener. It accepts the peer only if the address it got equals the connecting socket's `getsockname`, so no other local process can take the slot. On any failure it closes everything and returns -1.

- [ ] **Step 4: Run the tests to verify they pass**

Run: `tools\build_clitest.bat`
Expected: `RESULT: PASS`

- [ ] **Step 5: Commit**

```bash
git add src/cli/parse.h src/cli/parse.c src/cli/net.h src/cli/net.c src/clitest/clitest.c src/clitest/nettest.c
git commit -m "feat(cli): tunnel URL, refusal reasons and a loopback socket pair"
```

---

### Task 2: termita stand-in for the C# tests (`TunnelRelay`)

**Files:**
- Create: `src/test/Tunnel/TunnelRelay.cs`
- Test: `src/test/Tunnel/TunnelRelayTest.cs`

**Interfaces:**
- Produces (namespace `golddrive_test.Tunnel`):
  - `public sealed class TunnelRelay : IDisposable`
  - `public static TunnelRelay StartOrInconclusive()` — starts an `HttpListener` on `http://localhost:<free port>/`. If the listener can't start, it calls `Assert.Inconclusive`.
  - `public string Url` — `ws://localhost:<port>/tunnel`
  - `public IDictionary<string, string> Aliases` — maps the `host` query value to a real host. Unmapped values pass through.
  - `public int Status { get; set; }` — 0 accepts. Any other value answers that HTTP status before the upgrade.
  - `public int Accepted { get; }` — the number of WebSockets upgraded and connected to a target (Interlocked).
  - `public string LastQuery { get; }`
  - `public void DropAll()` — aborts every live WebSocket and its TCP connection, the way a restarted termita pod would.
  - `public static TcpListener StartEcho(out int port)` — a loopback TCP echo server for tests without sshd.

- [ ] **Step 1: Write the failing tests**

`Relay_PipesBytesToTargetAndBack`: with an echo target, `ClientWebSocket` connects to `Url + "?host=127.0.0.1&port=" + echoPort`, sends the binary frame `SSH-2.0-test\r\n`, and receives the same bytes. Asserts `Accepted == 1`.
`Relay_Status403_RefusesBeforeUpgrade`: with `Status = 403`, `ConnectAsync` throws `WebSocketException` and `Accepted == 0`.
`Relay_UnreachableTarget_Answers502`: with a closed port as target, `ConnectAsync` throws and the inner `WebException` carries status 502.
`Relay_Alias_MapsHost`: with `Aliases["gd-tunnel-test"] = "127.0.0.1"`, a connect to `host=gd-tunnel-test&port=<echo>` round-trips.
`Relay_DropAll_ClosesLiveTunnels`: after `DropAll()`, a pending `ReceiveAsync` completes with a close or an exception within 5 s.

- [ ] **Step 2: Run them to verify they fail**

Run: `dotnet test src\test\test.csproj -c Release --filter "FullyQualifiedName~TunnelRelayTest"`
Expected: build error, `TunnelRelay` not defined.

- [ ] **Step 3: Implement `TunnelRelay`**

Follow SEE's test stand-in (`/Users/san/dev/see/windows/Aramco.SEE.Test/SSH/WebSocketTunnelTest.cs`, `Serve`/`Echo`). The difference: it dials the requested `host:port` with `TcpClient` and answers 502 if that fails, *before* accepting the WebSocket. It then pipes both directions concurrently as binary frames.

- [ ] **Step 4: Run them to verify they pass**

Run: same as Step 2.
Expected: 5 passed.

- [ ] **Step 5: Commit**

```bash
git add src/test/Tunnel/TunnelRelay.cs src/test/Tunnel/TunnelRelayTest.cs
git commit -m "test: an in-process stand-in for termita's /tunnel"
```

---

### Task 3: Driver transport through the tunnel

**Files:**
- Create: `src/cli/wstunnel.h`, `src/cli/wstunnel.c`
- Modify: `src/cli/cli.vcxproj` (add `<ClCompile Include="wstunnel.c" />` and `<ClInclude Include="wstunnel.h" />` next to `net.c`/`net.h`)
- Modify: `src/cli/config.h` (`char* tunnel;` in `GDCONFIG`)
- Modify: `src/cli/parse.h` / `src/cli/parse.c` (`gd_json` gains `char tunnel[GD_REMOTE_MAX]; int has_tunnel;`, filled by `parse_json_buffer`)
- Modify: `src/cli/gd.c` — `load_json` (`:1693`) copies `has_tunnel`, and `gd_init_ssh` (`:80`) picks the transport
- Modify: `src/cli/main.c` — the `fs_opts` entry, the `--help` line, startup validation and the log line
- Test: `src/clitest/clitest.c` (`test_parse_json`), `src/test/Cli/CliTest.cs`

**Interfaces:**
- Consumes: `gd_tunnel_url`, `gd_tunnel_reason`, `gd_socketpair` (Task 1); `TunnelRelay` (Task 2).
- Produces: `SOCKET gd_ws_connect(const char* url, const char* host, int port, int timeout_ms, char* err, size_t errlen)` — a connected blocking SOCKET that libssh2 can use, or `INVALID_SOCKET` with a one-line reason in `err`. It also produces the `-o tunnel=URL` option and config.json `TunnelUrl` + `UseTunnel`.

- [ ] **Step 1: Write the failing tests**

In `test_parse_json` (`clitest.c`), add a config with three drives:
- `"T:":{"TunnelUrl":"wss://t/tunnel","UseTunnel":true}` → `has_tunnel` and `tunnel` equals `wss://t/tunnel`
- `"U:":{"UseTunnel":true,"TunnelUrl":"wss://t/tunnel"}` (keys reversed) → `has_tunnel`
- `"V:":{"TunnelUrl":"wss://t/tunnel","UseTunnel":false}` → `!has_tunnel`

Also check that a drive with `UseTunnel` true and no `TunnelUrl` gives `!has_tunnel`.

In `CliTest.cs`:
- `Cli_Help_ListsAllOptions`: also asserts that the help output contains `tunnel=`.
- `Cli_Tunnel_BadScheme_Fails`: `Run(@"Z: \\golddrive\h -o tunnel=https://x/tunnel")` → non-zero exit, and stderr contains `ws:// or wss://`.
- `Cli_MountThroughTunnel_Works`, following `Cli_MountUnmount_Works`: relay with `Aliases["gd-tunnel-test"] = _host`. Start `X: \\golddrive\{_user}@gd-tunnel-test -o tunnel={relay.Url}` and wait for `X:\`. Write a 20 MB random file `X:\tmp\tun_a.bin` while reading back a 20 MB file written just before (`tun_b.bin`), on two threads. Assert the MD5 of each read-back matches what was written, `relay.Accepted >= 1`, and delete both files.
- `Cli_TunnelRefused_ExitsWithReason`: with `relay.Status = 403`, the same start → the process exits non-zero within 20 s, and stdout contains `TUNNEL_TARGETS`.
- `Cli_TunnelDropped_Reconnects`: mount as above and record `gd.Threads.Count`. Then loop 5 times: `relay.Status = 503; relay.DropAll(); Thread.Sleep(3000); relay.Status = 0;`, then `File.WriteAllText(@"X:\tmp\tun_drop.txt", i.ToString())` and read it back equal within 60 s (retry on IOException). After the loop, assert the process is alive and `gd.Threads.Count <= before + 2`.
- `Cli_TunnelIdle_DoesNotReconnect` (`[TestCategory("Slow")]`): mount, write a file, and note `relay.Accepted`. `Thread.Sleep(TimeSpan.FromSeconds(75))`, then list `X:\tmp` successfully and assert `relay.Accepted` is unchanged.

- [ ] **Step 2: Run them to verify they fail**

Run: `tools\build_clitest.bat`, then `tools\build_cli.bat`, then `dotnet test src\test\test.csproj -c Release --filter "FullyQualifiedName~CliTest"`
Expected: the `test_parse_json` tunnel checks FAIL, `Cli_Help_ListsAllOptions` and `Cli_Tunnel_BadScheme_Fails` FAIL, and the mount tests FAIL. golddrive rejects the unknown `tunnel=` option, or connects to `gd-tunnel-test` directly and fails to resolve it.

- [ ] **Step 3: Implement config and options**

- `parse_json_buffer` collects `TunnelUrl` (string) and `UseTunnel` (primitive whose text is `true`) for the matched drive, in either order. It sets `has_tunnel` only when both are present and the URL is non-empty.
- `load_json`: `if (j.has_tunnel) fs->tunnel = _strdup(j.tunnel);`
- `main.c`:
  - Add `fs_OPT("tunnel=%s", tunnel, 0)`.
  - Add the help line `"    -o tunnel=URL              ssh through termita's web tunnel (ws:// or wss://)\n"`.
  - After `parse_remote`, if `g_conf.tunnel` is set and `gd_tunnel_url(g_conf.tunnel, g_conf.host, g_conf.port, …)` fails, print `error: tunnel must be a ws:// or wss:// URL` to stderr and return 1.
  - Log `tunnel   = %s` after `pkey`.
- `gd_init_ssh`: if `g_conf.tunnel` is set, use `gd_ws_connect(g_conf.tunnel, g_conf.host, g_conf.port, timeout_ms(g_conf.timeout, GD_TIMEOUT_DEFAULT, GD_TIMEOUT_MIN, GD_TIMEOUT_MAX), err, sizeof err)` and log failures as `failed to open web tunnel to %s:%d via %s: %s`. Otherwise use `gd_tcp_connect`, unchanged.

- [ ] **Step 4: Implement `gd_ws_connect` in `wstunnel.c`**

Handshake:
- `WinHttpOpen(L"Golddrive", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, …)`. Split the URL from `gd_tunnel_url` with `WinHttpCrackUrl`. Use `WINHTTP_FLAG_SECURE` for `wss`.
- On the request, set `WINHTTP_OPTION_AUTOLOGON_POLICY = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW` and `WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET`.
- Bound resolve, connect, send and receive by `timeout_ms` with `WinHttpSetTimeouts`.
- On 407, pick a scheme with `WinHttpQueryAuthSchemes`, call `WinHttpSetCredentials(…, WINHTTP_AUTH_TARGET_PROXY, scheme, NULL, NULL, NULL)`, and resend once.
- On any status other than 101, write `gd_tunnel_reason(status)` to `err`. On a WinHTTP failure, write `cannot reach termita at <urlhost>: WinHTTP error <GetLastError()>`. Return `INVALID_SOCKET` with every handle closed.
- After 101, set `WINHTTP_OPTION_RECEIVE_TIMEOUT` to 0 (infinite) on the request, then call `WinHttpWebSocketCompleteUpgrade`. An idle tunnel must never time out on its own (Review Focus 1). Dead peers are found by libssh2's own timeout and keepalive. If `Cli_TunnelIdle_DoesNotReconnect` still shows a reconnect, also set the option on the WebSocket handle.

Pump (the part the signature doesn't determine):
- `gd_socketpair(p)`, then return `p[0]` to the caller.
- Keep a heap struct holding `p[1]`, the session, connection, request and WebSocket handles, and `LONG refs = 2`. Start two threads with 32 KiB buffers.
  - **up** (socket → WebSocket): loop `recv(p[1])` and send each chunk with `WinHttpWebSocketSend(…, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE, …)`. When `recv` returns ≤ 0 or a send fails, call `WinHttpCloseHandle(ws)`, which cancels down's blocked receive.
  - **down** (WebSocket → socket): loop `WinHttpWebSocketReceive`. Forward the `BINARY_MESSAGE`/`BINARY_FRAGMENT` buffer types with `send(p[1])`, discard `UTF8_*`, and end on `CLOSE` or an error. On exit, call `shutdown(p[1], SD_BOTH)`, which unblocks up's `recv`.
  - The ws handle is closed exactly once, by up. Whichever thread drops `refs` to 0 closes the request, connection and session handles and `closesocket(p[1])`, then frees the struct.
- When the driver calls `closesocket(c->socket)` (`gd_reconnect`, teardown), up sees EOF, and the whole pipe unwinds with no other bookkeeping in `gd.c`.

- [ ] **Step 5: Run the tests to verify they pass**

Run: `tools\build_clitest.bat`, then `tools\build_cli.bat`, then `dotnet test src\test\test.csproj -c Release --filter "FullyQualifiedName~CliTest"`
Expected: `RESULT: PASS`, and every CliTest passes, including the existing mount tests over plain SSH.

- [ ] **Step 6: Commit**

```bash
git add src/cli/wstunnel.h src/cli/wstunnel.c src/cli/cli.vcxproj src/cli/config.h src/cli/parse.h src/cli/parse.c src/cli/gd.c src/cli/main.c src/clitest/clitest.c src/test/Cli/CliTest.cs
git commit -m "feat(cli): ssh through termita's web tunnel (-o tunnel=, config.json TunnelUrl/UseTunnel)"
```

---

### Task 4: App `WebSocketTunnel` (ported from SEE)

**Files:**
- Create: `src/app/Service/WebSocketTunnel.cs`
- Test: `src/test/Tunnel/WebSocketTunnelTest.cs`

**Interfaces:**
- Consumes: `TunnelRelay`, `TunnelRelay.StartEcho` (Task 2).
- Produces (namespace `golddrive`):
  - `public class TunnelException : Exception` — `public TunnelException(string message)`. Everything `Ensure` reports to the user is one of these, worded per Global Constraints.
  - `public class WebSocketTunnel : IDisposable` with `public WebSocketTunnel(string tunnelUrl, string host, int port)`, `public void Ensure()` (throws `TunnelException`), `public bool IsAlive`, `public string Host` (`127.0.0.1`), `public int Port` (0 until `Ensure`), `public void Dispose()` (repeatable), `public static Uri TunnelUri(string tunnelUrl, string host, int port)`, and `public static string Describe(Exception exp, string host, Uri url)`.

- [ ] **Step 1: Write the failing tests**

Port these from SEE's `WebSocketTunnelTest.cs`, using `TunnelRelay` with an echo target instead of SEE's echoing server: `TunnelUri_AddsHostAndPort`, `BeforeEnsure_IsDownWithPortZero`, `Ensure_BytesRoundTripThroughTheLocalPort`, `Ensure_Forbidden_SaysTermitaDoesNotAllowTheHost` (message equals `termita does not allow testhost (it is not in termita's TUNNEL_TARGETS).`), `Ensure_BadGateway_SaysTermitaCannotReachTheHost`, `Ensure_ServerDown_SaysTermitaIsUnreachable` (starts with `Cannot reach termita at localhost`), `LargeTransfer_BothWaysConcurrently` (4 MB each way), and `Dispose_StopsListening_AndIsRepeatable`. Drop SEE's `RunStdio_*` and `StdioStream_*` tests; golddrive has no ProxyCommand mode.

- [ ] **Step 2: Run them to verify they fail**

Run: `dotnet test src\test\test.csproj -c Release --filter "FullyQualifiedName~WebSocketTunnelTest"`
Expected: build error, `WebSocketTunnel` not defined.

- [ ] **Step 3: Port the class**

Copy SEE's `WebSocketTunnel` without `RunStdioAsync`, `ITunnel` and `NODE_PORT`. Change `Logger.LogAction`/`Logger.LogError` to golddrive's `Logger.Log`. `Ensure` throws `TunnelException(Describe(...))`. The rest stays: the 30 s connect timeout, the 20 s WebSocket keepalive, the system proxy with `DefaultCredentials`, TLS 1.2, and `ConfigureAwait(false)` with the preflight run through `Task.Run`.

- [ ] **Step 4: Run them to verify they pass**

Run: same as Step 2.
Expected: 8 passed.

- [ ] **Step 5: Commit**

```bash
git add src/app/Service/WebSocketTunnel.cs src/test/Tunnel/WebSocketTunnelTest.cs
git commit -m "feat(app): WebSocketTunnel, a local port to the host through termita (from SEE)"
```

---

### Task 5: Drive settings and SSH.NET through the tunnel

**Files:**
- Modify: `src/app/Common/Drive.cs` (properties, `Clone`, `Trim`)
- Modify: `src/app/Common/Settings.cs` (`Load`, `AddDrive`)
- Modify: `src/app/Service/SshService.cs` (`TestHost`, `TestPassword`, `TestSsh`, `SetupSsh`)
- Test: `src/test/Unit/DriveTest.cs`, `src/test/Unit/SettingsTest.cs`, `src/test/Tunnel/SshServiceTunnelTest.cs`

**Interfaces:**
- Consumes: `WebSocketTunnel`, `TunnelException` (Task 4); `TunnelRelay` (Task 2).
- Produces, on `Drive`:
  - `[JsonProperty] public string TunnelUrl` — null when empty, like `Args`. It notifies `HasTunnel` too.
  - `[JsonProperty] public bool UseTunnel`
  - `public bool HasTunnel => !string.IsNullOrEmpty(TunnelUrl)`
  - `public bool ViaTunnel => UseTunnel && HasTunnel`
- Produces, in `SshService`: `private IDisposable OpenRoute(Drive drive, out string host, out int port)`. With `ViaTunnel`, it builds a `WebSocketTunnel(drive.TunnelUrl, drive.Host, drive.CurrentPort)`, calls `Ensure()`, and returns the tunnel with its `Host`/`Port`. Otherwise it returns `null` with `drive.Host`/`drive.CurrentPort`.

- [ ] **Step 1: Write the failing tests**

Unit tests (no sshd):
- `Drive_ViaTunnel_RequiresUrlAndChoice` covers the four combinations.
- `Drive_Clone_CopiesTunnel`.
- `Drive_Trim_TrimsTunnelUrl`.
- `Settings_Load_ReadsTunnelFields`: a temporary config.json with `"Z:":{"MountPoint":"h","TunnelUrl":"wss://t/tunnel","UseTunnel":true}` loads `TunnelUrl` and `UseTunnel == true`.
- `Settings_AddDrive_CopiesTunnelFields`: updating an existing drive copies both fields.

`SshServiceTunnelTest`:
- `TestHost_ThroughTunnel_Forbidden_SaysWhy` (no sshd): with `relay.Status = 403`, `TestHost` returns `BAD_HOST` and an `Error` that contains `does not allow`.
- `TestHost_ThroughTunnel_Ok`: needs sshd; skip with `Assert.Inconclusive` like `MountManagerTest.RequireSsh`.
- `TestSsh_ThroughTunnel_UsesRelay`: the drive is `{_user}@gd-tunnel-test` with `UseTunnel`, the alias maps to `GOLDDRIVE_HOST`, and the default key is in place. Expects `MountStatus.OK` and `relay.Accepted >= 1`.
- `TestPassword_ThroughTunnel_Ok`: with `GOLDDRIVE_PASS`.

- [ ] **Step 2: Run them to verify they fail**

Run: `dotnet test src\test\test.csproj -c Release --filter "FullyQualifiedName~DriveTest|FullyQualifiedName~SettingsTest|FullyQualifiedName~SshServiceTunnelTest"`
Expected: build errors (`TunnelUrl`, `UseTunnel`, `ViaTunnel`).

- [ ] **Step 3: Implement**

- `Drive` and `Settings` get the properties above. `Settings.Load` reads `UseTunnel` with `Convert.ToBoolean`.
- `TestHost` with `ViaTunnel`: `using (var t = new WebSocketTunnel(...)) t.Ensure();` replaces the TCP probe. A `TunnelException` gives `BAD_HOST` with `Error = ex.Message`.
- `TestPassword`, `TestSsh` and `SetupSsh` wrap their `SshClient` in `using (OpenRoute(drive, out host, out port))` and connect to `host`/`port`. User-facing text keeps `drive.Host`. In each catch, a `TunnelException` maps to `BAD_HOST` with its message.
- The `ssh.exe` fallback in `TestSsh` runs inside the same route, using `-p {port} -oHostKeyAlias={alias} {user}@{host}`. `alias` is `drive.Host` when the port is 22 and `[{drive.Host}]:{drive.CurrentPort}` otherwise, which matches the key the driver checks with `libssh2_knownhost_checkp`.

- [ ] **Step 4: Run them to verify they pass**

Run: same as Step 2, then the whole suite with `tools\test.bat`.
Expected: all pass, and the existing direct-SSH tests are unchanged.

- [ ] **Step 5: Commit**

```bash
git add src/app/Common/Drive.cs src/app/Common/Settings.cs src/app/Service/SshService.cs src/test/Unit/DriveTest.cs src/test/Unit/SettingsTest.cs src/test/Tunnel/SshServiceTunnelTest.cs
git commit -m "feat(app): per-drive web tunnel; SSH.NET checks and key setup go through it"
```

---

### Task 6: SSH/WSS choice in the UI, mount end to end, docs

**Files:**
- Modify: `src/app/Controls/SettingsControl.xaml` (a "Web Tunnel" field after "Optional Arguments")
- Modify: `src/app/Controls/DriveControl.xaml` (SSH/WSS radio buttons under the `SelectedDrive.MountPoint` TextBlock)
- Modify: `src/app/ViewModel/MainWindowViewModel.cs` (`CanChooseRoute`, URL validation in `OnSettingsSave`)
- Modify: `src/app/Service/MountService.cs` (`Mount`, `:701`)
- Modify: `readme.md`, `changelog.md`
- Test: `src/test/Service/MountManagerTest.cs`

**Interfaces:**
- Consumes: `Drive.TunnelUrl`, `UseTunnel`, `HasTunnel` (Task 5); `TunnelRelay` (Task 2).
- Produces: `public bool CanChooseRoute => !IsWorking && ConnectButtonText == "Connect";` on `MainWindowViewModel`, notified wherever `IsWorking` or `DriveStatus` change.

- [ ] **Step 1: Write the failing test**

`MountThroughTunnel_CopiesBigFile` (`[TestCategory("Appveyor")]`):
- A drive `T:` with `MountPoint = $"{_user}@gd-tunnel-test"`, `TunnelUrl = relay.Url` and `UseTunnel = true`, with the relay alias mapped to `_host`.
- It uses `T:`, not `X:`. The CLI tests start `golddrive.exe X:` directly, and config.json overrides their `-o tunnel`, so a leftover `X:` entry would aim them at a dead relay. In `finally`, unmount and remove `T:` from config.json (`LoadSettings`, `Drives.Remove("T:")`, `SaveSettings`).
- `_mountService.Connect(drive, null)` gives `CONNECTED`.
- A 20 MB random file copied to `X:\tmp` and back matches by MD5 (reuse `RandomFile`/`Md5`).
- `relay.Accepted >= 2`: the app's checks plus the driver.
- Unmount gives `DISCONNECTED`, and `T:` is gone from config.json.

- [ ] **Step 2: Run it to verify it fails**

Run: `dotnet test src\test\test.csproj -c Release --filter "FullyQualifiedName~MountThroughTunnel"`
Expected: FAIL. `Mount` writes config.json only *after* `net use`, so the driver starts without `UseTunnel`, tries `gd-tunnel-test` directly, and the mount fails.

- [ ] **Step 3: Write the drive to config.json before mounting**

In `Mount`, move `LoadSettings()` / `settings.AddDrive(drive)` / `SaveSettings(settings)` ahead of `net use`, and keep `UpdateDrives(settings)` after it. The driver reads the route from config.json when WinFsp starts it. *(My call: if `net use` then fails, the drive stays saved, the same as a drive saved from the Settings page.)*

- [ ] **Step 4: Run it to verify it passes**

Run: same as Step 2.
Expected: PASS.

- [ ] **Step 5: Add the UI**

- `SettingsControl.xaml`: a TextBlock `Web Tunnel` and a TextBox bound to `SelectedDrive.TunnelUrl`, styled like the Optional Arguments pair, with hint `wss://termita/tunnel`.
- `DriveControl.xaml`: two `RadioButton`s in `GroupName="Route"`, `SSH` (`IsChecked` = `SelectedDrive.UseTunnel` through `NegateBoolConverter`) and `WSS` (`IsChecked` = `SelectedDrive.UseTunnel`). They're visible only when `SelectedDrive.HasTunnel` (`BoolToVis`), with `IsEnabled="{Binding CanChooseRoute}"`.
- `OnSettingsSave`: when `HasTunnel` and the URL doesn't match `^wss?://` (ignore case), set `Message = "Web tunnel must be a ws:// or wss:// URL"` and return without saving. When the URL is cleared, also set `UseTunnel = false`.

- [ ] **Step 6: Run the whole suite and check by hand**

Run: `tools\test.bat`
Expected: all pass.
Then run the app (`build\Release\x64\golddrive-app.exe`) with a drive whose Web Tunnel is a running `TunnelRelay` or the deployed termita:
- The radios appear only once the URL is set.
- They are disabled while the drive is connected.
- WSS connects, and SSH connects directly.
- A 403 shows `termita does not allow …` in the message area.

- [ ] **Step 7: Manual check on a VPN laptop (Review Focus 5)**

This needs termita's `TUNNEL_TARGETS` to list the drive's host exactly as golddrive names it. Over the corporate VPN and proxy:
1. WSS mounts the drive.
2. A 100 MB+ copy each way matches by hash.
3. After sleep and wake, the drive works again without a remount.
4. With SSH chosen on the VPN, the connection fails with "Host does not respond", and never falls back silently.

Record the results in the readme section from Step 8. Anything not verified goes under "Still to verify", the way SEE's `doc/websocket-jump.md` does.

- [ ] **Step 8: Docs**

- `readme.md`, a section "Over termita's web tunnel (VPN)": what WSS does, how to set Web Tunnel, and the termita requirements. `TUNNEL_TARGETS` must name each host exactly as configured. Each mounted drive holds `connections` tunnel slots (default 4) plus one short-lived slot per connect check, so size `TUNNEL_MAX` for that. Credentials never reach termita.
- `changelog.md`: one entry.

- [ ] **Step 9: Commit**

```bash
git add src/app/Controls/SettingsControl.xaml src/app/Controls/DriveControl.xaml src/app/ViewModel/MainWindowViewModel.cs src/app/Service/MountService.cs src/test/Service/MountManagerTest.cs readme.md changelog.md
git commit -m "feat(app): SSH/WSS choice per drive; mount through termita end to end"
git push -u origin feat/wss-tunnel
```
