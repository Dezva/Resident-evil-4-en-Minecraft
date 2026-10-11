$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$fixture = @'
namespace RE4BridgeView {
public class FakeViewDesktop : Desktop {
    public System.Collections.Generic.List<string> Calls = new System.Collections.Generic.List<string>();
    public IntPtr Current = new IntPtr(2);
    public bool GameAlive = true, FailPresent, FailRestore, DenyFocus;
    public bool SameWindow(IntPtr h,uint pid) { return h == new IntPtr(1) ? GameAlive && pid == 10 : h == new IntPtr(2) && pid == 20; }
    public IntPtr Foreground() { return Current; }
    public Snapshot Capture(IntPtr h,uint pid) { Calls.Add("capture"); return new Snapshot {Handle=h,Pid=pid}; }
    public void Present(Snapshot s,Rect r) { Calls.Add("present"); if(FailPresent) throw new Exception("partial present"); }
    public void Restore(Snapshot s) { Calls.Add("restore"); if(FailRestore) throw new Exception("restore failed"); }
    public bool Focus(IntPtr h) { Calls.Add("focus"+h); if (DenyFocus) return false; Current=h; return true; }
    public bool Stop(uint pid) { Calls.Add("stop"); return true; }
}
public class ViewChecks {
    static void Check(bool value,string description) { if(!value) throw new Exception(description); }
    static Session Create(FakeViewDesktop api) { return new Session(api,new IntPtr(1),10,new IntPtr(2),20); }
    public static void Run() {
        var api=new FakeViewDesktop(); var session=Create(api);
        session.Enter(new Rect()); Check(session.Active && api.Current==new IntPtr(2),"Presentation stole Minecraft focus");
        Check(!api.Calls.Contains("focus1") && !api.Calls.Contains("focus2"),"Enter must never focus a different window");
        session.Normal(true);
        Check(String.Join(",",api.Calls.ToArray())=="capture,present,stop,restore,focus1","Stop must precede window restoration and game focus");
        Check(!session.Active && session.StopConfirmed,"Normal must end presentation and latch native stop");
        session.Prepare(); Check(api.Current==new IntPtr(2),"Prepare must give Minecraft focus after stopping");
        api.Current=new IntPtr(3);
        try { session.Enter(new Rect()); throw new Exception("wrong focus accepted"); } catch(InvalidOperationException) {}
        api.Current=new IntPtr(2); session.Enter(new Rect());
        api.GameAlive=false; int count=api.Calls.Count;
        session.Normal(true); Check(api.Calls.Count==count,"Never write IPC or restore/focus a reused or closed game HWND");
        api=new FakeViewDesktop(); session=Create(api); api.FailPresent=true;
        try { session.Enter(new Rect()); throw new Exception("partial failure accepted"); } catch(Exception e) { Check(e.Message=="partial present","Unexpected failure"); }
        Check(!session.Active && api.Calls.Contains("restore") && api.Calls.Contains("stop"),"Partial presentation needs stop and restoration");
        api=new FakeViewDesktop(); session=Create(api); session.Enter(new Rect()); api.FailRestore=true;
        try { session.Normal(false); } catch(Exception) {}
        Check(session.Active,"Keep restoration pending after failure, so cleanup can retry");
        api.FailRestore=false; session.Normal(false); Check(!session.Active,"Cleanup retry must restore window");
        Check(!api.Calls.Contains("focus1"),"Background cleanup must not steal focus from another app");
        api=new FakeViewDesktop(); session=Create(api); session.Enter(new Rect()); api.DenyFocus=true;
        session.Normal(true);
        Check(!session.Active && session.StopConfirmed && !session.FocusConfirmed,"Denied foreground must still stop and restore without throwing");
        IntPtr view=Marshal.AllocHGlobal(80);
        try {
            for(int version=1;version<=2;version++) {
                for(int word=0;word<20;word++) Marshal.WriteInt32(view,word*4,100+word);
                Marshal.WriteInt32(view,0,0x52453443); Marshal.WriteInt32(view,4,version); Marshal.WriteInt32(view,28,1);
                int[] before=new int[20]; for(int word=0;word<20;word++) before[word]=Marshal.ReadInt32(view,word*4);
                Check(WindowsDesktop.LatchStop(view),"Supported IPC must accept stop");
                for(int word=0;word<20;word++) Check(Marshal.ReadInt32(view,word*4)==(word==6?1:before[word]),"Stop must not change host sequence/armed/buttons/look or native status");
            }
            foreach(int offset in new int[]{0,4,28}) {
                Marshal.WriteInt32(view,0,0x52453443); Marshal.WriteInt32(view,4,2); Marshal.WriteInt32(view,28,1);
                Marshal.WriteInt32(view,24,0); Marshal.WriteInt32(view,offset,99);
                Check(!WindowsDesktop.LatchStop(view) && Marshal.ReadInt32(view,24)==0,"Unknown mapping must not be written");
            }
        } finally { Marshal.FreeHGlobal(view); }
    }
}
}
'@
Add-Type -TypeDefinition ((Get-Content -LiteralPath (Join-Path $PSScriptRoot 'RE4View.cs') -Raw) + [Environment]::NewLine + $fixture)
[RE4BridgeView.ViewChecks]::Run()
$tokens=$null; $errors=$null
[void][System.Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'VIEW-RE4.ps1'),[ref]$tokens,[ref]$errors)
if ($errors.Count -ne 0) { throw ($errors | Out-String) }
$ast=[System.Management.Automation.Language.Parser]::ParseFile((Join-Path $root 'VIEW-RE4.ps1'),[ref]$tokens,[ref]$errors)
$returnFunction=$ast.Find({param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Return-ToNormal'},$true)
if ($null -eq $returnFunction) { throw 'Missing direct return action.' }
# Execute the real return action with the panel hidden; no WinForms button click is needed.
Invoke-Expression $returnFunction.Extent.Text
$script:panelHidden=$true
function Set-ViewPanel([bool]$compact) { $script:panelHidden=$compact }
$api=[RE4BridgeView.FakeViewDesktop]::new()
$session=[RE4BridgeView.Session]::new($api,[IntPtr]::new(1),10,[IntPtr]::new(2),20)
$session.Enter([RE4BridgeView.Rect]::new())
$label=[pscustomobject]@{Text=''}
Return-ToNormal
if ($session.Active -or $script:panelHidden -or !$session.StopConfirmed -or $api.Current -ne [IntPtr]::new(1)) {
    throw 'Hidden-panel return must stop, restore, show panel and focus RE4.'
}
$api.Current=[IntPtr]::new(2)
$session.Enter([RE4BridgeView.Rect]::new())
$api.DenyFocus=$true
Return-ToNormal
if ($session.Active -or !$session.StopConfirmed -or $label.Text -notlike '*pulsa la ventana de RE4*' -or $label.Text -like 'Error*') {
    throw 'Denied focus must report manual activation after restoring and stopping.'
}
Write-Host 'View fixtures passed: focus preserved, native stop before return, partial failure cleanup, closed/reused window protection, restoration retry, IPC v1/v2 stop-only write and invalid-header rejection. GUI script parsed. Real Windows presentation not executed.'
