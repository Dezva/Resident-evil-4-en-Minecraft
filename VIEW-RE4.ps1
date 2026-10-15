param([string]$GameDir=(Join-Path $env:APPDATA '.minecraft'))
$ErrorActionPreference = 'Stop'
$session = $null
$viewLock = $null
$ownsLock = $false
$script:prepareCall=$null
$script:waitingForMinecraft=$false
$script:preparedMode='keep'
try {
    . (Join-Path $PSScriptRoot 'tools\Prepare-Minecraft.ps1')
    Add-Type -AssemblyName System.Windows.Forms
    Add-Type -AssemblyName System.Drawing
    Add-Type -Path (Join-Path $PSScriptRoot 'tools\RE4View.cs')
    Add-Type -ReferencedAssemblies @([System.Windows.Forms.Form].Assembly.Location,[System.Drawing.Size].Assembly.Location,[System.ComponentModel.Component].Assembly.Location) -TypeDefinition @'
public class RE4BridgeViewPanel : System.Windows.Forms.Form {
    protected override bool ShowWithoutActivation { get { return true; } }
}
'@
    $games = @(Get-Process bio4 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne [IntPtr]::Zero })
    $clients = @(Get-Process java,javaw -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowTitle -like 'Minecraft*' -and $_.MainWindowHandle -ne [IntPtr]::Zero })
    if ($games.Count -ne 1) { throw 'Abre una sola instancia de RE4, en modo ventana y con partida cargada.' }
    if ($clients.Count -ne 1) { throw 'Abre una sola ventana de Minecraft antes de VIEW-RE4.' }
    $game = $games[0]; $client = $clients[0]
    $viewLock = [System.Threading.Mutex]::new($false,('Local\RE4Bridge-View-'+$game.Id))
    $ownsLock = $viewLock.WaitOne(0)
    if (!$ownsLock) { throw 'Ya hay un panel VIEW-RE4 para este juego. Usa el panel existente.' }
    $api = New-Object RE4BridgeView.WindowsDesktop
    $session = [RE4BridgeView.Session]::new($api,$game.MainWindowHandle,[uint32]$game.Id,$client.MainWindowHandle,[uint32]$client.Id)
    $screen = [System.Windows.Forms.Screen]::FromHandle($game.MainWindowHandle).Bounds
    $bounds = New-Object RE4BridgeView.Rect
    $bounds.Left=$screen.Left; $bounds.Top=$screen.Top; $bounds.Right=$screen.Right; $bounds.Bottom=$screen.Bottom
    $form = New-Object RE4BridgeViewPanel
    $form.Text = 'RE4 Bridge - Vista y retorno'
    $form.ClientSize = [System.Drawing.Size]::new(425,178)
    $form.FormBorderStyle = 'FixedToolWindow'; $form.TopMost = $true
    $form.ShowInTaskbar=$false
    $form.StartPosition = 'Manual'; $form.Location = [System.Drawing.Point]::new($screen.Left+12,$screen.Top+12)
    $label = New-Object System.Windows.Forms.Label
    $label.Location=[System.Drawing.Point]::new(10,10); $label.Size=[System.Drawing.Size]::new(405,65)
    $label.Text="Abre una partida RE4 y un mundo Minecraft local.`r`nElige el modo y pulsa Preparar Minecraft; F9 muestra RE4.`r`nE inventario; R interactuar RE4; Esc menu; F5 modo; F8 RE4 normal."
    $form.Controls.Add($label)
    $modeLabel=New-Object System.Windows.Forms.Label
    $modeLabel.Text='Modo:'; $modeLabel.Location=[System.Drawing.Point]::new(10,80); $modeLabel.Size=[System.Drawing.Size]::new(45,22)
    $mode=New-Object System.Windows.Forms.ComboBox
    $mode.DropDownStyle='DropDownList'; $mode.Location=[System.Drawing.Point]::new(60,77); $mode.Size=[System.Drawing.Size]::new(210,25)
    [void]$mode.Items.AddRange(@('Mantener modo del mundo','Supervivencia','Creativo')); $mode.SelectedIndex=0
    $form.Controls.Add($modeLabel); $form.Controls.Add($mode)
    $prepare = New-Object System.Windows.Forms.Button
    $prepare.Text='Preparar Minecraft'; $prepare.Location=[System.Drawing.Point]::new(10,116); $prepare.Size=[System.Drawing.Size]::new(160,45)
    function Write-ViewEvent([string]$Message) {
        try {
            $file=Join-Path $PSScriptRoot 'view-events.log'
            if ((Test-Path -LiteralPath $file) -and (Get-Item -LiteralPath $file).Length -gt 1MB) {Move-Item -LiteralPath $file -Destination ($file+'.1') -Force}
            Add-Content -LiteralPath $file -Value ((Get-Date -Format o)+' '+$Message)
        } catch {}
    }
    function Begin-MinecraftPreparation {
        $script:waitingForMinecraft=$false
        try {$script:prepareCall=Invoke-MinecraftPreparation $GameDir $script:preparedMode;$label.Text='Preparando Minecraft... F8 cancela.'}
        catch {$session.Normal($false);$label.Text=$_.Exception.Message}
    }
    function Continue-MinecraftPreparation {
        if (!$script:waitingForMinecraft) {return}
        if (!$session.Prepared) {$script:waitingForMinecraft=$false;return}
        if ($session.InputFocused) { Begin-MinecraftPreparation }
    }
    function Prepare-All {
        if ($null -ne $script:prepareCall -or $script:waitingForMinecraft) {return}
        $script:preparedMode=@('keep','survival','creative')[$mode.SelectedIndex]
        Write-ViewEvent ('Preparar Minecraft; modo='+$script:preparedMode)
        $session.Prepare($bounds)
        if ($session.InputFocused) { Begin-MinecraftPreparation }
        else {
            $script:waitingForMinecraft=$true
            $label.Text='Vista ajustada. Pulsa la ventana de Minecraft: la preparacion continuara automaticamente. F8 cancela.'
        }
    }
    $prepare.Add_Click({ try { Prepare-All } catch { $label.Text=$_.Exception.Message } })
    $normal = New-Object System.Windows.Forms.Button
    $normal.Text='Volver a RE4 normal (F8)'; $normal.Location=[System.Drawing.Point]::new(180,116); $normal.Size=[System.Drawing.Size]::new(235,45)
    function Return-ToNormal {
        Write-ViewEvent 'Retorno solicitado (F8/panel).'
        $script:waitingForMinecraft=$false
        if ($null -ne $script:prepareCall) {$script:prepareCall.Dispose();$script:prepareCall=$null}
        try {
            Set-ViewPanel $false
            $session.Normal($true)
            $label.Text='RE4 normal. Para volver al puente: Preparar Minecraft y F9.'
            if (!$session.FocusConfirmed) { $label.Text='Vista restaurada. Windows no permitio dar foco: pulsa la ventana de RE4. Para volver: Preparar Minecraft y F9.' }
            if (!$session.StopConfirmed) { $label.Text+=' IPC no disponible: el corte depende de perder foco/TTL.' }
        } catch { $label.Text='Error de retorno: '+$_.Exception.Message }
    }
    $normal.Add_Click({ Return-ToNormal })
    $form.Controls.Add($prepare); $form.Controls.Add($normal)
    function Set-ViewPanel([bool]$compact) {
        $label.Visible=!$compact; $prepare.Visible=!$compact
        $mode.Visible=!$compact; $modeLabel.Visible=!$compact
        if ($compact) { $form.Hide() } else { $form.Show(); $api.RaiseControls($form.Handle) }
        if ($compact) {
            $form.ClientSize=[System.Drawing.Size]::new(245,55)
            $normal.Location=[System.Drawing.Point]::new(5,5)
        } else {
            $form.ClientSize=[System.Drawing.Size]::new(425,178)
            $normal.Location=[System.Drawing.Point]::new(180,116)
        }
    }
    $script:viewFocusSuspended=$false
    function Monitor-ViewFocus {
        $reason=$session.Interruption()
        if ($null -eq $reason) {
            if ($script:viewFocusSuspended -ne $session.FocusSuspended) {
                $script:viewFocusSuspended=$session.FocusSuspended
                Write-ViewEvent $(if ($session.FocusSuspended) {'Foco transitorio de Windows; se conserva la vista hasta 200ms, sin reenfocar.'} else {'Minecraft recupero el foco; vista conservada.'})
            }
            return
        }
        Write-ViewEvent $reason
        $session.Normal($false); Set-ViewPanel $false; $label.Text=$reason+' Preparar Minecraft y F9 para volver.'
        $script:viewFocusSuspended=$false
    }
    $timer = New-Object System.Windows.Forms.Timer
    $timer.Interval=16
    $keys = @{F8=$false; F9=$false}
    $timer.Add_Tick({
        $f8=([RE4BridgeView.WindowsDesktop]::GetAsyncKeyState(0x77) -band 0x8000) -ne 0
        $f9=([RE4BridgeView.WindowsDesktop]::GetAsyncKeyState(0x78) -band 0x8000) -ne 0
        try {
            if (!$f8) { Continue-MinecraftPreparation }
            if (!$f8 -and $null -ne $script:prepareCall -and $script:prepareCall.Completed) {
                try {$label.Text=Complete-MinecraftPreparation $script:prepareCall $script:preparedMode}
                catch {$session.Normal($false);$label.Text=$_.Exception.Message}
                finally {$script:prepareCall.Dispose();$script:prepareCall=$null}
            }
            if ($session.Active) { $session.PumpMouse() }
            if ($f8 -and !$keys.F8) { Return-ToNormal }
            elseif ($f9 -and !$keys.F9) {
                if ($script:waitingForMinecraft) {$label.Text='Pulsa la ventana de Minecraft para terminar la preparacion; despues F9. F8 cancela.'}
                elseif ($null -ne $script:prepareCall) {$label.Text='Espera a que termine Preparar Minecraft; F8 cancela.'}
                elseif ($session.Active) { Set-ViewPanel $false; Prepare-All }
                else { $session.Enter($bounds); Write-ViewEvent 'Vista RE4 activada con F9.'; Set-ViewPanel $true; $label.Text='Vista RE4 activa. Minecraft recibe los inputs. F8 vuelve a RE4 normal.' }
            } elseif ($session.Active) { Monitor-ViewFocus }
        } catch { $label.Text=$_.Exception.Message }
        $keys.F8=$f8; $keys.F9=$f9
    })
    $form.Add_FormClosing({ $timer.Stop() })
    $timer.Start()
    [System.Windows.Forms.Application]::Run($form)
    $timer.Stop(); $timer.Dispose(); $form.Dispose()
} catch { Write-Host ('ERROR: '+$_.Exception.Message); exit 1 }
finally {
    if ($null -ne $script:prepareCall) {$script:prepareCall.Dispose();$script:prepareCall=$null}
    if ($null -ne $session) {
        try { $session.Normal($false) } catch { Write-Host ('No se pudo restaurar la ventana: '+$_.Exception.Message) }
    }
    if ($null -ne $viewLock) {
        if ($ownsLock) { $viewLock.ReleaseMutex() }
        $viewLock.Dispose()
    }
}
