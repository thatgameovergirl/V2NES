# 🎮 V2NES Dual-Screen Companion Suite (v1.4.0)

Welcome to **V2NES**, the ultimate dual-screen multitasking companion suite for Nintendo 3DS! This suite includes the highly modified **V2NES 3DS Emulator** (rendering crisp retro gameplay on the top screen) and the sleek standalone **Desktop Asset Companion App** (`v2m_compiler.exe`) designed to stage, compile, and deploy walkthrough guides and maps to the bottom screen.

---

## 🛠️ Supported Filetypes

The Companion App automatically reads and validates the following file types:

| Asset Type | Supported Formats | Target Destination on SD Card |
| :--- | :--- | :--- |
| **Game ROMs** | `.nes`, `.fds` | Anywhere on SD Card / Emulator |
| **Level Maps** | `.png`, `.jpg`, `.jpeg`, `.bmp` | `sdmc:/V2NES/maps/` |
| **Walkthroughs** | **Plain Text `.txt` only** | `sdmc:/V2NES/guides/` |

> [!IMPORTANT]
> To ensure flawless rendering inside the 3DS virtualized viewport engine, game guides **must be standard, plain-text `.txt` files**. If your guide is formatted as a `.md` or `.html` file, simply copy and paste its raw text into a plain notepad document and save it as `.txt`.

---

## 📊 Conversion Limits

* **ROM Processing**: **1 ROM** per conversion batch. Dragging in a ROM locks the game title prefix for all associated assets.
* **Map Compilation**: Concurrently compile up to **10 maps at the exact same time**! The smart role-assignment engine easily indexes them, displays an auto-assignment selector, and tracks individual compiling tasks with a live progress bar.
* **Guides Processing**: Unlimited plain text guides!
  * Dragging in **1 guide** compiles it directly to `<GameName>.txt` for automatic loading.
  * Dragging in **multiple guides** automatically compiles them in indexed order to `<GameName>-1.txt`, `<GameName>-2.txt`, etc., allowing in-game manual navigation.

---

## 🚀 How to Use the Companion App

### Step 1: Open the Application
Run **`v2m_compiler.exe`** inside `C:\V2NES\companion_app\`. You will be greeted by a gorgeous, compact pitch-black window decorated with the dynamic cyan circular **V2N Brand Logo** and taskbar icon.

### Step 2: Drag & Drop Assets
Select your **NES ROM**, your **Map image files**, and your **Walkthrough `.txt` files** together and drag them directly into the dashed **Drag & Drop Zone** (or click inside the box to browse manually).
* *Note: The app will automatically sanitize the game title (replacing spaces with underscores) and lock it.*

### Step 3: Configure Quality Options
Ensure **`1:1 Pixel-Perfect (Skip Downscaling Engine)`** is checked (it is checked by default). This tells the graphics compiler to skip aggressive compression and instead draw your map's pixels at their raw, crystal-clear resolution centered onto the 3DS GPU's viewport canvas.

### Step 4: Map Role Assignment
Click **CONVERT & DEPLOY ASSETS**. The upgraded **Assign Map Roles** window will launch modal:
1. Review the files. The companion app's **Smart Auto-Detection** scans filenames and pre-selects the correct options (e.g. automatically matching `"overworld"` to the base map and `"level1"` to the Level 1 dungeon slot).
2. For specific non-standard maps, choose **Custom Suffix...** and the custom input rectangle will slide into view dynamically.
3. Click **CONFIRM & EXPORT**.

### Step 5: Visual Progress Bar Tracking
Watch the futuristic vertical **PROGRESS** indicator fill up on the right side of the window! All inputs will lock safely, and the log terminal will report live progress details in bright green/red terminal formatting.
* On successful completion, a beautiful popup will alert you: `"Conversion successfully completed!"` and the window will close.
* If any hardware read or write error occurs, the window aborts cleanly and reports: `"Conversion Aborted due to an error!"`.

### Step 6: Deploy to 3DS SD Card
All compiled binary `.v2m` maps and optimized `.txt` guides are output directly next to your ROM.
Copy them to your emulator or real 3DS SD card structure:
* **Maps (`.v2m`)**: Copy to `sdmc:/V2NES/maps/`
* **Guides (`.txt`)**: Copy to `sdmc:/V2NES/guides/`

Start **V2NES** on your 3DS, choose your game, click **PLAY**, and enjoy your ultimate dual-screen adventure! 🎮🗺️✨

---

## 👩‍💻 Credits & Support

Developed with 🩵 by **HristValkyrja**  
✉️ Contact: [thatgameovergirl@gmail.com](mailto:thatgameovergirl@gmail.com)
