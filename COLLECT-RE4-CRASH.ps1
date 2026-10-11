param([string]$OutputDir = $PSScriptRoot, [switch]$FunctionsOnly)
$ErrorActionPreference = 'Stop'

function Convert-RE4CrashEvent($Event) {
    [xml]$xml = $Event.ToXml()
    $data = @{}
    foreach ($item in $xml.Event.EventData.Data) {
        if ($item.Name) { $data[[string]$item.Name] = [string]$item.'#text' }
    }
    $app = if ($Event.Id -eq 1000) { $data['AppName'] } else { $data['P1'] }
    if ($app -notmatch '(?i)(^|[\\/])bio4\.exe$') { return }
    # Whitelist technical fields; do not export messages, dumps or WER archives.
    $fields = if ($Event.Id -eq 1000) {
        @('AppName','AppVersion','ModuleName','ModuleVersion','ExceptionCode','FaultingOffset','AppPath','ModulePath')
    } else { @('EventName','P1','P2','P4','P5','P7','P8') }
    $details = [ordered]@{}
    foreach ($field in $fields) {
        if ($data.ContainsKey($field)) { $details[$field] = $data[$field] }
    }
    [pscustomobject]@{ time_utc=$Event.TimeCreated.ToUniversalTime().ToString('o'); id=$Event.Id; details=$details }
}

if ($FunctionsOnly) { return }
try {
    # Read previously recorded Application Error / Windows Error Reporting events.
    # No process access, restart, memory writes, registry or Defender changes.
    $queryErrors = @()
    $events = @(Get-WinEvent -FilterHashtable @{
        LogName='Application'; Id=1000,1001; StartTime=(Get-Date).AddDays(-7)
    } -MaxEvents 1000 -ErrorAction SilentlyContinue -ErrorVariable queryErrors)
    $unexpected = @($queryErrors | Where-Object { $_.FullyQualifiedErrorId -notlike 'NoMatchingEventsFound*' })
    if ($unexpected.Count) { throw $unexpected[0] }
    $crashes = @($events | ForEach-Object { Convert-RE4CrashEvent $_ })
    $report = [ordered]@{ mode='READ ONLY EVENT LOG'; diagnostic='0.4.14'; generated_utc=(Get-Date).ToUniversalTime().ToString('o'); days=7; events=$crashes }
    $path = Join-Path (Resolve-Path -LiteralPath $OutputDir).Path ('re4-crash-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.json')
    $report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $path -Encoding UTF8
    Write-Host "Informe guardado: $path"
    Write-Host "Fallos de bio4 encontrados: $($crashes.Count). No hace falta provocar otro cierre."
    exit 0
} catch {
    Write-Host "ERROR al leer el registro: $($_.Exception.Message)"
    Write-Host 'No se modifico RE4. Si Windows deniega acceso, ejecuta este colector como administrador.'
    exit 1
}
