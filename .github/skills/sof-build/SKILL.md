---
name: sof-build
description: "Build SOF (Sound Open Firmware) for Intel audio DSPs. Use when: building firmware, running remote build, syncing source to remote machine, triggering VS Code build tasks, understanding build configuration, adding new platform targets, debugging build failures, rsync sync issues, plink SSH tunnel, rimage signing."
---

# SOF Firmware Build Workflow

## Overview

SOF uses a remote Linux build machine (`sofdev`, PuTTY session `sof_dev`). The local Windows
workspace is synced via rsync, then `build_vs.sh` is invoked remotely via plink.

## VS Code Tasks

Trigger via **Ctrl+Shift+B** or `Terminal > Run Task`:

| Task | Action |
|------|--------|
| `ilab: Build` | rsync + incremental build (default) |
| `ilab: Update && Build` | rsync + west update + build |
| `ilab: Clean && Build` | rsync + clean build from scratch |
| `ilab: llext build` | build llext modules only |
| `Build lmdk library` | build a specific lmdk library file |
| `Download elf` | copy .elf from remote |
| `Download build dir` | copy full build output from remote |

All tasks use a single **config picker** (12 options: `ptl`, `mtl`, `lnl`, `tgl`, etc.).

## Build Script

`.vscode/sof-build.ps1` — PowerShell, called by VS Code tasks.

### Parameters
```powershell
-WorkspaceFolder  # Workspace root path
-Action           # build | update | clean | llext | lib
-Config           # Named config (ptl, mtl-fpga, lnl-fpga, ptl-fpga-userspace, ...)
-RelativeFile     # Used only for 'lib' action
```

### rsync Mechanism (daemon + SSH tunnel)

MSYS rsync (from VS bundle) cannot use Win32 plink as pipe transport. Workaround:

1. Start `rsync --daemon` on remote via `plink -batch sof_dev "sh -c 'cat > /tmp/rsyncd_vscode_8873.conf << EOF...'"` — port 8873
2. Open SSH tunnel: `Start-Process plink -ArgumentList "-batch -L 8873:localhost:8873 sof_dev -N"`
3. MSYS rsync connects over TCP: `rsync rsync://localhost:8873/sof/`
4. Cleanup: kill daemon PID + tunnel process

### rsync Options
```
-azv -8 --delete
--exclude '.vs/'
--exclude '.vscode/'
--exclude '__pycache__/'
```
- `-a` = archive (recursive + preserve times/permissions)
- `-z` = compress
- `-v` = verbose (show transferred files)
- `-8` = 8-bit output (non-ASCII filenames)
- `--delete` = remove remote files absent locally

### Local Path Format
MSYS rsync requires `/cygdrive/<drive>/<path>/` (e.g., `/cygdrive/c/src/git/zephyr/sof/`).
`C:\...` is treated as a remote host. Convert with:
```powershell
$drive   = $WorkspaceFolder[0].ToString().ToLower()
$relPath = $WorkspaceFolder.Substring(3).Replace('\', '/')
$cygSrc  = "/cygdrive/$drive/$relPath/"
```

### Remote Build Command
```bash
~/build_vs.sh -d ~/git/zephyr/sof -p <target> [overlay args]
```

## Config Lookup Table

| Config name | Platform target | Overlay |
|-------------|----------------|---------|
| ptl | ptl | — |
| ptl-fpga | ptl | `-o overlays/ptl/fpga_overlay.conf` |
| ptl-fpga-userspace | ptl | fpga + userspace overlays |
| ptl-sim | ptl-sim | — |
| mtl | mtl | — |
| mtl-fpga | mtl | `-o overlays/mtl/fpga_overlay.conf` |
| lnl | lnl | — |
| lnl-fpga | lnl | `-o overlays/lnl/fpga_overlay.conf` |
| tgl | tgl | — |
| wcl-fpga | wcl | `-o overlays/wcl/fpga_overlay.conf` |
| nvl | nvl | — |
| nvl-fpga | nvl | `-o overlays/nvl/fpga_overlay.conf` |

## Remote Machine Details

| Property | Value |
|----------|-------|
| Hostname | `sofdev` |
| PuTTY session | `sof_dev` |
| Remote user | `awarecki` |
| Remote project dir | `~/git/zephyr/sof` |
| SSH key (PuTTY) | `C:\Users\awarecki\OneDrive - Intel Corporation\Documents\Klucze\dev_private.ppk` |
| rsync binary | `C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\VC\Linux\bin\rsync\rsync.exe` |
| plink | `C:\Program Files\PuTTY\plink.exe` (in PATH) |

## Adding a New Platform Config

1. Add entry to `$configs` hashtable in `.vscode/sof-build.ps1`
2. Add matching `pickString` option to the `config` input in `.vscode/tasks.json`
3. Mirror the corresponding entry in `CppProperties.json` for IntelliSense

## Update Build

Use **`ilab: Update && Build`** (action `update`) to run `west update` on the remote before building.

**When to use:**
- After changing `west.yml` (manifest) — e.g. bumping Zephyr or HAL revision
- When the remote Zephyr workspace is out of sync with the manifest
- Does NOT clean the build directory — incremental build follows the update

**How to run:**
- VS Code: `Terminal > Run Task > ilab: Update && Build`, pick config (e.g. `ptl`)
- Terminal: `powershell -ExecutionPolicy Bypass -File .vscode\sof-build.ps1 -WorkspaceFolder <root> -Action update -Config ptl`

**Notes:**
- `west update` runs non-interactively on remote, no input required — just takes time
- Success recognized by `#################### COPY ARTIFACTS ####################` + exit code 0

## Clean Build

Use **`ilab: Clean && Build`** (action `clean`) to delete the remote build directory and rebuild from scratch.

**When to use:**
- After structural CMake or Kconfig changes that incremental builds won't pick up
- When the build state is suspected corrupt
- After resolving compile errors that left partial object files

**How to run:**
- VS Code: `Terminal > Run Task > ilab: Clean && Build`, pick config (e.g. `ptl`)
- Terminal: `powershell -ExecutionPolicy Bypass -File .vscode\sof-build.ps1 -WorkspaceFolder <root> -Action clean -Config ptl`

**Recognizing success:**
The build ends with:
```
#################### COPY ARTIFACTS ####################
```
followed by the artifact tree (`sof-ptl.ri`, `sof-ptl-openmodules.ri`, llext modules, etc.) and exit code 0.

**Note:** Clean build takes significantly longer than incremental — it recompiles all ~530 objects.

## Common Build Failures

| Error | Cause | Fix |
|-------|-------|-----|
| `@ERROR: chdir failed` | rsyncd.conf path not expanded | Use `$HOME` not `~/` in daemon conf |
| `both remote` rsync error | Local path uses `C:\` (treated as host) | Use `/cygdrive/c/...` format |
| `Device or resource busy` on `.vs/` | VS locks index files | Exclude `.vs/` — already in script |
| Compile error in C file | Bad code in source | Fix source, re-sync (rsync sends only changed files) |
