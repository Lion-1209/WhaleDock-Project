# 鲸屿 WhaleDock · TLS 证书包一键修复
# 适用场景：本机装了 Watt Toolkit (Steam++) 且开启"网络加速"时，github.com 被本地
# MITM 重签证书，PlatformIO / pip / requests (Python OpenSSL 栈) 一律报
# "certificate verify failed: unable to get local issuer certificate"。
#
# 原理：把 Windows 证书库（含 Watt Toolkit 装入的 SteamTools 根证书）全部导出为
# PEM 证书包，并追加进 PlatformIO penv 的 certifi，让所有验证路径都能信任本机链。
# 只影响 ~/.platformio 隔离环境与生成的 PEM 文件，不改动系统证书库。
#
# 用法：管理员或普通权限 PowerShell 均可
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\fix-tls-bundle.ps1
# 然后重开终端 / 完全重启 VS Code 再跑 pio run。

$ErrorActionPreference = 'Stop'

$bundle = "$env:USERPROFILE\ca-bundle-windows.pem"
$stores = @(
  'Cert:\LocalMachine\Root', 'Cert:\LocalMachine\CA', 'Cert:\LocalMachine\AuthRoot',
  'Cert:\CurrentUser\Root', 'Cert:\CurrentUser\CA', 'Cert:\CurrentUser\AuthRoot'
)

# 1. 导出全部根/中间证书（按指纹去重）
$seen = @{}
$out = New-Object System.Text.StringBuilder
$count = 0
foreach ($s in $stores) {
  Get-ChildItem $s -ErrorAction SilentlyContinue | ForEach-Object {
    if (-not $seen.ContainsKey($_.Thumbprint)) {
      $seen[$_.Thumbprint] = $true
      [void]$out.AppendLine('-----BEGIN CERTIFICATE-----')
      [void]$out.AppendLine([Convert]::ToBase64String($_.RawData, 'InsertLineBreaks'))
      [void]$out.AppendLine('-----END CERTIFICATE-----')
      $count++
    }
  }
}
[System.IO.File]::WriteAllText($bundle, $out.ToString())
Write-Host ("[1/3] exported " + $count + " certs -> " + $bundle)

# 2. 常驻环境变量（对以后新开的终端/VS Code 生效）
[Environment]::SetEnvironmentVariable('REQUESTS_CA_BUNDLE', $bundle, 'User')
[Environment]::SetEnvironmentVariable('SSL_CERT_FILE', $bundle, 'User')
Write-Host "[2/3] set user env REQUESTS_CA_BUNDLE / SSL_CERT_FILE"

# 3. 追加进 penv 的 certifi（PlatformIO 内部某些下载路径不走上面的环境变量）
$certifi = "$env:USERPROFILE\.platformio\penv\Lib\site-packages\certifi\cacert.pem"
if (Test-Path $certifi) {
  Copy-Item $certifi "$certifi.bak" -Force
  $orig = (Select-String -Path $certifi -Pattern 'BEGIN CERTIFICATE').Count
  Add-Content -Path $certifi -Value ([System.IO.File]::ReadAllText($bundle)) -Encoding ascii
  $now = (Select-String -Path $certifi -Pattern 'BEGIN CERTIFICATE').Count
  Write-Host ("[3/3] certifi: " + $orig + " -> " + $now + " certs (backup: cacert.pem.bak)")
} else {
  Write-Host "[3/3] penv certifi not found (PlatformIO 未安装？跳过)"
}

Write-Host "完成。重开终端 / 完全重启 VS Code 后重试 pio run。"
