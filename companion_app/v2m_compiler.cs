using System;
using System.IO;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using System.Text.RegularExpressions;

class V2MCompilerForm : Form
{
    private Panel dropZone;
    private ListBox fileListBox;
    private Button convertBtn;
    private TextBox logConsole;
    private Label titleLabel;
    private Label fileListLabel;
    internal CheckBox pixelPerfectCheck;
    internal ComboBox scaleCombo;

    private List<string> selectedFiles = new List<string>();
    private string loadedRomPath = "";
    internal string detectedRomName = "";
    internal string romDirectory = "";

    private string SanitizeTitle(string name)
    {
        string clean = "";
        foreach (char c in name)
        {
            if (c == ' ')
            {
                clean += '_';
            }
            else if (char.IsLetterOrDigit(c) || c == '_' || c == '-')
            {
                clean += c;
            }
        }
        return clean;
    }

    [STAThread]
    static void Main()
    {
        Application.EnableVisualStyles();
        Application.SetCompatibleTextRenderingDefault(false);
        Application.Run(new V2MCompilerForm());
    }

    public V2MCompilerForm()
    {
        // 1. Form Window Settings (Compact clean dimensions)
        this.Text = "V2NES Companion v1.5.0";
        this.Size = new Size(500, 830);
        this.FormBorderStyle = FormBorderStyle.FixedSingle;
        this.MaximizeBox = false;
        this.BackColor = Color.FromArgb(10, 10, 12); // Deep main background
        this.ForeColor = Color.FromArgb(224, 224, 224); // Light text
        this.StartPosition = FormStartPosition.CenterScreen;
 
        // Dynamic Adventure Map Icon Generation (Vintage parchment map with grid, dotted path, and red X)
        try
        {
            using (Bitmap iconBmp = new Bitmap(32, 32))
            {
                using (Graphics g = Graphics.FromImage(iconBmp))
                {
                    g.Clear(Color.FromArgb(235, 205, 150)); // Vintage parchment paper background
                    
                    // Draw soft coordinate grid lines
                    using (Pen gridPen = new Pen(Color.FromArgb(200, 170, 120), 1f))
                    {
                        g.DrawLine(gridPen, 8, 0, 8, 32);
                        g.DrawLine(gridPen, 16, 0, 16, 32);
                        g.DrawLine(gridPen, 24, 0, 24, 32);
                        g.DrawLine(gridPen, 0, 8, 32, 8);
                        g.DrawLine(gridPen, 0, 16, 32, 16);
                        g.DrawLine(gridPen, 0, 24, 32, 24);
                    }
                    
                    // Draw a curving dotted path leading to the treasure
                    using (Pen pathPen = new Pen(Color.FromArgb(220, 20, 60), 2f))
                    {
                        pathPen.DashStyle = DashStyle.Dot;
                        g.DrawCurve(pathPen, new Point[] {
                            new Point(4, 28),
                            new Point(12, 20),
                            new Point(20, 22),
                            new Point(26, 10)
                        });
                    }
                    
                    // Draw a bold red "X marks the spot"
                    using (Font xFont = new Font("Arial", 9, FontStyle.Bold))
                    using (Brush xBrush = new SolidBrush(Color.FromArgb(220, 0, 0)))
                    {
                        g.DrawString("X", xFont, xBrush, 21, 2);
                    }
                }
                this.Icon = Icon.FromHandle(iconBmp.GetHicon());
            }
        }
        catch {}
 
        InitializeComponents();
        AddLog("V2NES Companion initialized...");
        AddLog("Drag and drop your NES ROM, Maps, and Guides together!");
    }

    private void InitializeComponents()
    {
        int elementWidth = 445;

        // 2. Title Header Label
        titleLabel = new Label();
        titleLabel.Text = "V2NES COMPANION";
        titleLabel.Font = new Font("Segoe UI", 16, FontStyle.Bold);
        titleLabel.ForeColor = Color.FromArgb(0, 242, 255); // Cyan brand color
        titleLabel.Location = new Point(20, 15);
        titleLabel.Size = new Size(elementWidth, 35);
        titleLabel.TextAlign = ContentAlignment.MiddleLeft;
        this.Controls.Add(titleLabel);

        // 3. Drag & Drop Zone Panel
        dropZone = new Panel();
        dropZone.Size = new Size(elementWidth, 140);
        dropZone.Location = new Point(20, 60);
        dropZone.BackColor = Color.FromArgb(20, 20, 24); // Slate card background
        dropZone.AllowDrop = true;
        dropZone.Cursor = Cursors.Hand;

        // Custom drawing for dashed border
        dropZone.Paint += (s, e) => {
            using (Pen pen = new Pen(Color.FromArgb(0, 242, 255), 2))
            {
                pen.DashStyle = DashStyle.Dash;
                e.Graphics.DrawRectangle(pen, 1, 1, dropZone.Width - 2, dropZone.Height - 2);
            }
        };

        // Text inside Drag & Drop Zone
        Label dropText = new Label();
        dropText.Text = "DRAG & DROP FILES HERE\n\n(ROMs, Maps, and Guides)\nOr Click to Browse";
        dropText.Font = new Font("Segoe UI", 9, FontStyle.Bold);
        dropText.ForeColor = Color.FromArgb(136, 136, 136);
        dropText.TextAlign = ContentAlignment.MiddleCenter;
        dropText.Dock = DockStyle.Fill;
        dropText.AllowDrop = true;

        dropText.DragEnter += (s, e) => e.Effect = DragDropEffects.Copy;
        dropText.DragDrop += (s, e) => HandleDroppedFiles((string[])e.Data.GetData(DataFormats.FileDrop));
        dropText.Click += (s, e) => BrowseFiles();

        dropZone.Controls.Add(dropText);
        this.Controls.Add(dropZone);

        // 4. Staged Files Label
        fileListLabel = new Label();
        fileListLabel.Text = "STAGED FILES FOR COMPILING";
        fileListLabel.Font = new Font("Segoe UI", 8, FontStyle.Bold);
        fileListLabel.ForeColor = Color.FromArgb(136, 136, 136);
        fileListLabel.Location = new Point(20, 215);
        fileListLabel.Size = new Size(elementWidth, 15);
        this.Controls.Add(fileListLabel);

        // Selected Files List Box
        fileListBox = new ListBox();
        fileListBox.Size = new Size(elementWidth, 80);
        fileListBox.Location = new Point(20, 235);
        fileListBox.BackColor = Color.FromArgb(20, 20, 24);
        fileListBox.ForeColor = Color.FromArgb(224, 224, 224);
        fileListBox.BorderStyle = BorderStyle.FixedSingle;
        fileListBox.Font = new Font("Segoe UI", 9);
        this.Controls.Add(fileListBox);

        // 5. 1:1 Pixel-Perfect Checkbox
        pixelPerfectCheck = new CheckBox();
        pixelPerfectCheck.Text = "1:1 Pixel-Perfect (Skip Downscaling Engine)";
        pixelPerfectCheck.Font = new Font("Segoe UI", 9, FontStyle.Bold);
        pixelPerfectCheck.ForeColor = Color.FromArgb(0, 242, 255); // Cyan brand highlight
        pixelPerfectCheck.Location = new Point(20, 325);
        pixelPerfectCheck.Size = new Size(elementWidth, 25);
        pixelPerfectCheck.Cursor = Cursors.Hand;
        pixelPerfectCheck.Checked = true; // Default to checked since it works perfectly!
        this.Controls.Add(pixelPerfectCheck);

        // 5b. Output Scale selector
        Label scaleLabel = new Label();
        scaleLabel.Text = "OUTPUT SCALE  (reduces VRAM usage on 3DS)";
        scaleLabel.Font = new Font("Segoe UI", 8, FontStyle.Bold);
        scaleLabel.ForeColor = Color.FromArgb(136, 136, 136);
        scaleLabel.Location = new Point(20, 355);
        scaleLabel.Size = new Size(elementWidth, 15);
        this.Controls.Add(scaleLabel);

        scaleCombo = new ComboBox();
        scaleCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        scaleCombo.Items.Add("100%  — full resolution  (best quality, most VRAM)");
        scaleCombo.Items.Add("75%   — slight reduction  (good quality)");
        scaleCombo.Items.Add("50%   — half resolution   (recommended for large overworlds)");
        scaleCombo.Items.Add("33%   — third resolution  (low VRAM, lower quality)");
        scaleCombo.SelectedIndex = 0;
        scaleCombo.Font = new Font("Segoe UI", 9);
        scaleCombo.BackColor = Color.FromArgb(20, 20, 24);
        scaleCombo.ForeColor = Color.FromArgb(224, 224, 224);
        scaleCombo.Location = new Point(20, 373);
        scaleCombo.Size = new Size(elementWidth, 25);
        scaleCombo.Cursor = Cursors.Hand;
        this.Controls.Add(scaleCombo);

        // 6. Convert Button
        convertBtn = new Button();
        convertBtn.Text = "CONVERT & DEPLOY ASSETS";
        convertBtn.Size = new Size(elementWidth, 45);
        convertBtn.Location = new Point(20, 408);
        convertBtn.FlatStyle = FlatStyle.Flat;
        convertBtn.FlatAppearance.BorderSize = 0;
        convertBtn.BackColor = Color.FromArgb(0, 242, 255);
        convertBtn.ForeColor = Color.FromArgb(10, 10, 12);
        convertBtn.Font = new Font("Segoe UI", 10, FontStyle.Bold);
        convertBtn.Cursor = Cursors.Hand;
        convertBtn.Click += (s, e) => ConvertAssets();
        this.Controls.Add(convertBtn);

        // 6b. Calibrate Player Marker Button
        Button calibrateBtn = new Button();
        calibrateBtn.Text = "CALIBRATE PLAYER MARKER";
        calibrateBtn.Size = new Size(elementWidth, 35);
        calibrateBtn.Location = new Point(20, 458);
        calibrateBtn.FlatStyle = FlatStyle.Flat;
        calibrateBtn.FlatAppearance.BorderColor = Color.FromArgb(0, 242, 255);
        calibrateBtn.FlatAppearance.BorderSize = 1;
        calibrateBtn.BackColor = Color.FromArgb(10, 10, 12);
        calibrateBtn.ForeColor = Color.FromArgb(0, 242, 255);
        calibrateBtn.Font = new Font("Segoe UI", 9, FontStyle.Bold);
        calibrateBtn.Cursor = Cursors.Hand;
        calibrateBtn.Click += (s, e) => {
            // Find a map image from the staged files to pre-load
            string preloadImage = "";
            foreach (string f in selectedFiles)
            {
                string ext = Path.GetExtension(f).ToLower();
                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
                { preloadImage = f; break; }
            }
            using (var dlg = new MapCalibrationDialog(preloadImage, detectedRomName, romDirectory, this))
                dlg.ShowDialog();
        };
        this.Controls.Add(calibrateBtn);

        // 7. Log Console Header Label
        Label logHeader = new Label();
        logHeader.Text = "LOG CONSOLE";
        logHeader.Font = new Font("Segoe UI", 8, FontStyle.Bold);
        logHeader.ForeColor = Color.FromArgb(136, 136, 136);
        logHeader.Location = new Point(20, 503);
        logHeader.Size = new Size(elementWidth, 15);
        this.Controls.Add(logHeader);

        // Log Text Box Console
        logConsole = new TextBox();
        logConsole.Multiline = true;
        logConsole.ReadOnly = true;
        logConsole.ScrollBars = ScrollBars.Vertical;
        logConsole.Size = new Size(elementWidth, 110);
        logConsole.Location = new Point(20, 521);
        logConsole.BackColor = Color.FromArgb(15, 15, 18); // Modern pitch-black terminal
        logConsole.ForeColor = Color.FromArgb(0, 255, 136); // Emerald terminal green
        logConsole.BorderStyle = BorderStyle.FixedSingle;
        logConsole.Font = new Font("Consolas", 9);
        this.Controls.Add(logConsole);
 
        // 8. Sleek Dynamic Footer Brand Bar (Large, perfectly legible font)
        Label footerLabel = new Label();
        footerLabel.Text = "Made with love by HristValkyrja - thatgameovergirl@gmail.com";
        footerLabel.Font = new Font("Segoe UI", 10, FontStyle.Bold | FontStyle.Italic);
        footerLabel.ForeColor = Color.FromArgb(180, 180, 190); // Highly readable silver
        footerLabel.Location = new Point(20, 638);
        footerLabel.Size = new Size(elementWidth, 25);
        footerLabel.TextAlign = ContentAlignment.MiddleCenter;
        this.Controls.Add(footerLabel);
    }

    internal void AddLog(string message, string type = "info")
    {
        string timestamp = DateTime.Now.ToString("HH:mm:ss");
        string prefix = "[" + timestamp + "] ";
        if (type == "success") prefix += ">>> ";
        if (type == "error") prefix += "[ERROR] ";

        logConsole.AppendText(prefix + message + Environment.NewLine);
        logConsole.SelectionStart = logConsole.TextLength;
        logConsole.ScrollToCaret();
    }

    private void BrowseFiles()
    {
        using (OpenFileDialog openFileDialog = new OpenFileDialog())
        {
            openFileDialog.Multiselect = true;
            openFileDialog.Filter = "Supported Files (*.nes;*.fds;*.png;*.jpg;*.jpeg;*.bmp;*.txt)|*.nes;*.fds;*.png;*.jpg;*.jpeg;*.bmp;*.txt";
            if (openFileDialog.ShowDialog() == DialogResult.OK)
            {
                HandleDroppedFiles(openFileDialog.FileNames);
            }
        }
    }

    private void HandleDroppedFiles(string[] files)
    {
        foreach (string file in files)
        {
            string ext = Path.GetExtension(file).ToLower();

            // Explicitly reject ZIP, HTML, WEBP
            if (ext == ".zip" || ext == ".html" || ext == ".webp")
            {
                AddLog("Rejected: \"" + Path.GetFileName(file) + "\" (ZIP, HTML, and WEBP formats are not supported)", "error");
                continue;
            }

            // Validate against supported types
            bool isRom = (ext == ".nes" || ext == ".fds" || ext == ".unf");
            bool isMap = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp");
            bool isGuide = (ext == ".txt");

            if (!isRom && !isMap && !isGuide)
            {
                AddLog("Rejected: \"" + Path.GetFileName(file) + "\" (Unsupported file format)", "error");
                continue;
            }

            if (!selectedFiles.Contains(file))
            {
                selectedFiles.Add(file);
                fileListBox.Items.Add(Path.GetFileName(file));
                AddLog("File added: " + Path.GetFileName(file));

                if (isRom)
                {
                    loadedRomPath = file;
                    string rawName = Path.GetFileNameWithoutExtension(file);
                    detectedRomName = SanitizeTitle(rawName);
                    romDirectory = Path.GetDirectoryName(file);
                    AddLog("ROM detected: \"" + Path.GetFileName(file) + "\". Game title locked to: \"" + detectedRomName + "\"", "success");
                }
            }
        }
    }

    private async void ConvertAssets()
    {
        if (selectedFiles.Count == 0)
        {
            AddLog("No files selected!", "error");
            return;
        }

        if (string.IsNullOrEmpty(detectedRomName))
        {
            AddLog("No ROM file detected! Please drag and drop your NES or FDS ROM file first to set the game title.", "error");
            return;
        }

        convertBtn.Enabled = false;
        AddLog("STARTING CONVERSION FOR GAME: " + detectedRomName, "success");

        string title = detectedRomName;
        List<string> maps = new List<string>();
        List<string> guides = new List<string>();

        foreach (string file in selectedFiles)
        {
            string ext = Path.GetExtension(file).ToLower();
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
            {
                maps.Add(file);
            }
            else if (ext == ".txt" && Path.GetFileNameWithoutExtension(file).ToLower() != title.ToLower())
            {
                guides.Add(file);
            }
        }

        // 1. Process Maps
        if (maps.Count > 0)
        {
            if (maps.Count > 1)
            {
                using (MapTypeDialog dlg = new MapTypeDialog(this, maps))
                {
                    if (dlg.ShowDialog() != DialogResult.OK)
                    {
                        AddLog("Conversion aborted or cancelled.", "error");
                        convertBtn.Enabled = true;
                        return;
                    }
                }
            }
            else
            {
                // Single map automatically converts
                string outPath = Path.Combine(romDirectory, title + ".v2m");
                bool isPixelPerfect = pixelPerfectCheck.Checked;
                float scale = GetSelectedScale();
                bool mapOk = await System.Threading.Tasks.Task.Run(() => ProcessImageToV2M(maps[0], outPath, isPixelPerfect, scale));
                if (mapOk)
                {
                    AddLog("Exported Map: " + title + ".v2m", "success");
                    MessageBox.Show("Conversion successfully completed!", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                }
                else
                {
                    AddLog("Failed to process map: " + Path.GetFileName(maps[0]), "error");
                    MessageBox.Show("Conversion Aborted due to an error!", "Aborted", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    convertBtn.Enabled = true;
                    return;
                }
            }
        }
        else
        {
            AddLog("No map images to process.");
        }

        // 2. Process Guides
        if (guides.Count > 0)
        {
            for (int i = 0; i < guides.Count; i++)
            {
                string file = guides[i];
                string outGuideName = (guides.Count == 1) ? (title + ".txt") : (title + "-" + (i + 1) + ".txt");
                string outPath = Path.Combine(romDirectory, outGuideName);
                
                bool guideOk = await System.Threading.Tasks.Task.Run(() => CopyFile(file, outPath));
                if (guideOk)
                {
                    AddLog("Exported Guide: " + outGuideName, "success");
                }
            }
        }
        else
        {
            AddLog("No guides to process.");
        }

        AddLog("CONVERSION COMPLETE! Assets saved next to your ROM.", "success");
        convertBtn.Enabled = true;
    }

    internal float GetSelectedScale()
    {
        switch (scaleCombo.SelectedIndex)
        {
            case 1: return 0.75f;
            case 2: return 0.50f;
            case 3: return 0.33f;
            default: return 1.00f;
        }
    }

    internal bool ProcessImageToV2M(string inputPath, string outputPath, bool pixelPerfect, float scale = 1.0f)
    {
        try
        {
            using (Bitmap srcBmp = new Bitmap(inputPath))
            {
                int width = srcBmp.Width;
                int height = srcBmp.Height;

                int minX = width;
                int maxX = 0;
                int minY = height;
                int maxY = 0;
                bool hasContent = false;

                // Auto-border crop scan
                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        Color pixel = srcBmp.GetPixel(x, y);
                        if (pixel.A > 10 && (pixel.R > 15 || pixel.G > 15 || pixel.B > 15))
                        {
                            if (x < minX) minX = x;
                            if (x > maxX) maxX = x;
                            if (y < minY) minY = y;
                            if (y > maxY) maxY = y;
                            hasContent = true;
                        }
                    }
                }

                if (!hasContent)
                {
                    minX = 0; maxX = width - 1;
                    minY = 0; maxY = height - 1;
                }

                int srcActiveW = (maxX - minX) + 1;
                int srcActiveH = (maxY - minY) + 1;

                // Apply output scale: shrinks the logical canvas so the
                // resulting .v2m uses fewer 1024×1024 tiles (less 3DS VRAM).
                int activeW = Math.Max(1, (int)Math.Round(srcActiveW * scale));
                int activeH = Math.Max(1, (int)Math.Round(srcActiveH * scale));

                // Calculate grid columns and rows based on 1024x1024 tile size
                int gridCols = (int)Math.Ceiling((double)activeW / 1024.0);
                int gridRows = (int)Math.Ceiling((double)activeH / 1024.0);

                using (BinaryWriter bw = new BinaryWriter(File.Open(outputPath, FileMode.Create)))
                {
                    // Write V2M v2 header
                    bw.Write(new char[] { 'V', 'M', '\x02', '\x00' });
                    bw.Write(activeW);
                    bw.Write(activeH);
                    bw.Write(gridCols);
                    bw.Write(gridRows);
                    bw.Write(2); // Format: 2 = RGB565 pre-swizzled (PICA200 Morton tiled)

                    // Process each tile in the grid
                    for (int r = 0; r < gridRows; r++)
                    {
                        for (int c = 0; c < gridCols; c++)
                        {
                            // Destination tile content area (in output/scaled space)
                            int tileW = Math.Min(1024, activeW - c * 1024);
                            int tileH = Math.Min(1024, activeH - r * 1024);

                            // Corresponding source region (in original image space)
                            int srcTileX = minX + (int)Math.Round(c * 1024 / scale);
                            int srcTileY = minY + (int)Math.Round(r * 1024 / scale);
                            int srcTileW = (int)Math.Round(tileW / scale);
                            int srcTileH = (int)Math.Round(tileH / scale);
                            // Clamp to source bounds
                            srcTileW = Math.Min(srcTileW, srcBmp.Width  - srcTileX);
                            srcTileH = Math.Min(srcTileH, srcBmp.Height - srcTileY);

                            bw.Write(tileW);
                            bw.Write(tileH);

                            using (Bitmap tileBmp = new Bitmap(1024, 1024))
                            {
                                using (Graphics g = Graphics.FromImage(tileBmp))
                                {
                                    g.Clear(Color.Black);
                                    // Use high-quality bicubic for scaled output so detail
                                    // is preserved even at 50% — NearestNeighbor only for 100%.
                                    g.InterpolationMode = (scale >= 1.0f)
                                        ? InterpolationMode.NearestNeighbor
                                        : InterpolationMode.HighQualityBicubic;
                                    g.PixelOffsetMode = PixelOffsetMode.Half;
                                    // Draw source region → scaled tile content area
                                    g.DrawImage(srcBmp,
                                        new Rectangle(0, 0, tileW, tileH),
                                        new Rectangle(srcTileX, srcTileY, srcTileW, srcTileH),
                                        GraphicsUnit.Pixel);
                                }

                                // Lock bits to convert to RGB565
                                var rect = new Rectangle(0, 0, 1024, 1024);
                                var data = tileBmp.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppRgb);
                                int bytesCount = data.Stride * data.Height;
                                byte[] rgbValues = new byte[bytesCount];
                                Marshal.Copy(data.Scan0, rgbValues, 0, bytesCount);
                                tileBmp.UnlockBits(data);

                                // Step 1: convert pixels to linear RGB565
                                byte[] rgb565Data = new byte[1024 * 1024 * 2];
                                int idx565 = 0;
                                for (int i = 0; i < bytesCount; i += 4)
                                {
                                    byte blue  = rgbValues[i];
                                    byte green = rgbValues[i + 1];
                                    byte red   = rgbValues[i + 2];

                                    ushort pixel565 = (ushort)(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
                                    rgb565Data[idx565++] = (byte)(pixel565 & 0xFF);
                                    rgb565Data[idx565++] = (byte)((pixel565 >> 8) & 0xFF);
                                }

                                // Step 2: Morton-swizzle into PICA200 tiled layout.
                                // Doing this on the PC (instant) means the 3DS loader
                                // can skip the swizzle loop entirely and go straight to
                                // C3D_TexUpload — no CPU stall during map loading.
                                byte[] tiledData = new byte[1024 * 1024 * 2];
                                for (int py = 0; py < 1024; py++)
                                {
                                    for (int px = 0; px < 1024; px++)
                                    {
                                        int pixelX = px % 8;
                                        int pixelY = py % 8;

                                        int morton = 0;
                                        morton |= (pixelX & 1) << 0;
                                        morton |= (pixelY & 1) << 1;
                                        morton |= (pixelX & 2) << 1;
                                        morton |= (pixelY & 2) << 2;
                                        morton |= (pixelX & 4) << 2;
                                        morton |= (pixelY & 4) << 3;

                                        int blockIndex  = (py / 8) * (1024 / 8) + (px / 8);
                                        int tiledOffset = (blockIndex * 64 + morton) * 2;
                                        int linearOffset = (py * 1024 + px) * 2;

                                        tiledData[tiledOffset]     = rgb565Data[linearOffset];
                                        tiledData[tiledOffset + 1] = rgb565Data[linearOffset + 1];
                                    }
                                }

                                bw.Write(tiledData);
                            }
                        }
                    }
                }
            }
            return true;
        }
        catch (Exception)
        {
            return false;
        }
    }

    private bool CopyFile(string inputPath, string outputPath)
    {
        try
        {
            File.Copy(inputPath, outputPath, true);
            return true;
        }
        catch (Exception)
        {
            return false;
        }
    }
}

public class MapConfig
{
    public string FilePath { get; set; }
    public string Suffix { get; set; } // "" for Overworld
}

internal class MapTypeDialog : Form
{
    private List<string> mapPaths;
    private List<MapConfig> results = new List<MapConfig>();
    private V2MCompilerForm parent;
    private VerticalProgressBar progressBar;

    public List<MapConfig> Results { get { return results; } }

    internal MapTypeDialog(V2MCompilerForm parent, List<string> paths)
    {
        this.parent = parent;
        this.mapPaths = paths;
        this.Text = "Assign Map Roles";
        this.Size = new Size(740, 100 + paths.Count * 60);
        this.BackColor = Color.FromArgb(15, 15, 18);
        this.ForeColor = Color.FromArgb(224, 224, 224);
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.MaximizeBox = false;
        this.MinimizeBox = false;
        this.StartPosition = FormStartPosition.CenterParent;
 
        // Dynamic Adventure Map Icon Generation (Vintage parchment map with grid, dotted path, and red X)
        try
        {
            using (Bitmap iconBmp = new Bitmap(32, 32))
            {
                using (Graphics g = Graphics.FromImage(iconBmp))
                {
                    g.Clear(Color.FromArgb(235, 205, 150)); // Vintage parchment paper background
                    
                    // Draw soft coordinate grid lines
                    using (Pen gridPen = new Pen(Color.FromArgb(200, 170, 120), 1f))
                    {
                        g.DrawLine(gridPen, 8, 0, 8, 32);
                        g.DrawLine(gridPen, 16, 0, 16, 32);
                        g.DrawLine(gridPen, 24, 0, 24, 32);
                        g.DrawLine(gridPen, 0, 8, 32, 8);
                        g.DrawLine(gridPen, 0, 16, 32, 16);
                        g.DrawLine(gridPen, 0, 24, 32, 24);
                    }
                    
                    // Draw a curving dotted path leading to the treasure
                    using (Pen pathPen = new Pen(Color.FromArgb(220, 20, 60), 2f))
                    {
                        pathPen.DashStyle = DashStyle.Dot;
                        g.DrawCurve(pathPen, new Point[] {
                            new Point(4, 28),
                            new Point(12, 20),
                            new Point(20, 22),
                            new Point(26, 10)
                        });
                    }
                    
                    // Draw a bold red "X marks the spot"
                    using (Font xFont = new Font("Arial", 9, FontStyle.Bold))
                    using (Brush xBrush = new SolidBrush(Color.FromArgb(220, 0, 0)))
                    {
                        g.DrawString("X", xFont, xBrush, 21, 2);
                    }
                }
                this.Icon = Icon.FromHandle(iconBmp.GetHicon());
            }
        }
        catch {}

        Label header = new Label();
        header.Text = "Please assign a role/level for each map image:";
        header.Font = new Font("Segoe UI", 10, FontStyle.Bold);
        header.Location = new Point(20, 15);
        header.Size = new Size(540, 25);
        header.ForeColor = Color.FromArgb(0, 242, 255);
        this.Controls.Add(header);

        // Progress label
        Label progressLabel = new Label();
        progressLabel.Text = "PROGRESS";
        progressLabel.Font = new Font("Segoe UI", 8, FontStyle.Bold);
        progressLabel.ForeColor = Color.FromArgb(136, 136, 136);
        progressLabel.Location = new Point(580, 28);
        progressLabel.Size = new Size(120, 15);
        this.Controls.Add(progressLabel);

        // Progress bar on the right
        progressBar = new VerticalProgressBar();
        progressBar.Location = new Point(580, 50);
        progressBar.Size = new Size(120, paths.Count * 50 - 30);
        progressBar.Minimum = 0;
        progressBar.Maximum = paths.Count;
        progressBar.Value = 0;
        progressBar.BackColor = Color.FromArgb(25, 25, 30);
        progressBar.ForeColor = Color.FromArgb(0, 242, 255);
        this.Controls.Add(progressBar);

        int startY = 50;
        List<ComboBox> comboBoxes = new List<ComboBox>();
        List<TextBox> textBoxes = new List<TextBox>();

        for (int i = 0; i < paths.Count; i++)
        {
            string path = paths[i];
            string filename = Path.GetFileName(path);

            Label fileLbl = new Label();
            fileLbl.Text = filename;
            fileLbl.Font = new Font("Segoe UI", 8);
            fileLbl.Location = new Point(20, startY + i * 50);
            fileLbl.Size = new Size(220, 20);
            this.Controls.Add(fileLbl);

            ComboBox typeCombo = new ComboBox();
            typeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            typeCombo.Items.Add("Overworld (Base Map)");
            typeCombo.Items.Add("Level 1 / Dungeon 1");
            typeCombo.Items.Add("Level 2 / Dungeon 2");
            typeCombo.Items.Add("Level 3 / Dungeon 3");
            typeCombo.Items.Add("Level 4 / Dungeon 4");
            typeCombo.Items.Add("Level 5 / Dungeon 5");
            typeCombo.Items.Add("Level 6 / Dungeon 6");
            typeCombo.Items.Add("Level 7 / Dungeon 7");
            typeCombo.Items.Add("Level 8 / Dungeon 8");
            typeCombo.Items.Add("Level 9 / Dungeon 9");
            typeCombo.Items.Add("Custom Suffix...");
            typeCombo.Location = new Point(250, startY + i * 50);
            typeCombo.Size = new Size(180, 21);
            typeCombo.BackColor = Color.FromArgb(30, 30, 35);
            typeCombo.ForeColor = Color.FromArgb(224, 224, 224);
            this.Controls.Add(typeCombo);
            comboBoxes.Add(typeCombo);

            TextBox customTxt = new TextBox();
            customTxt.Location = new Point(440, startY + i * 50);
            customTxt.Size = new Size(120, 20);
            customTxt.BackColor = Color.FromArgb(30, 30, 35);
            customTxt.ForeColor = Color.FromArgb(224, 224, 224);
            customTxt.Visible = false; // Hidden by default!
            this.Controls.Add(customTxt);
            textBoxes.Add(customTxt);

            // Toggle custom text box based on ComboBox selection
            int index = i;
            typeCombo.SelectedIndexChanged += (s, e) => {
                customTxt.Visible = (typeCombo.SelectedIndex == 10);
            };

            // Smart Auto-Detection
            string lowerName = filename.ToLower();
            if (lowerName.Contains("overworld") || lowerName.Contains("worldmap") || lowerName.Contains("main"))
            {
                typeCombo.SelectedIndex = 0; // Overworld
            }
            else if (lowerName.Contains("level1") || lowerName.Contains("level 1") || lowerName.Contains("dungeon1") || lowerName.Contains("dungeon 1"))
            {
                typeCombo.SelectedIndex = 1; // Level 1
            }
            else if (lowerName.Contains("level2") || lowerName.Contains("level 2") || lowerName.Contains("dungeon2") || lowerName.Contains("dungeon 2"))
            {
                typeCombo.SelectedIndex = 2; // Level 2
            }
            else if (lowerName.Contains("level3") || lowerName.Contains("level 3") || lowerName.Contains("dungeon3") || lowerName.Contains("dungeon 3"))
            {
                typeCombo.SelectedIndex = 3; // Level 3
            }
            else if (lowerName.Contains("level4") || lowerName.Contains("level 4") || lowerName.Contains("dungeon4") || lowerName.Contains("dungeon 4"))
            {
                typeCombo.SelectedIndex = 4; // Level 4
            }
            else if (lowerName.Contains("level5") || lowerName.Contains("level 5") || lowerName.Contains("dungeon5") || lowerName.Contains("dungeon 5"))
            {
                typeCombo.SelectedIndex = 5; // Level 5
            }
            else if (lowerName.Contains("level6") || lowerName.Contains("level 6") || lowerName.Contains("dungeon6") || lowerName.Contains("dungeon 6"))
            {
                typeCombo.SelectedIndex = 6; // Level 6
            }
            else if (lowerName.Contains("level7") || lowerName.Contains("level 7") || lowerName.Contains("dungeon7") || lowerName.Contains("dungeon 7"))
            {
                typeCombo.SelectedIndex = 7; // Level 7
            }
            else if (lowerName.Contains("level8") || lowerName.Contains("level 8") || lowerName.Contains("dungeon8") || lowerName.Contains("dungeon 8"))
            {
                typeCombo.SelectedIndex = 8; // Level 8
            }
            else if (lowerName.Contains("level9") || lowerName.Contains("level 9") || lowerName.Contains("dungeon9") || lowerName.Contains("dungeon 9"))
            {
                typeCombo.SelectedIndex = 9; // Level 9
            }
            else
            {
                typeCombo.SelectedIndex = 0; // Default to Overworld
            }
        }

        Button confirmBtn = new Button();
        confirmBtn.Text = "CONFIRM & EXPORT";
        confirmBtn.Font = new Font("Segoe UI", 9, FontStyle.Bold);
        confirmBtn.FlatStyle = FlatStyle.Flat;
        confirmBtn.FlatAppearance.BorderSize = 0;
        confirmBtn.BackColor = Color.FromArgb(0, 242, 255);
        confirmBtn.ForeColor = Color.FromArgb(10, 10, 12);
        confirmBtn.Size = new Size(200, 35);
        confirmBtn.Location = new Point(260, startY + paths.Count * 50 + 15);
        confirmBtn.Click += async (s, e) => {
            confirmBtn.Enabled = false;
            foreach (var cb in comboBoxes) cb.Enabled = false;
            foreach (var tb in textBoxes) tb.Enabled = false;

            bool hasError = false;
            for (int i = 0; i < paths.Count; i++)
            {
                string filename = Path.GetFileName(paths[i]);
                string suffix = "";
                int sel = comboBoxes[i].SelectedIndex;
                if (sel == 0) suffix = "";
                else if (sel >= 1 && sel <= 9) suffix = "Level" + sel;
                else if (sel == 10) suffix = textBoxes[i].Text.Trim();

                string outMapName = "";
                if (string.IsNullOrEmpty(suffix))
                {
                    outMapName = parent.detectedRomName + ".v2m";
                }
                else
                {
                    outMapName = parent.detectedRomName + "-" + suffix + ".v2m";
                }

                string outPath = Path.Combine(parent.romDirectory, outMapName);
                parent.AddLog("Processing map: " + filename + " -> " + outMapName);

                bool success = await System.Threading.Tasks.Task.Run(() => parent.ProcessImageToV2M(paths[i], outPath, parent.pixelPerfectCheck.Checked, parent.GetSelectedScale()));
                if (success)
                {
                    parent.AddLog("Exported Map: " + outMapName, "success");
                    progressBar.Value = i + 1;
                }
                else
                {
                    parent.AddLog("Failed to process map: " + filename, "error");
                    hasError = true;
                    break;
                }
            }

            if (hasError)
            {
                MessageBox.Show("Conversion Aborted due to an error!", "Aborted", MessageBoxButtons.OK, MessageBoxIcon.Error);
                this.DialogResult = DialogResult.Cancel;
            }
            else
            {
                MessageBox.Show("Conversion successfully completed!", "Success", MessageBoxButtons.OK, MessageBoxIcon.Information);
                this.DialogResult = DialogResult.OK;
            }
            this.Close();
        };
        this.Controls.Add(confirmBtn);

        this.Size = new Size(740, startY + paths.Count * 50 + 110);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// MAP CALIBRATION DIALOG
// Lets the user click two reference corners on their overworld map image so
// V2NES can draw an accurate player-position dot while the game is running.
//
// The tool writes a plain-text .pos sidecar file next to the .v2m:
//   game=Zelda
//   ram_addr=235
//   world_cols=16
//   world_rows=8
//   map_left=0.0075    ← normalised X of screen-0 top-left  (0..1)
//   map_top=0.01       ← normalised Y of screen-0 top-left
//   map_right=0.9875   ← normalised X of last-screen bottom-right
//   map_bottom=0.9875  ← normalised Y of last-screen bottom-right
//   img_width=1600     ← original image dimensions (for reference)
//   img_height=800
//
// V2NES reads this file when it loads the map, then every frame it reads
// ram_addr, converts (screen % world_cols, screen / world_cols) to normalised
// coords, and draws a small cyan square over the active screen cell.
// ─────────────────────────────────────────────────────────────────────────────
public class MapCalibrationDialog : Form
{
    // ── UI controls ──────────────────────────────────────────────────────────
    private PictureBox mapBox;
    private ComboBox gameCombo;
    private Label instructionLabel;
    private Label coordLabel;
    private Button loadImageBtn, exportBtn, resetBtn;
    private TextBox ramAddrBox, colsBox, rowsBox;
    private TextBox outputPathBox;
    private Button browseOutputBtn;

    // ── State ────────────────────────────────────────────────────────────────
    private Bitmap originalImage;           // the full-res source image
    private Bitmap displayImage;            // scaled version shown in mapBox
    private float displayScale = 1.0f;      // displayImage / originalImage ratio

    private PointF ref1 = PointF.Empty;     // click 1: top-left of screen(0,0)
    private PointF ref2 = PointF.Empty;     // click 2: bottom-right of last screen
    private int clickState = 0;             // 0=waiting for click1, 1=waiting for click2, 2=done

    private string preloadImagePath;
    private string detectedRomName;
    private string romDirectory;
    private V2MCompilerForm parent;

    // ── Known game presets ────────────────────────────────────────────────────
    private static readonly string[] GameNames = {
        "The Legend of Zelda",
        "Dragon Warrior",
        "Final Fantasy",
        "Custom / Other"
    };
    private static readonly int[] GameRamAddr   = { 0xEB, 0x3B, 0x4C, 0x00 };
    private static readonly int[] GameWorldCols = { 16,   120,  256,  16   };
    private static readonly int[] GameWorldRows = { 8,    120,  256,  8    };

    internal MapCalibrationDialog(string preloadPath, string romName, string romDir, V2MCompilerForm par)
    {
        preloadImagePath = preloadPath;
        detectedRomName  = romName;
        romDirectory     = romDir;
        parent           = par;

        this.Text = "Player Marker Calibration";
        this.Size = new Size(860, 680);
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.MaximizeBox = false;
        this.StartPosition = FormStartPosition.CenterParent;
        this.BackColor = Color.FromArgb(10, 10, 12);
        this.ForeColor = Color.FromArgb(224, 224, 224);

        BuildUI();

        if (!string.IsNullOrEmpty(preloadImagePath) && File.Exists(preloadImagePath))
            LoadImage(preloadImagePath);

        // Auto-detect game preset from ROM name, falling back to the image filename.
        // This way the preset is picked automatically whether a ROM was dropped or not.
        string detectFrom = !string.IsNullOrEmpty(romName)
            ? romName
            : Path.GetFileNameWithoutExtension(preloadImagePath ?? "");
        ApplyGamePreset(detectFrom);

        // Default output path
        if (!string.IsNullOrEmpty(romDir) && !string.IsNullOrEmpty(romName))
            outputPathBox.Text = Path.Combine(romDir, romName + "-Overworld.pos");
    }

    private void BuildUI()
    {
        int leftW = 200;

        // ── Left panel ───────────────────────────────────────────────────────
        Label gameLabel = MakeLabel("GAME PRESET", 10, 10, leftW);
        this.Controls.Add(gameLabel);

        gameCombo = new ComboBox();
        gameCombo.DropDownStyle = ComboBoxStyle.DropDownList;
        foreach (string g in GameNames) gameCombo.Items.Add(g);
        gameCombo.Location = new Point(10, 28);
        gameCombo.Size = new Size(leftW, 22);
        gameCombo.BackColor = Color.FromArgb(20, 20, 24);
        gameCombo.ForeColor = Color.FromArgb(224, 224, 224);
        gameCombo.SelectedIndexChanged += GameCombo_Changed;
        this.Controls.Add(gameCombo);

        this.Controls.Add(MakeLabel("RAM ADDRESS (hex)", 10, 60, leftW));
        ramAddrBox = MakeTextBox("EB", 10, 78, 90);
        this.Controls.Add(ramAddrBox);

        this.Controls.Add(MakeLabel("WORLD COLS", 10, 108, 90));
        colsBox = MakeTextBox("16", 10, 126, 60);
        this.Controls.Add(colsBox);

        this.Controls.Add(MakeLabel("WORLD ROWS", 110, 108, 90));
        rowsBox = MakeTextBox("8", 110, 126, 60);
        this.Controls.Add(rowsBox);

        loadImageBtn = MakeButton("LOAD MAP IMAGE", 10, 165, leftW, false);
        loadImageBtn.Click += (s, e) => BrowseImage();
        this.Controls.Add(loadImageBtn);

        resetBtn = MakeButton("RESET POINTS", 10, 205, leftW, false);
        resetBtn.Click += (s, e) => ResetPoints();
        this.Controls.Add(resetBtn);

        // Instructions box
        instructionLabel = new Label();
        instructionLabel.Text = "Step 1: Load your overworld map image.\n\n" +
                                 "Step 2: Click the TOP-LEFT corner of screen (0,0) — the very first cell in the top-left of the map grid.\n\n" +
                                 "Step 3: Click the BOTTOM-RIGHT corner of the last screen cell.\n\n" +
                                 "Step 4: Verify the grid overlay looks correct, then export.";
        instructionLabel.Font = new Font("Segoe UI", 8);
        instructionLabel.ForeColor = Color.FromArgb(180, 180, 190);
        instructionLabel.Location = new Point(10, 248);
        instructionLabel.Size = new Size(leftW, 200);
        this.Controls.Add(instructionLabel);

        coordLabel = new Label();
        coordLabel.Text = "";
        coordLabel.Font = new Font("Consolas", 8);
        coordLabel.ForeColor = Color.FromArgb(0, 242, 255);
        coordLabel.Location = new Point(10, 455);
        coordLabel.Size = new Size(leftW, 80);
        this.Controls.Add(coordLabel);

        // Output path
        this.Controls.Add(MakeLabel("OUTPUT .POS FILE", 10, 540, leftW));
        outputPathBox = MakeTextBox("", 10, 558, leftW - 28);
        this.Controls.Add(outputPathBox);
        browseOutputBtn = MakeButton("...", leftW - 16, 558, 26, false);
        browseOutputBtn.Click += (s, e) => BrowseOutputPath();
        this.Controls.Add(browseOutputBtn);

        exportBtn = MakeButton("EXPORT .POS FILE", 10, 592, leftW, true);
        exportBtn.Click += (s, e) => ExportPos();
        this.Controls.Add(exportBtn);

        // ── Map preview box ──────────────────────────────────────────────────
        mapBox = new PictureBox();
        mapBox.Location = new Point(220, 10);
        mapBox.Size = new Size(620, 610);
        mapBox.BackColor = Color.FromArgb(20, 20, 24);
        mapBox.BorderStyle = BorderStyle.FixedSingle;
        mapBox.SizeMode = PictureBoxSizeMode.Normal;
        mapBox.Cursor = Cursors.Cross;
        mapBox.MouseClick += MapBox_Click;
        mapBox.Paint += MapBox_Paint;
        this.Controls.Add(mapBox);
    }

    // ── UI helpers ────────────────────────────────────────────────────────────
    private Label MakeLabel(string text, int x, int y, int w)
    {
        return new Label {
            Text = text, Font = new Font("Segoe UI", 7, FontStyle.Bold),
            ForeColor = Color.FromArgb(136, 136, 136),
            Location = new Point(x, y), Size = new Size(w, 14)
        };
    }
    private TextBox MakeTextBox(string val, int x, int y, int w)
    {
        return new TextBox {
            Text = val, Location = new Point(x, y), Size = new Size(w, 20),
            BackColor = Color.FromArgb(20, 20, 24), ForeColor = Color.FromArgb(224, 224, 224),
            BorderStyle = BorderStyle.FixedSingle, Font = new Font("Consolas", 9)
        };
    }
    private Button MakeButton(string text, int x, int y, int w, bool accent)
    {
        var b = new Button {
            Text = text, Location = new Point(x, y), Size = new Size(w, 28),
            FlatStyle = FlatStyle.Flat, Font = new Font("Segoe UI", 8, FontStyle.Bold),
            Cursor = Cursors.Hand
        };
        if (accent) {
            b.BackColor = Color.FromArgb(0, 242, 255);
            b.ForeColor = Color.FromArgb(10, 10, 12);
            b.FlatAppearance.BorderSize = 0;
        } else {
            b.BackColor = Color.FromArgb(10, 10, 12);
            b.ForeColor = Color.FromArgb(0, 242, 255);
            b.FlatAppearance.BorderColor = Color.FromArgb(0, 242, 255);
            b.FlatAppearance.BorderSize = 1;
        }
        return b;
    }

    // ── Preset selection ──────────────────────────────────────────────────────
    private void GameCombo_Changed(object sender, EventArgs e)
    {
        int i = gameCombo.SelectedIndex;
        if (i < 0 || i >= GameRamAddr.Length) return;
        ramAddrBox.Text = GameRamAddr[i].ToString("X");
        colsBox.Text    = GameWorldCols[i].ToString();
        rowsBox.Text    = GameWorldRows[i].ToString();
    }

    // ── Image loading ─────────────────────────────────────────────────────────
    private void BrowseImage()
    {
        using (var dlg = new OpenFileDialog())
        {
            dlg.Filter = "Image Files (*.png;*.jpg;*.bmp)|*.png;*.jpg;*.bmp";
            if (dlg.ShowDialog() == DialogResult.OK)
                LoadImage(dlg.FileName);
        }
    }
    // Auto-select the game preset combo by matching keywords in a name string
    // (ROM title, image filename, or any other hint). Falls back to Custom.
    private void ApplyGamePreset(string hint)
    {
        if (string.IsNullOrEmpty(hint)) { gameCombo.SelectedIndex = 0; return; }
        string lower = hint.ToLower();
        if      (lower.Contains("zelda"))                              gameCombo.SelectedIndex = 0;
        else if (lower.Contains("dragon") || lower.Contains("warrior")) gameCombo.SelectedIndex = 1;
        else if (lower.Contains("final")  || lower.Contains("fantasy")) gameCombo.SelectedIndex = 2;
        else                                                            gameCombo.SelectedIndex = 3;
    }

    private void LoadImage(string path)
    {
        try
        {
            if (originalImage != null) { originalImage.Dispose(); originalImage = null; }
            if (displayImage  != null) { displayImage.Dispose();  displayImage  = null; }
            originalImage = new Bitmap(path);

            // Scale to fit the 620x610 preview box, preserving aspect ratio
            float scaleX = 620.0f / originalImage.Width;
            float scaleY = 610.0f / originalImage.Height;
            displayScale = Math.Min(scaleX, scaleY);
            int dW = (int)(originalImage.Width  * displayScale);
            int dH = (int)(originalImage.Height * displayScale);
            displayImage = new Bitmap(dW, dH);
            using (Graphics g = Graphics.FromImage(displayImage))
            {
                g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                g.DrawImage(originalImage, 0, 0, dW, dH);
            }
            mapBox.Image = displayImage;
            mapBox.Size  = new Size(dW, dH);
            ResetPoints();

            // If no ROM was dropped, auto-detect preset and output path from the image name.
            string imgStem = Path.GetFileNameWithoutExtension(path);
            if (string.IsNullOrEmpty(detectedRomName))
            {
                ApplyGamePreset(imgStem);
            }

            // Auto-fill output path: prefer ROM-derived name, fall back to image name.
            string outStem = !string.IsNullOrEmpty(detectedRomName) ? detectedRomName : imgStem;
            string outDir  = !string.IsNullOrEmpty(romDirectory)    ? romDirectory    : Path.GetDirectoryName(path);
            outputPathBox.Text = Path.Combine(outDir, outStem + "-Overworld.pos");
        }
        catch (Exception ex)
        {
            MessageBox.Show("Could not load image: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    // ── Click handling ────────────────────────────────────────────────────────
    private void MapBox_Click(object sender, MouseEventArgs e)
    {
        if (originalImage == null) return;
        // Convert display coords → original image coords
        float origX = e.X / displayScale;
        float origY = e.Y / displayScale;

        if (clickState == 0)
        {
            ref1 = new PointF(origX, origY);
            clickState = 1;
            instructionLabel.Text = "✓ Point 1 set!\n\nNow click the BOTTOM-RIGHT corner of the last screen cell.";
            UpdateCoordLabel();
        }
        else if (clickState == 1)
        {
            ref2 = new PointF(origX, origY);
            clickState = 2;
            instructionLabel.Text = "✓ Both points set!\n\nVerify the grid overlay on the map, then click EXPORT.";
            UpdateCoordLabel();
        }
        else
        {
            // Third click resets
            ResetPoints();
        }
        mapBox.Invalidate();
    }

    private void ResetPoints()
    {
        ref1 = ref2 = PointF.Empty;
        clickState = 0;
        instructionLabel.Text = "Step 1: Load your overworld map image.\n\n" +
                                 "Step 2: Click the TOP-LEFT corner of screen (0,0).\n\n" +
                                 "Step 3: Click the BOTTOM-RIGHT corner of the last screen cell.\n\n" +
                                 "Step 4: Verify the grid, then export.";
        coordLabel.Text = "";
        if (mapBox != null) mapBox.Invalidate();
    }

    private void UpdateCoordLabel()
    {
        string s = "";
        if (clickState >= 1)
            s += string.Format("P1: ({0:0}, {1:0})\n", ref1.X, ref1.Y);
        if (clickState >= 2)
        {
            s += string.Format("P2: ({0:0}, {1:0})\n", ref2.X, ref2.Y);
            if (originalImage != null)
            {
                float nLeft   = ref1.X / originalImage.Width;
                float nTop    = ref1.Y / originalImage.Height;
                float nRight  = ref2.X / originalImage.Width;
                float nBottom = ref2.Y / originalImage.Height;
                s += string.Format("\nleft:   {0:0.0000}\ntop:    {1:0.0000}\nright:  {2:0.0000}\nbottom: {3:0.0000}",
                    nLeft, nTop, nRight, nBottom);
            }
        }
        coordLabel.Text = s;
    }

    // ── Grid overlay paint ────────────────────────────────────────────────────
    private void MapBox_Paint(object sender, PaintEventArgs e)
    {
        if (clickState == 0 || originalImage == null) return;

        // Draw ref1 crosshair
        using (Pen cyan = new Pen(Color.FromArgb(0, 242, 255), 1))
        {
            float d1x = ref1.X * displayScale;
            float d1y = ref1.Y * displayScale;
            e.Graphics.DrawLine(cyan, d1x - 8, d1y, d1x + 8, d1y);
            e.Graphics.DrawLine(cyan, d1x, d1y - 8, d1x, d1y + 8);
        }

        if (clickState < 2) return;

        // Draw ref2 crosshair
        using (Pen red = new Pen(Color.FromArgb(255, 60, 60), 1))
        {
            float d2x = ref2.X * displayScale;
            float d2y = ref2.Y * displayScale;
            e.Graphics.DrawLine(red, d2x - 8, d2y, d2x + 8, d2y);
            e.Graphics.DrawLine(red, d2x, d2y - 8, d2x, d2y + 8);
        }

        // Draw grid overlay
        int cols, rows;
        if (!int.TryParse(colsBox.Text, out cols) || cols < 1) return;
        if (!int.TryParse(rowsBox.Text, out rows) || rows < 1) return;

        float gridLeft   = ref1.X * displayScale;
        float gridTop    = ref1.Y * displayScale;
        float gridRight  = ref2.X * displayScale;
        float gridBottom = ref2.Y * displayScale;
        float cellW = (gridRight  - gridLeft) / cols;
        float cellH = (gridBottom - gridTop)  / rows;

        using (Pen gridPen = new Pen(Color.FromArgb(80, 0, 242, 255), 1))
        {
            for (int c = 0; c <= cols; c++)
                e.Graphics.DrawLine(gridPen, gridLeft + c * cellW, gridTop,
                                             gridLeft + c * cellW, gridBottom);
            for (int r = 0; r <= rows; r++)
                e.Graphics.DrawLine(gridPen, gridLeft, gridTop + r * cellH,
                                             gridRight, gridTop + r * cellH);
        }

        // Highlight cell (0,0) in cyan
        using (Brush highlightBrush = new SolidBrush(Color.FromArgb(40, 0, 242, 255)))
            e.Graphics.FillRectangle(highlightBrush, gridLeft, gridTop, cellW, cellH);
    }

    // ── Export ────────────────────────────────────────────────────────────────
    private void BrowseOutputPath()
    {
        using (var dlg = new SaveFileDialog())
        {
            dlg.Filter = "Position Files (*.pos)|*.pos";
            dlg.FileName = Path.GetFileName(outputPathBox.Text);
            if (!string.IsNullOrEmpty(outputPathBox.Text))
                dlg.InitialDirectory = Path.GetDirectoryName(outputPathBox.Text);
            if (dlg.ShowDialog() == DialogResult.OK)
                outputPathBox.Text = dlg.FileName;
        }
    }

    private void ExportPos()
    {
        if (clickState < 2)
        {
            MessageBox.Show("Please set both reference points first.", "Not ready",
                MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }
        if (originalImage == null)
        {
            MessageBox.Show("No image loaded.", "Not ready", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }
        if (string.IsNullOrWhiteSpace(outputPathBox.Text))
        {
            MessageBox.Show("Please set an output path.", "Not ready", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            return;
        }

        int ramAddr, cols, rows;
        try { ramAddr = Convert.ToInt32(ramAddrBox.Text.Replace("0x","").Replace("0X",""), 16); }
        catch { MessageBox.Show("Invalid RAM address. Enter a hex value like EB", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }
        if (!int.TryParse(colsBox.Text, out cols) || cols < 1)
        { MessageBox.Show("Invalid world columns.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }
        if (!int.TryParse(rowsBox.Text, out rows) || rows < 1)
        { MessageBox.Show("Invalid world rows.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error); return; }

        float nLeft   = ref1.X / originalImage.Width;
        float nTop    = ref1.Y / originalImage.Height;
        float nRight  = ref2.X / originalImage.Width;
        float nBottom = ref2.Y / originalImage.Height;

        string gameName = (gameCombo.SelectedIndex >= 0 && gameCombo.SelectedIndex < GameNames.Length)
            ? GameNames[gameCombo.SelectedIndex] : "Custom";

        try
        {
            using (StreamWriter sw = new StreamWriter(outputPathBox.Text))
            {
                sw.WriteLine("# V2NES Player Marker Calibration");
                sw.WriteLine("# Generated by V2NES Companion — do not edit manually unless you know what you're doing");
                sw.WriteLine("game=" + gameName);
                sw.WriteLine("ram_addr=" + ramAddr);
                sw.WriteLine("world_cols=" + cols);
                sw.WriteLine("world_rows=" + rows);
                sw.WriteLine(string.Format(System.Globalization.CultureInfo.InvariantCulture,
                    "map_left={0:0.000000}", nLeft));
                sw.WriteLine(string.Format(System.Globalization.CultureInfo.InvariantCulture,
                    "map_top={0:0.000000}", nTop));
                sw.WriteLine(string.Format(System.Globalization.CultureInfo.InvariantCulture,
                    "map_right={0:0.000000}", nRight));
                sw.WriteLine(string.Format(System.Globalization.CultureInfo.InvariantCulture,
                    "map_bottom={0:0.000000}", nBottom));
                sw.WriteLine("img_width=" + originalImage.Width);
                sw.WriteLine("img_height=" + originalImage.Height);
            }
            parent.AddLog("Exported: " + Path.GetFileName(outputPathBox.Text), "success");
            MessageBox.Show("Calibration file saved!\n\n" + outputPathBox.Text +
                "\n\nPlace this .pos file next to your .v2m map file on the SD card.",
                "Saved", MessageBoxButtons.OK, MessageBoxIcon.Information);
        }
        catch (Exception ex)
        {
            MessageBox.Show("Failed to save: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            if (originalImage != null) originalImage.Dispose();
            if (displayImage  != null) displayImage.Dispose();
        }
        base.Dispose(disposing);
    }
}

public class VerticalProgressBar : ProgressBar
{
    protected override CreateParams CreateParams
    {
        get
        {
            CreateParams cp = base.CreateParams;
            cp.Style |= 0x04; // PBS_VERTICAL
            return cp;
        }
    }
}
