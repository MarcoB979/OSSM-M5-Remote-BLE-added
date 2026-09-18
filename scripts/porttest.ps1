$ErrorActionPreference = 'SilentlyContinue'
foreach ($p in @(80, 81)) {
    $c = New-Object System.Net.Sockets.TcpClient
    $iar = $c.BeginConnect('192.168.178.53', $p, $null, $null)
    $ok = $iar.AsyncWaitHandle.WaitOne(3000)
    if ($ok) {
        try {
            $c.EndConnect($iar)
            Write-Output "port $p : OPEN"
        } catch {
            Write-Output "port $p : REFUSED"
        }
    } else {
        Write-Output "port $p : TIMEOUT (not listening)"
    }
    $c.Close()
}
