$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$fixture = @'
namespace RE4BridgeView {
public sealed class FakeMouseDesktop : MouseDesktop {
    public IntPtr Current=new IntPtr(2),Target=new IntPtr(1);
    public bool GameAlive=true,McAlive=true,FailSend;
    public uint Mods;public bool MenuOpen {get;set;}
    public System.Collections.Generic.List<string> Sent=new System.Collections.Generic.List<string>();
    public bool SameWindow(IntPtr h,uint pid){return h==new IntPtr(1)?GameAlive && pid==10:h==new IntPtr(2) && McAlive && pid==20;}
    public IntPtr Foreground(){return Current;}
    public IntPtr At(Point p){return Target;}
    public uint Modifiers(){return Mods;}
    public bool Send(IntPtr h,uint msg,uint buttons,Point p,ushort wheel){Sent.Add(h+":"+msg+":"+buttons+":"+wheel);return !FailSend;}
}
public static class MouseChecks {
    static void Check(bool test,string message){if(!test)throw new Exception(message);}
    static MouseRouter Create(FakeMouseDesktop api){return new MouseRouter(api,new Snapshot{Handle=new IntPtr(1),Pid=10},new IntPtr(2),20);}
    public static void Run(){
        var api=new FakeMouseDesktop();var router=Create(api);var p=new Point{X=123,Y=-23};
        for(int i=0;i<100;i++){
            Check(router.Route(0,0x204,p,0,0),"right down must stay in Minecraft");
            Check(router.Route(0,0x205,p,0,0),"right up must stay in Minecraft");
            Check(router.Route(0,0x201,p,0,0),"left down must stay in Minecraft");
            Check(router.Route(0,0x202,p,0,0),"left up must stay in Minecraft");
        }
        Check(api.Sent.Count==400 && api.Current==new IntPtr(2),"Repeated build/break must never focus another window");
        api.Mods=12;Check(router.Route(0,0x201,p,0,0),"modified click");
        Check(api.Sent[400]=="2:513:13:0","preserve shift/control and held buttons");
        Check(router.Route(0,0x204,p,0,0) && api.Sent[401]=="2:516:15:0","simultaneous left/right");
        api.Target=new IntPtr(2);Check(router.Route(0,0x202,p,0,0),"matching up after capture must be forwarded once");
        Check(api.Sent[402]=="2:514:14:0","left release keeps right held");
        router.Release(p);Check(api.Sent[403]=="2:517:12:0","stop releases held right button");
        int count=api.Sent.Count;router.Release(p);Check(api.Sent.Count==count,"release is idempotent");
        api.Target=new IntPtr(1);api.Mods=0;
        Check(router.Route(0,0x20a,p,unchecked((uint)(-120<<16)),0),"hotbar wheel");
        Check(api.Sent[count]=="2:522:0:65416","preserve negative wheel delta");
        Check(router.Route(0,0x20b,p,2u<<16,0),"X2 down");router.Release(p);
        Check(api.Sent[api.Sent.Count-1]=="2:524:0:2","X2 cleanup");
        count=api.Sent.Count;
        Check(!router.Route(0,0x200,p,0,0),"never duplicate raw mouse motion");
        api.MenuOpen=true;int moves=api.Sent.Count;
        for(int i=0;i<10000;i++)Check(!router.Route(0,0x200,p,0,0),"OS cursor continues during menu flood");
        Check(api.Sent.Count==moves,"menu flood does not fill the message queue");router.FlushMotion();
        Check(api.Sent.Count==moves+1,"only latest menu position is sent at display cadence");
        router.Route(0,0x200,p,0,0);router.Route(0,0x201,p,0,0);
        Check(api.Sent[api.Sent.Count-2].Contains(":512:") && api.Sent[api.Sent.Count-1].Contains(":513:"),"menu position precedes click");router.Release(p);
        router.Route(0,0x200,p,0,0);api.Current=new IntPtr(3);moves=api.Sent.Count;router.FlushMotion();Check(api.Sent.Count==moves,"queued cursor cannot enter another window");api.Current=new IntPtr(2);
        api.MenuOpen=false;count=api.Sent.Count;
        Check(!router.Route(-1,0x201,p,0,0),"negative hook code");
        Check(!router.Route(0,0x201,p,0,1),"injected events are not forwarded");
        api.Target=new IntPtr(3);Check(!router.Route(0,0x201,p,0,0),"click elsewhere is not captured");
        api.Target=new IntPtr(1);api.Current=new IntPtr(3);Check(!router.Route(0,0x201,p,0,0),"alt-tab is not captured or refocused");
        api.Current=new IntPtr(2);api.McAlive=false;Check(!router.Route(0,0x201,p,0,0),"closed/reused MC HWND");
        api.McAlive=true;api.GameAlive=false;Check(!router.Route(0,0x201,p,0,0),"closed RE4 HWND");
        Check(api.Sent.Count==count,"rejected inputs send nothing");
        api.GameAlive=true;api.FailSend=true;Check(router.Route(0,0x201,p,0,0) && router.Failed,"failed delivery must suppress click and end view");
        count=api.Sent.Count;Check(!router.Route(0,0x201,p,0,0) && api.Sent.Count==count,"failed router does not repeat sends");
        api=new FakeMouseDesktop();router=Create(api);router.Route(0,0x201,p,0,0);api.McAlive=false;count=api.Sent.Count;router.Release(p);
        Check(api.Sent.Count==count,"cleanup never writes to reused MC HWND");
    }
}
}

namespace RE4BridgeView {
public class FakeViewDesktop : Desktop {
    public System.Collections.Generic.List<string> Calls = new System.Collections.Generic.List<string>();
    public IntPtr Current = new IntPtr(2);
    public int Resizes;
    public bool GameAlive = true, FailPresent, FailRestore, DenyFocus, FailMouse, FailMouseEnd;
    public bool SameWindow(IntPtr h,uint pid) { return h == new IntPtr(1) ? GameAlive && pid == 10 : h == new IntPtr(2) && pid == 20; }
    public IntPtr Foreground() { return Current; }
    public Snapshot Capture(IntPtr h,uint pid) { Calls.Add("capture"); return new Snapshot {Handle=h,Pid=pid}; }
    public void PrepareInput(Snapshot s,Rect r){Calls.Add("inputsize");}
    public void PrepareView(Snapshot s,Rect r){Calls.Add("resize");Resizes++;}
    public void Present(Snapshot s,Rect r) { Calls.Add("present"); if(FailPresent) throw new Exception("partial present"); }
    public void Restore(Snapshot s) { Calls.Add("restore"); if(FailRestore) throw new Exception("restore failed"); }
    public bool Focus(IntPtr h) { Calls.Add("focus"+h); if (DenyFocus) return false; Current=h; return true; }
    public bool MouseFailed {get;set;}
    public string MouseFailure {get{return "test delivery failure";}}
    public void PumpMouse(){}
    public void StartMouse(Snapshot s,IntPtr mc,uint pid){Calls.Add("mouseon");if(FailMouse)throw new Exception("mouse unavailable");}
    public void EndMouse(){Calls.Add("mouseoff");if(FailMouseEnd)throw new Exception("mouse cleanup failed");}
    public bool Stop(uint pid) { Calls.Add("stop"); return true; }
}
public class ViewChecks {
    static void Check(bool value,string description) { if(!value) throw new Exception(description); }
    static Session Create(FakeViewDesktop api) { return new Session(api,new IntPtr(1),10,new IntPtr(2),20); }
    static Session Ready(FakeViewDesktop api){var session=Create(api);session.Prepare(new Rect());api.Calls.Clear();return session;}
    public static void Run() {
        var api=new FakeViewDesktop();var unprepared=Create(api);
        try{unprepared.Enter(new Rect());throw new Exception("unprepared view accepted");}catch(InvalidOperationException){}
        unprepared.Prepare(new Rect());Check(String.Join(",",api.Calls.ToArray())=="capture,capture,stop,mouseoff,inputsize,resize,focus2","Stop and resize before arming, Minecraft visible");
        Check(unprepared.Prepared && !unprepared.Active,"prepare must not cover Minecraft");unprepared.Normal(false);Check(!unprepared.Prepared,"F8 restores even an inactive prepared view");
        api=new FakeViewDesktop(); var session=Ready(api);
        session.Enter(new Rect()); Check(session.Active && api.Resizes==1 && api.Current==new IntPtr(2),"Presentation stole Minecraft focus");
        Check(!api.Calls.Contains("focus1") && !api.Calls.Contains("focus2"),"Enter must never focus a different window");
        session.Normal(true);
        Check(String.Join(",",api.Calls.ToArray())=="present,mouseon,stop,mouseoff,restore,restore,focus1","Stop must precede window restoration and game focus");
        Check(!session.Active && session.StopConfirmed,"Normal must end presentation and latch native stop");
        session.Prepare(new Rect()); Check(api.Current==new IntPtr(2),"Prepare must give Minecraft focus after stopping");
        api.Current=new IntPtr(3);
        try { session.Enter(new Rect()); throw new Exception("wrong focus accepted"); } catch(InvalidOperationException) {}
        api.Current=new IntPtr(2); session.Enter(new Rect());
        api.GameAlive=false; int count=api.Calls.Count;
        session.Normal(true); Check(api.Calls.Count==count+2 && api.Calls[api.Calls.Count-2]=="mouseoff" && api.Calls[api.Calls.Count-1]=="restore","Never write IPC or restore/focus a reused or closed game HWND");
        api=new FakeViewDesktop(); session=Ready(api); api.FailPresent=true;
        try { session.Enter(new Rect()); throw new Exception("partial failure accepted"); } catch(Exception e) { Check(e.Message=="partial present","Unexpected failure"); }
        Check(!session.Active && api.Calls.Contains("restore") && api.Calls.Contains("stop"),"Partial presentation needs stop and restoration");
        api=new FakeViewDesktop(); session=Ready(api); session.Enter(new Rect()); api.FailRestore=true;
        try { session.Normal(false); } catch(Exception) {}
        Check(session.Active,"Keep restoration pending after failure, so cleanup can retry");
        api.FailRestore=false; session.Normal(false); Check(!session.Active,"Cleanup retry must restore window");
        Check(!api.Calls.Contains("focus1"),"Background cleanup must not steal focus from another app");
        api=new FakeViewDesktop(); session=Ready(api); session.Enter(new Rect()); api.DenyFocus=true;
        session.Normal(true);
        Check(!session.Active && session.StopConfirmed && !session.FocusConfirmed,"Denied foreground must still stop and restore without throwing");
        api=new FakeViewDesktop();session=Ready(api);api.FailMouse=true;
        try{session.Enter(new Rect());throw new Exception("missing mouse accepted");}catch(Exception e){Check(e.Message=="mouse unavailable","mouse failure");}
        Check(!session.Active && api.Calls.Contains("restore") && session.StopConfirmed,"Mouse setup failure must stop and restore");
        api=new FakeViewDesktop();session=Ready(api);session.Enter(new Rect());api.MouseFailed=true;
        Check(!session.StillFocused(),"Delivery failure ends view instead of recovering focus");
        api.FailMouseEnd=true;try{session.Normal(false);}catch(Exception){}
        Check(!session.Active && api.Calls.Contains("restore") && session.StopConfirmed,"Even mouse cleanup failure must restore window");
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
[RE4BridgeView.MouseChecks]::Run()
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
$session.Prepare([RE4BridgeView.Rect]::new())
$session.Enter([RE4BridgeView.Rect]::new())
$label=[pscustomobject]@{Text=''}
Return-ToNormal
if ($session.Active -or $script:panelHidden -or !$session.StopConfirmed -or $api.Current -ne [IntPtr]::new(1)) {
    throw 'Hidden-panel return must stop, restore, show panel and focus RE4.'
}
$api.Current=[IntPtr]::new(2)
$session.Prepare([RE4BridgeView.Rect]::new())
$session.Enter([RE4BridgeView.Rect]::new())
$api.DenyFocus=$true
Return-ToNormal
if ($session.Active -or !$session.StopConfirmed -or $label.Text -notlike '*pulsa la ventana de RE4*' -or $label.Text -like 'Error*') {
    throw 'Denied focus must report manual activation after restoring and stopping.'
}
Write-Host 'View fixtures passed: focus preserved, native stop before return, partial failure cleanup, closed/reused window protection, restoration retry, IPC v1/v2 stop-only write and invalid-header rejection. GUI script parsed. Button/wheel routing, simultaneous held buttons, capture releases, modifiers, injected/foreign event rejection and send failures passed. Real Windows presentation not executed.'
. (Join-Path $PSScriptRoot 'Prepare-Minecraft.ps1')
$listener=[System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback,0)
$listener.Start()
$temporary=Join-Path ([System.IO.Path]::GetTempPath()) ('re4-prepare-'+[Guid]::NewGuid())
try {
    $port=$listener.LocalEndpoint.Port
    New-Item -ItemType Directory -Path (Join-Path $temporary 'config') | Out-Null
    @{protocol=1;port=$port-1;token=('a'*64)} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $temporary 'config/re4bridge.json')
    $call=Invoke-MinecraftPreparation $temporary
    $peer=$listener.AcceptTcpClient();$peer.ReceiveTimeout=2000
    $reader=[System.IO.StreamReader]::new($peer.GetStream())
    $request=$reader.ReadLine() | ConvertFrom-Json
    if ($request.type -ne 'prepare' -or $request.token -ne ('a'*64) -or $request.protocol -ne 1 -or $call.Completed) {throw 'Preparation must authenticate without blocking the UI.'}
    $bytes=[System.Text.Encoding]::UTF8.GetBytes('{"protocol":1,"ok":true,"message":"Listo F9"}'+"`n")
    $peer.GetStream().Write($bytes,0,$bytes.Length)
    $deadline=[DateTime]::UtcNow.AddSeconds(2)
    while (!$call.Completed -and [DateTime]::UtcNow -lt $deadline) {Start-Sleep -Milliseconds 5}
    if (!$call.Completed -or (Complete-MinecraftPreparation $call) -ne 'Listo F9') {throw 'Preparation reply missing.'}
    $call.Dispose();$reader.Dispose();$peer.Close()
    $call=Invoke-MinecraftPreparation $temporary;$peer=$listener.AcceptTcpClient();$peer.ReceiveTimeout=2000
    $reader=[System.IO.StreamReader]::new($peer.GetStream());[void]$reader.ReadLine()
    $script:prepareCall=$call;Return-ToNormal
    if ($null -ne $script:prepareCall -or $peer.GetStream().ReadByte() -ne -1) {throw 'F8 must close pending preparation without waiting for Minecraft.'}
    $reader.Dispose();$peer.Close()
    Write-Host 'Automatic preparation: authenticated loopback request, asynchronous reply and F8 cancellation passed.'
} finally {$listener.Stop();Remove-Item -LiteralPath $temporary -Recurse -Force}
