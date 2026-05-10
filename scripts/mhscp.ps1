
param (
    [Parameter(Mandatory=$true, Position=0)]
    [string]$Source,

    [Parameter(Mandatory=$true, Position=1)]
    [string]$Destination
)

$ipSuffix = $null
$newSource = $Source
$newDest = $Destination

# Regex to find the device IP pattern (e.g., "10.5:/path" or "user@10.5:/path")
# It looks for digits.digits followed by a colon.
$regex = "^(?:(?<user>[^@]+)@)?(?<ip>\d{1,3}\.\d{1,3}):(?<path>.*)$"

# 1. Determine which argument is the remote device
if ($Source -match $regex) {
    $ipSuffix = $matches['ip']
    $user = if ($matches['user']) { $matches['user'] } else { "root" }
    $fullIp = "11.1.$ipSuffix"
    $newSource = "${user}@${fullIp}:$($matches['path'])"
} 
elseif ($Destination -match $regex) {
    $ipSuffix = $matches['ip']
    $user = if ($matches['user']) { $matches['user'] } else { "root" }
    $fullIp = "11.1.$ipSuffix"
    $newDest = "${user}@${fullIp}:$($matches['path'])"
} 
else {
    Write-Error "Could not find a valid remote device argument. Use format '10.5:/remote/file'."
    return
}

# 2. Send 1 ping to verify it is online
if (-not (Test-Connection -ComputerName $fullIp -Count 1 -Quiet)) {
    Write-Host "Host $fullIp is not reachable. Exiting..." -ForegroundColor Red
    return
}

# 3. Remove the old host key
ssh-keygen -R $fullIp

# 4. Execute the SCP command
Write-Host "Copying via: scp $newSource $newDest" -ForegroundColor Cyan
scp -o "StrictHostKeyChecking no" $newSource $newDest
