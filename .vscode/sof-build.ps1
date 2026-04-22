param(
    [Parameter(Mandatory)][string]$WorkspaceFolder,
    [Parameter(Mandatory)][ValidateSet('build', 'update', 'clean', 'llext', 'lib')][string]$Action,
    [Parameter(Mandatory)][string]$Config,
    [string]$RelativeFile = ""
)

$ErrorActionPreference = "Stop"

# --- Configuration name -> (target, overlay) lookup ---
# Mirrors the named configurations from CppProperties.json
$configs = @{
    "mtl"                = @{ target = "mtl";     overlay = "" }
    "mtl-fpga"           = @{ target = "mtl";     overlay = "-o overlays/mtl/fpga_overlay.conf" }
    "lnl"                = @{ target = "lnl";     overlay = "" }
    "lnl-fpga"           = @{ target = "lnl";     overlay = "-o overlays/lnl/fpga_overlay.conf" }
    "ptl"                = @{ target = "ptl";     overlay = "" }
    "ptl-fpga"           = @{ target = "ptl";     overlay = "-o overlays/ptl/fpga_overlay.conf" }
    "ptl-fpga-userspace" = @{ target = "ptl";     overlay = "-o overlays/ptl/fpga_overlay.conf -o overlays/ptl/userspace_overlay.conf" }
    "ptl-sim"            = @{ target = "ptl-sim"; overlay = "" }
    "tgl"                = @{ target = "tgl";     overlay = "" }
    "wcl-fpga"           = @{ target = "wcl";     overlay = "-o overlays/wcl/fpga_overlay.conf" }
    "nvl"                = @{ target = "nvl";     overlay = "" }
    "nvl-fpga"           = @{ target = "nvl";     overlay = "-o overlays/nvl/fpga_overlay.conf" }
}

if (-not $configs.ContainsKey($Config)) {
    Write-Error "Unknown config: '$Config'. Valid options: $($configs.Keys -join ', ')"
    exit 1
}

$Target  = $configs[$Config].target
$Overlay = $configs[$Config].overlay

# --- Remote build settings ---
$rsync        = "C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\VC\Linux\bin\rsync\rsync.exe"
$plinkSession = "sof_dev"          # PuTTY saved session name for plink (has host/user/key)
$remoteDir    = "~/git/zephyr/sof" # Project directory on the remote machine

# --- rsync sync via remote daemon + SSH tunnel ---
# MSYS rsync (VS bundle) cannot use Win32 plink/ssh as a pipe transport (handle incompatibility).
# Workaround: start a temporary rsync daemon on remote via plink, tunnel its port locally via
# plink -L, then MSYS rsync connects over TCP - no pipes involved, no compatibility issues.
$rsyncPort  = 8873
$drive      = $WorkspaceFolder[0].ToString().ToLower()
$relPath    = $WorkspaceFolder.Substring(2).Replace('\', '/')
$localSrc   = "/$drive$relPath/"
$remoteConf = "/tmp/rsyncd_vscode_${rsyncPort}.conf"

# Create temp rsyncd.conf and start daemon on remote; capture its PID.
# NOTE: rsyncd.conf 'path' does not expand tilde - use $HOME explicitly.
# The config is written via 'sh -c' to allow $HOME expansion and avoid
# printf unicode-escape interpretation (\u in 'use chroot').
$daemonCmd = "sh -c 'cat > $remoteConf << EOF" + [char]10 + `
             "[sof]" + [char]10 + `
             "    path = `$HOME/git/zephyr/sof" + [char]10 + `
             "    read only = no" + [char]10 + `
             "    use chroot = no" + [char]10 + `
             "EOF" + [char]10 + `
             "rsync --daemon --no-detach --port=$rsyncPort --config=$remoteConf & echo `$!'"

Write-Host ""
Write-Host "==> Starting remote rsync daemon on port $rsyncPort..."
$remotePid = (plink -batch $plinkSession $daemonCmd).Trim()
Write-Host "    Remote daemon PID: $remotePid"

Write-Host "==> Opening SSH tunnel localhost:${rsyncPort} -> remote:${rsyncPort}..."
$tunnelProc = Start-Process plink `
    -ArgumentList "-batch -L ${rsyncPort}:localhost:${rsyncPort} $plinkSession -N" `
    -PassThru -WindowStyle Hidden

# Give the daemon and tunnel a moment to be ready
Start-Sleep -Seconds 2

try {
    # MSYS rsync treats 'X:\path' as remote (interprets X: as hostname).
    # Use /cygdrive/x/path format which MSYS rsync recognizes as a local path.
    $drive      = $WorkspaceFolder[0].ToString().ToLower()
    $relPath    = $WorkspaceFolder.Substring(3).Replace('\', '/')
    $cygSrc     = "/cygdrive/$drive/$relPath/"
    Write-Host "==> rsync: $cygSrc -> rsync://localhost:${rsyncPort}/sof/"
    & $rsync -azv -8 --delete --exclude '.vs/' --exclude '.vscode/' --exclude '__pycache__/' $cygSrc "rsync://localhost:${rsyncPort}/sof/"
    if ($LASTEXITCODE -ne 0) {
        Write-Error "rsync failed (exit code: $LASTEXITCODE)"
        exit $LASTEXITCODE
    }
} finally {
    Write-Host "==> Cleaning up: killing remote daemon (PID $remotePid) and SSH tunnel..."
    if ($remotePid) { plink -batch $plinkSession "kill $remotePid 2>/dev/null; rm -f $remoteConf" | Out-Null }
    if (-not $tunnelProc.HasExited) { $tunnelProc.Kill() }
}

Write-Host ""
Write-Host "==> Building: config=$Config (target=$Target), action=$Action"

switch ($Action) {
    'build'  { $cmd = "~/build_vs.sh -d $remoteDir -p $Target $Overlay".Trim() }
    'update' { $cmd = "~/build_vs.sh -d $remoteDir -p $Target -u $Overlay".Trim() }
    'clean'  { $cmd = "~/build_vs.sh -d $remoteDir -p $Target -c $Overlay".Trim() }
    'llext'  { $cmd = "~/build_vs.sh -d $remoteDir -p $Target -c -o module_overlay" }
    'lib'    { $cmd = "~/build_lib_vs.sh '$remoteDir' '$RelativeFile' $Target" }
}

Write-Host "==> Command: $cmd"
plink -batch $plinkSession $cmd
exit $LASTEXITCODE
