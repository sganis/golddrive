# Changelog

## 1. .NET Framework 4.5.2 → .NET 8.0 Migration

The biggest theme across the staged changes. Both `src/app/app.csproj` and `src/test/test.csproj` were rewritten from legacy MSBuild format to modern SDK-style targeting `net8.0-windows`. Key consequences:

- **`packages.config` deleted** — replaced by `<PackageReference>` entries with updated versions (MaterialDesignThemes 4→5, NLog 4→5, SSH.NET 2020→2024, etc.)
- **`AssemblyInfo.cs`** — version changed from `2.29.0.*` to `2.29.0.0` (SDK-style projects don't support wildcard versioning)
- **`NLog.config`** — `${windows-identity}` → `${environment-user}` (NLog 5 compatibility), encoding changed to UTF-8

## 2. C/C++ Build Modernization

All native `.vcxproj` files (cli, runapp, sanssh, sanssh-libssh) were updated:

- **PlatformToolset** changed from hardcoded `v140`/`v142` to `$(DefaultPlatformToolset)` — no longer locked to a specific VS version
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

## 9. New Files

- **`CLAUDE.md`** — project documentation for AI assistant context
- **`PLAN.md`** — 5-phase improvement plan (many items above correspond to this plan)
- **`.claude/settings.local.json`** / **`.vscode/settings.json`** — tool/editor config
