param()
$ErrorActionPreference = 'Stop'
$session = $null
$viewLock = $null
$ownsLock = $false
try {
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
    $form.ClientSize = [System.Drawing.Size]::new(425,145)
    $form.FormBorderStyle = 'FixedToolWindow'; $form.TopMost = $true
    $form.ShowInTaskbar=$false
    $form.StartPosition = 'Manual'; $form.Location = [System.Drawing.Point]::new($screen.Left+12,$screen.Top+12)
    $label = New-Object System.Windows.Forms.Label
    $label.Location=[System.Drawing.Point]::new(10,10); $label.Size=[System.Drawing.Size]::new(405,65)
    $label.Text="RE4 en ventana, Minecraft abierto sin minimizar.`r`nPrepara controller/arm en Minecraft. F9 muestra RE4.`r`nF8 detiene el puente y devuelve control directo a RE4."
    $form.Controls.Add($label)
    $prepare = New-Object System.Windows.Forms.Button
    $prepare.Text='Preparar Minecraft'; $prepare.Location=[System.Drawing.Point]::new(10,83); $prepare.Size=[System.Drawing.Size]::new(160,45)
    $prepare.Add_Click({ try { $session.Prepare() } catch { $label.Text=$_.Exception.Message } })
    $normal = New-Object System.Windows.Forms.Button
    $normal.Text='Volver a RE4 normal (F8)'; $normal.Location=[System.Drawing.Point]::new(180,83); $normal.Size=[System.Drawing.Size]::new(235,45)
    function Return-ToNormal {
        try {
            Set-ViewPanel $false
            $session.Normal($true)
            $label.Text='RE4 normal. Para volver al puente: Preparar Minecraft, arm y F9.'
            if (!$session.StopConfirmed) { $label.Text+=' IPC no disponible: el corte depende de perder foco/TTL.' }
        } catch { $label.Text='Error de retorno: '+$_.Exception.Message }
    }
    $normal.Add_Click({ Return-ToNormal })
    $form.Controls.Add($prepare); $form.Controls.Add($normal)
    function Set-ViewPanel([bool]$compact) {
        $label.Visible=!$compact; $prepare.Visible=!$compact
        if ($compact) { $form.Hide() } else { $form.Show(); $api.RaiseControls($form.Handle) }
        if ($compact) {
            $form.ClientSize=[System.Drawing.Size]::new(245,55)
            $normal.Location=[System.Drawing.Point]::new(5,5)
        } else {
            $form.ClientSize=[System.Drawing.Size]::new(425,145)
            $normal.Location=[System.Drawing.Point]::new(180,83)
        }
    }
    $timer = New-Object System.Windows.Forms.Timer
    $timer.Interval=40
    $keys = @{F8=$false; F9=$false}
    $timer.Add_Tick({
        $f8=([RE4BridgeView.WindowsDesktop]::GetAsyncKeyState(0x77) -band 0x8000) -ne 0
        $f9=([RE4BridgeView.WindowsDesktop]::GetAsyncKeyState(0x78) -band 0x8000) -ne 0
        try {
            if ($f8 -and !$keys.F8) { Return-ToNormal }
            elseif ($f9 -and !$keys.F9) {
                if ($session.Active) { Set-ViewPanel $false; $session.Prepare(); $label.Text='Minecraft visible. Requiere otro arm; F9 vuelve a mostrar RE4.' }
                else { $session.Enter($bounds); Set-ViewPanel $true; $label.Text='Vista RE4 activa. Minecraft recibe los inputs. F8 vuelve a RE4 normal.' }
            } elseif ($session.Active -and !$session.StillFocused()) {
                $session.Normal($false); Set-ViewPanel $false; $label.Text='Modo detenido al perder foco. Preparar Minecraft, arm y F9 para volver.'
            }
        } catch { $label.Text=$_.Exception.Message }
        $keys.F8=$f8; $keys.F9=$f9
    })
    $form.Add_FormClosing({ $timer.Stop() })
    $timer.Start()
    [System.Windows.Forms.Application]::Run($form)
    $timer.Stop(); $timer.Dispose(); $form.Dispose()
} catch { Write-Host ('ERROR: '+$_.Exception.Message); exit 1 }
finally {
    if ($null -ne $session) {
        try { $session.Normal($false) } catch { Write-Host ('No se pudo restaurar la ventana: '+$_.Exception.Message) }
    }
    if ($null -ne $viewLock) {
        if ($ownsLock) { $viewLock.ReleaseMutex() }
        $viewLock.Dispose()
    }
}
