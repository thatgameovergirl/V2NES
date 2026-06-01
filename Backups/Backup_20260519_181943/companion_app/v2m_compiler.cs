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
        this.Text = "V2NES Companion v1.4.0";
        this.Size = new Size(500, 640); // Expanded height for footer
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

        // 6. Convert Button
        convertBtn = new Button();
        convertBtn.Text = "CONVERT & DEPLOY ASSETS";
        convertBtn.Size = new Size(elementWidth, 45);
        convertBtn.Location = new Point(20, 360);
        convertBtn.FlatStyle = FlatStyle.Flat;
        convertBtn.FlatAppearance.BorderSize = 0;
        convertBtn.BackColor = Color.FromArgb(0, 242, 255); // Cyan background
        convertBtn.ForeColor = Color.FromArgb(10, 10, 12); // Deep contrast text
        convertBtn.Font = new Font("Segoe UI", 10, FontStyle.Bold);
        convertBtn.Cursor = Cursors.Hand;
        convertBtn.Click += (s, e) => ConvertAssets();
        this.Controls.Add(convertBtn);

        // 7. Log Console Header Label
        Label logHeader = new Label();
        logHeader.Text = "LOG CONSOLE";
        logHeader.Font = new Font("Segoe UI", 8, FontStyle.Bold);
        logHeader.ForeColor = Color.FromArgb(136, 136, 136);
        logHeader.Location = new Point(20, 420);
        logHeader.Size = new Size(elementWidth, 15);
        this.Controls.Add(logHeader);

        // Log Text Box Console
        logConsole = new TextBox();
        logConsole.Multiline = true;
        logConsole.ReadOnly = true;
        logConsole.ScrollBars = ScrollBars.Vertical;
        logConsole.Size = new Size(elementWidth, 130);
        logConsole.Location = new Point(20, 440);
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
        footerLabel.Location = new Point(20, 578);
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
                bool mapOk = await System.Threading.Tasks.Task.Run(() => ProcessImageToV2M(maps[0], outPath, isPixelPerfect));
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

    internal bool ProcessImageToV2M(string inputPath, string outputPath, bool pixelPerfect)
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

                int activeW = (maxX - minX) + 1;
                int activeH = (maxY - minY) + 1;

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
                    bw.Write(1); // Format: 1 = RGB565

                    // Process each tile in the grid
                    for (int r = 0; r < gridRows; r++)
                    {
                        for (int c = 0; c < gridCols; c++)
                        {
                            int tileX = minX + c * 1024;
                            int tileY = minY + r * 1024;
                            int tileW = Math.Min(1024, activeW - c * 1024);
                            int tileH = Math.Min(1024, activeH - r * 1024);

                            bw.Write(tileW);
                            bw.Write(tileH);

                            using (Bitmap tileBmp = new Bitmap(1024, 1024))
                            {
                                using (Graphics g = Graphics.FromImage(tileBmp))
                                {
                                    g.Clear(Color.Black);
                                    g.InterpolationMode = InterpolationMode.NearestNeighbor;
                                    g.PixelOffsetMode = PixelOffsetMode.Half;
                                    // Draw the subregion of the cropped image onto the tile
                                    g.DrawImage(srcBmp, new Rectangle(0, 0, tileW, tileH), new Rectangle(tileX, tileY, tileW, tileH), GraphicsUnit.Pixel);
                                }

                                // Lock bits to convert to RGB565
                                var rect = new Rectangle(0, 0, 1024, 1024);
                                var data = tileBmp.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppRgb);
                                int bytesCount = data.Stride * data.Height;
                                byte[] rgbValues = new byte[bytesCount];
                                Marshal.Copy(data.Scan0, rgbValues, 0, bytesCount);
                                tileBmp.UnlockBits(data);

                                byte[] rgb565Data = new byte[1024 * 1024 * 2];
                                int idx565 = 0;
                                for (int i = 0; i < bytesCount; i += 4)
                                {
                                    byte blue = rgbValues[i];
                                    byte green = rgbValues[i + 1];
                                    byte red = rgbValues[i + 2];

                                    ushort pixel565 = (ushort)(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
                                    rgb565Data[idx565++] = (byte)(pixel565 & 0xFF);
                                    rgb565Data[idx565++] = (byte)((pixel565 >> 8) & 0xFF);
                                }

                                bw.Write(rgb565Data);
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

                bool success = await System.Threading.Tasks.Task.Run(() => parent.ProcessImageToV2M(paths[i], outPath, parent.pixelPerfectCheck.Checked));
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
