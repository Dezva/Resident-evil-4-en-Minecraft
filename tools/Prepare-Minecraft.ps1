function Invoke-MinecraftPreparation([string]$GameDir,[string]$Mode='keep') {
    if ($Mode -cnotin @('keep','survival','creative')) {throw 'Modo de juego no valido.'}
    $file=Join-Path $GameDir 'config\re4bridge.json'
    if (!(Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).Length -gt 4096) { throw 'Abre Minecraft con el mod nuevo antes de Preparar.' }
    $config=Get-Content -LiteralPath $file -Raw | ConvertFrom-Json
    if ($config.protocol -ne 1 -or ($config.port -isnot [int] -and $config.port -isnot [long]) -or $config.port -lt 1024 -or $config.port -gt 65535 -or $config.token -isnot [string] -or $config.token -cnotmatch '^[0-9a-f]{64}$') { throw 'Configuracion del puente no valida.' }
    $port=if ($config.port -eq 65535) {65534} else {$config.port+1}
    $request=@{protocol=1;type='prepare';token=$config.token;game_mode=$Mode} | ConvertTo-Json -Compress
    return [RE4BridgeView.PreparationCall]::new($port,$request)
}
function Complete-MinecraftPreparation($Call,[string]$ExpectedMode='keep') {
    $reply=$Call.Result() | ConvertFrom-Json
    if ($reply.protocol -ne 1 -or $reply.ok -isnot [bool] -or $reply.message -isnot [string] -or $reply.message.Length -gt 1000) { throw 'Respuesta de preparacion no valida.' }
    if (!$reply.ok) {throw $reply.message}
    if ($ExpectedMode -ne 'keep' -and $reply.game_mode -cne $ExpectedMode) {throw 'Actualiza el mod de Minecraft: no confirmo el modo solicitado.'}
    return $reply.message
}
