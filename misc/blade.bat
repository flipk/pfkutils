
@echo off

rem make a shortcut "C:\Windows\System32\cmd.exe /c h:\vorpal\blade.bat"

c:
cd \users\pknaack1\Desktop

echo doing nslookup of blade.phillipknaack.com
nslookup blade.phillipknaack.com 140.101.80.92 > turd.txt

for /f "tokens=1,2" %%i in (turd.txt) do ^
if "%%i"=="Address:" set blade=%%j

del turd.txt

echo blade is at %blade%
echo expect fingerprint 05:44:7f:38:8c:20:08:cc:36:06:fa:bb:4a:61:09:4f
echo blade's vnc is on :9
echo leviathan's vnc is on :8

echo to surf the web, run:
echo   [path]\chrome.exe --proxy-server="socks5://127.0.0.1:5000"
echo to access rdio use:
echo   [path]\chrome.exe --proxy-server="socks5://127.0.0.1:5000" -user-data-dir=C:\Users\pknaack1\data\bladechromeroot

"C:\ProgramData\Microsoft\Windows\Start Menu\Programs\PuTTy.exe" ^-D 5000 ^-L 5909:127.0.0.1:5901 ^-L 5908:127.0.0.1:5902 flipk@%blade%

REM 
REM Note the silly thing I had to do to allow this to be 'pin'ned to the
REM start menu:  i had to create a shortcut whose target looks like this:
REM    c:\windows\system32\cmd.exe /c h:\vorpal\blade.bat
REM 
REM For some reason, Windows 7 will not 'pin' a shortcut if the target of
REM the shortcut is a BAT, only if it is an EXE.
REM 
REM WTF, MS?
REM 
