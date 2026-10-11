$ErrorActionPreference='Stop'
. (Join-Path (Split-Path $PSScriptRoot -Parent) 'COLLECT-RE4-CRASH.ps1') -FunctionsOnly
function Make-Event($id,$text) {
    $event=[pscustomobject]@{Id=$id;TimeCreated=[datetime]'2026-10-11T08:00:00Z';Xml=$text}
    $event | Add-Member ScriptMethod ToXml { return $this.Xml }
    return $event
}
$event=Make-Event 1000 '<Event><EventData><Data Name="AppName">bio4.exe</Data><Data Name="ModuleName">dinput8.dll</Data><Data Name="ExceptionCode">c0000005</Data><Data Name="FaultingOffset">00001337</Data><Data Name="PrivateField">secret</Data></EventData></Event>'
$report=Convert-RE4CrashEvent $event
if ($report.id -ne 1000 -or $report.details.ModuleName -ne 'dinput8.dll' -or $report.details.ExceptionCode -ne 'c0000005' -or $report.details.FaultingOffset -ne '00001337') { throw 'Fault details lost' }
if ($report.details.Contains('PrivateField')) { throw 'Unapproved event field exported' }
$wer=Make-Event 1001 '<Event><EventData><Data Name="EventName">APPCRASH</Data><Data Name="P1">bio4.exe</Data><Data Name="P4">d3d9.dll</Data><Data Name="P7">c0000005</Data><Data Name="P8">00009999</Data><Data Name="StorePath">private archive</Data></EventData></Event>'
$report=Convert-RE4CrashEvent $wer
if ($report.details.P4 -ne 'd3d9.dll' -or $report.details.P8 -ne '00009999' -or $report.details.Contains('StorePath')) { throw 'WER details/filter wrong' }
$other=Make-Event 1000 '<Event><EventData><Data Name="AppName">other.exe</Data></EventData></Event>'
if ($null -ne (Convert-RE4CrashEvent $other)) { throw 'Unrelated process exported' }
$missing=Make-Event 1000 '<Event><EventData><Data Name="ModuleName">bio4.exe</Data></EventData></Event>'
if ($null -ne (Convert-RE4CrashEvent $missing)) { throw 'Missing app identity accepted' }
Write-Host '4 crash-event fixtures passed: Application Error, WER, unrelated process, missing identity. Windows event querying not executed on Linux.'
