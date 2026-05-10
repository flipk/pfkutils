
param (
    [Parameter(Mandatory=$true)]
    [string]$Suffix
)

# 1. Construct the full IP address
$IP = "11.1.$Suffix"

# 2. Send 1 ping. -Quiet returns True if it succeeds, False if it fails.
if (-not (Test-Connection -ComputerName $IP -Count 1 -Quiet)) {
    Write-Host "Host $IP is not reachable. Exiting..." -ForegroundColor Red
    exit
}

# 3. Remove the old host key (since it changes every boot)
# We use & to call external applications if the path might have spaces, 
# but for standard commands, just calling them works.
ssh-keygen -R $IP

# 4. SSH into the unit
ssh -o "StrictHostKeyChecking no" -l root $IP
