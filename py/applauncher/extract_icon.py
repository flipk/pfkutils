
# the purpose of this program is to open a windows EXE file
# and extract an ICO image out of it, converting to PNG format.

# "pip install pywin32" if needed
import win32ui
import win32gui
import win32con
import win32api
from PIL import Image
import sys

if len(sys.argv) != 3:
    print('usage:  py.exe extract_icon.py  <EXE>  <PNG>')
    exit(0)

exe_path = sys.argv[1]  # R"C:\Users\pknaack1\data\apps\Notepad++\notepad++.exe"
output_path = sys.argv[2]  # "notepad++.png"

# --- 1. Extract the icon handle ---
large_icons, small_icons = win32gui.ExtractIconEx(exe_path, 0, 1)

if large_icons:
    h_icon = large_icons[0]
    
    # --- 2. Get Icon Info & Create a Bitmap ---
    info = win32gui.GetIconInfo(h_icon)

    # Get dimensions by querying the bitmap handle
    h_bmp_color = info[4]
    # CORRECTED LINE: GetObject is in win32gui
    bitmap_info = win32gui.GetObject(h_bmp_color) 
    width = bitmap_info.bmWidth
    height = bitmap_info.bmHeight

    hdc = win32ui.CreateDCFromHandle(win32gui.GetDC(0))
    h_bmp = win32ui.CreateBitmap()
    h_bmp.CreateCompatibleBitmap(hdc, width, height)
    mem_dc = hdc.CreateCompatibleDC()
    mem_dc.SelectObject(h_bmp)
    
    # --- 3. Draw the Icon onto the Bitmap ---
    mem_dc.SetBkMode(win32con.TRANSPARENT)
    brush = win32gui.GetSysColorBrush(win32con.COLOR_WINDOW)
    win32gui.FillRect(mem_dc.GetSafeHdc(), (0, 0, width, height), brush)
    win32gui.DrawIconEx(mem_dc.GetSafeHdc(), 0, 0, h_icon, 0, 0, 0, 0, win32con.DI_NORMAL)
    
    # --- 4. Convert to PNG using Pillow ---
    bmp_info = h_bmp.GetInfo()
    bmp_str = h_bmp.GetBitmapBits(True)

    pil_image = Image.frombuffer(
        'RGBA',
        (bmp_info['bmWidth'], bmp_info['bmHeight']),
        bmp_str, 'raw', 'BGRA', 0, 1
    )

    pil_image.save(output_path, "PNG")
    print(f"✅ Icon extracted and saved to {output_path}")

    # --- 5. Clean up resources ---
    win32gui.DeleteObject(info[3])
    win32gui.DeleteObject(info[4])
    win32gui.DestroyIcon(h_icon)
    win32gui.DeleteObject(h_bmp.GetHandle())
    mem_dc.DeleteDC()
    hdc.DeleteDC()

else:
    print("❌ No icons found in the specified file.")
