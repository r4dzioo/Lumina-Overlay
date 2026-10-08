using Launcher.Services;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media;

namespace Launcher;

/// <summary>
/// Converts a 0–100 Progress value into a pixel width for the custom progress bar.
/// </summary>
public sealed class ProgressWidthConverter : IMultiValueConverter
{
    public object Convert(object[] values, Type targetType, object parameter, CultureInfo culture)
    {
        if (values.Length < 2 || values[0] is not double progress || values[1] is not double totalWidth)
            return 0.0;
        return Math.Max(0.0, Math.Min(totalWidth, totalWidth * progress / 100.0));
    }

    public object[] ConvertBack(object value, Type[] targetTypes, object parameter, CultureInfo culture)
        => throw new NotSupportedException();
}

public partial class MainWindow : Window, INotifyPropertyChanged
{
    private readonly LauncherSettings _settings;
    private readonly UpdateService _updateService;
    private readonly OverlayProcessSupervisor _overlay;
    private VersionManifest? _remoteManifest;
    private bool _updateAvailable;
    private bool _busy;
    private bool _overlayRunning;

    // ── Observable properties ──────────────────────────────────────────────

    private string _statusText       = "Starting";
    private string _versionText      = "Local version unknown";
    private string _changelog        = "Checking for release notes...";
    private string _overlayState     = "Overlay is not running.";
    private string _primaryButtonText = "Launch Overlay";
    private string _progressText     = "Idle";
    private string _pageTitle        = "Ready to play";
    private double _progress;
    private double _opacityPercent   = 88;
    private bool   _rgbAccentEnabled = false;
    private bool   _widgetFpsEnabled      = true;
    private bool   _widgetFrametimeEnabled = false;
    private bool   _widgetCpuEnabled      = true;
    private bool   _widgetGpuEnabled      = true;
    private bool   _widgetRamEnabled      = true;
    private bool   _widgetNetworkEnabled  = true;

    public MainWindow()
    {
        InitializeComponent();
        DataContext = this;

        _settings = LauncherSettings.Load(AppContext.BaseDirectory);
        _updateService = new UpdateService(AppContext.BaseDirectory, _settings);
        _overlay = new OverlayProcessSupervisor(AppContext.BaseDirectory, _settings);
        OverlayState = _overlay.DescribeInstall();

        _overlay.OverlayExited += (_, message) =>
        {
            Dispatcher.Invoke(() =>
            {
                _overlayRunning = false;
                OverlayState = message;
                StatusText = "Overlay stopped";
                PageTitle = "Ready to play";
                OnPropertyChanged(nameof(CanStopOverlay));
                OnPropertyChanged(nameof(CanRunPrimaryAction));
                RefreshStatusColors();
            });
        };

        // Load config values for customization page
        LoadConfigValues();

        Loaded += async (_, _) => await CheckForUpdatesAsync();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    // ── Properties ────────────────────────────────────────────────────────

    public string StatusText        { get => _statusText;        set => SetField(ref _statusText, value); }
    public string VersionText       { get => _versionText;       set => SetField(ref _versionText, value); }
    public string Changelog         { get => _changelog;         set => SetField(ref _changelog, value); }
    public string OverlayState      { get => _overlayState;      set => SetField(ref _overlayState, value); }
    public string PrimaryButtonText { get => _primaryButtonText; set => SetField(ref _primaryButtonText, value); }
    public string ProgressText      { get => _progressText;      set => SetField(ref _progressText, value); }
    public string PageTitle         { get => _pageTitle;         set => SetField(ref _pageTitle, value); }
    public double Progress          { get => _progress;          set => SetField(ref _progress, value); }
    public double OpacityPercent    { get => _opacityPercent;    set => SetField(ref _opacityPercent, value); }
    public bool   RgbAccentEnabled  { get => _rgbAccentEnabled;  set => SetField(ref _rgbAccentEnabled, value); }
    public bool   WidgetFpsEnabled           { get => _widgetFpsEnabled;           set => SetField(ref _widgetFpsEnabled, value); }
    public bool   WidgetFrametimeEnabled     { get => _widgetFrametimeEnabled;     set => SetField(ref _widgetFrametimeEnabled, value); }
    public bool   WidgetCpuEnabled           { get => _widgetCpuEnabled;           set => SetField(ref _widgetCpuEnabled, value); }
    public bool   WidgetGpuEnabled           { get => _widgetGpuEnabled;           set => SetField(ref _widgetGpuEnabled, value); }
    public bool   WidgetRamEnabled           { get => _widgetRamEnabled;           set => SetField(ref _widgetRamEnabled, value); }
    public bool   WidgetNetworkEnabled       { get => _widgetNetworkEnabled;       set => SetField(ref _widgetNetworkEnabled, value); }
    public bool   CanRunPrimaryAction => !_busy;
    public bool   CanStopOverlay      => _overlayRunning && !_busy;

    // Status light / badge colours (bound from XAML)
    private Brush _statusBgColor     = new SolidColorBrush(Color.FromRgb(0x13, 0x1E, 0x2E));
    private Brush _statusBorderColor = new SolidColorBrush(Color.FromRgb(0x1E, 0x2E, 0x42));
    private Brush _statusDotColor    = new SolidColorBrush(Color.FromRgb(0x3D, 0x55, 0x70));
    private Brush _statusFgColor     = new SolidColorBrush(Color.FromRgb(0x7A, 0x96, 0xB2));
    public Brush StatusBgColor     { get => _statusBgColor;     set => SetField(ref _statusBgColor, value); }
    public Brush StatusBorderColor { get => _statusBorderColor; set => SetField(ref _statusBorderColor, value); }
    public Brush StatusDotColor    { get => _statusDotColor;    set => SetField(ref _statusDotColor, value); }
    public Brush StatusFgColor     { get => _statusFgColor;     set => SetField(ref _statusFgColor, value); }

    // ── Navigation ────────────────────────────────────────────────────────

    private void NavDashboard_Click(object sender, RoutedEventArgs e)
    {
        PageDashboard.Visibility = Visibility.Visible;
        PageCustomize.Visibility = Visibility.Collapsed;
        PageUpdates.Visibility   = Visibility.Collapsed;
    }

    private void NavCustomize_Click(object sender, RoutedEventArgs e)
    {
        LoadConfigValues();
        PageDashboard.Visibility = Visibility.Collapsed;
        PageCustomize.Visibility = Visibility.Visible;
        PageUpdates.Visibility   = Visibility.Collapsed;
    }

    private void NavUpdates_Click(object sender, RoutedEventArgs e)
    {
        PageDashboard.Visibility = Visibility.Collapsed;
        PageCustomize.Visibility = Visibility.Collapsed;
        PageUpdates.Visibility   = Visibility.Visible;
    }

    // ── Overlay lifecycle ─────────────────────────────────────────────────

    private async void PrimaryButton_Click(object sender, RoutedEventArgs e)
    {
        if (_updateAvailable && _remoteManifest is not null)
        {
            await InstallUpdateAsync(_remoteManifest);
            return;
        }
        LaunchOverlay();
    }

    private async void StopButton_Click(object sender, RoutedEventArgs e)
    {
        await RunBusyAsync(async () =>
        {
            StatusText = "Stopping overlay";
            await _overlay.StopAsync();
            _overlayRunning = false;
            OverlayState = "Overlay was stopped manually.";
            StatusText = "Overlay stopped";
            PageTitle = "Ready to play";
            OnPropertyChanged(nameof(CanStopOverlay));
            RefreshStatusColors();
        });
    }

    private async void CheckButton_Click(object sender, RoutedEventArgs e)
    {
        await CheckForUpdatesAsync();
    }

    private void UninstallButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            _overlay.StartUninstall();
            Application.Current.Shutdown();
        }
        catch (Exception ex)
        {
            OverlayState = ex.Message;
            StatusText = "Uninstall failed";
        }
    }

    // ── Customization handlers ────────────────────────────────────────────

    private void OpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
    {
        // Live preview — actual save on "Save Changes"
    }

    private void RgbAccent_Changed(object sender, RoutedEventArgs e) { /* deferred to Save */ }
    private void Widget_Changed(object sender, RoutedEventArgs e)     { /* deferred to Save */ }

    private void SaveCustomization_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            WriteConfigValues();
            OverlayState = "Settings saved. Restart overlay to apply changes.";
        }
        catch (Exception ex)
        {
            OverlayState = $"Failed to save settings: {ex.Message}";
        }
    }

    private void ResetDefaults_Click(object sender, RoutedEventArgs e)
    {
        OpacityPercent         = 88;
        RgbAccentEnabled       = false;
        WidgetFpsEnabled       = true;
        WidgetFrametimeEnabled = false;
        WidgetCpuEnabled       = true;
        WidgetGpuEnabled       = true;
        WidgetRamEnabled       = true;
        WidgetNetworkEnabled   = true;
        SaveCustomization_Click(sender, e);
    }

    // ── Config.json read / write ──────────────────────────────────────────

    private string ConfigPath => Path.Combine(AppContext.BaseDirectory, "assets", "config.json");

    private void LoadConfigValues()
    {
        try
        {
            if (!File.Exists(ConfigPath)) return;
            var root = JsonNode.Parse(File.ReadAllText(ConfigPath));
            if (root is null) return;

            // Read from first profile
            var profiles = root["profiles"]?.AsArray();
            if (profiles is null || profiles.Count == 0) return;
            var profile = profiles[0]!;

            double opacity = profile["opacity"]?.GetValue<double>() ?? 0.88;
            OpacityPercent   = Math.Round(opacity * 100.0);
            RgbAccentEnabled = profile["rgbAccent"]?.GetValue<bool>() ?? false;

            var widgets = profile["widgets"]?.AsArray();
            if (widgets is null) return;
            foreach (var w in widgets)
            {
                string id      = w?["id"]?.GetValue<string>() ?? "";
                bool   enabled = w?["enabled"]?.GetValue<bool>() ?? true;
                switch (id)
                {
                    case "fps":       WidgetFpsEnabled       = enabled; break;
                    case "frametime": WidgetFrametimeEnabled = enabled; break;
                    case "cpu":       WidgetCpuEnabled       = enabled; break;
                    case "gpu":       WidgetGpuEnabled       = enabled; break;
                    case "ram":       WidgetRamEnabled       = enabled; break;
                    case "network":   WidgetNetworkEnabled   = enabled; break;
                }
            }
        }
        catch { /* silently ignore parse errors */ }
    }

    private void WriteConfigValues()
    {
        if (!File.Exists(ConfigPath)) return;

        var root = JsonNode.Parse(File.ReadAllText(ConfigPath))?.AsObject()
                   ?? throw new InvalidOperationException("config.json is not a JSON object.");

        var profiles = root["profiles"]?.AsArray();
        if (profiles is null || profiles.Count == 0) return;
        var profile = profiles[0]!.AsObject();

        profile["opacity"]   = Math.Round(OpacityPercent / 100.0, 2);
        profile["rgbAccent"] = RgbAccentEnabled;

        var widgets = profile["widgets"]?.AsArray();
        if (widgets is not null)
        {
            var enableMap = new Dictionary<string, bool>
            {
                ["fps"]       = WidgetFpsEnabled,
                ["frametime"] = WidgetFrametimeEnabled,
                ["cpu"]       = WidgetCpuEnabled,
                ["gpu"]       = WidgetGpuEnabled,
                ["ram"]       = WidgetRamEnabled,
                ["network"]   = WidgetNetworkEnabled,
            };

            foreach (var w in widgets)
            {
                string id = w?["id"]?.GetValue<string>() ?? "";
                if (enableMap.TryGetValue(id, out bool enabled))
                {
                    w!.AsObject()["enabled"] = enabled;
                }
            }
        }

        File.WriteAllText(ConfigPath, root.ToJsonString(new JsonSerializerOptions { WriteIndented = true }));
    }

    // ── Update / install ──────────────────────────────────────────────────

    private async Task CheckForUpdatesAsync()
    {
        await RunBusyAsync(async () =>
        {
            StatusText  = "Checking updates";
            ProgressText = "Downloading latest.json";
            Progress = 8;

            Version local = await _updateService.GetLocalVersionAsync();
            _remoteManifest = await _updateService.GetLatestManifestAsync();
            _updateAvailable = _updateService.IsNewer(local, _remoteManifest.VersionValue);

            VersionText       = $"Installed {local}  ·  Latest {_remoteManifest.Version}";
            Changelog         = _remoteManifest.ChangelogText;
            PrimaryButtonText = _updateAvailable ? "Install Update" : "Launch Overlay";
            StatusText        = _updateAvailable ? "Update available" : "Up to date";
            ProgressText      = "Ready";
            Progress          = 100;
            RefreshStatusColors();
        });
    }

    private async Task InstallUpdateAsync(VersionManifest manifest)
    {
        await RunBusyAsync(async () =>
        {
            StatusText        = "Updating";
            PrimaryButtonText = "Updating...";
            var progress = new Progress<UpdateProgress>(p =>
            {
                Progress     = p.Percent;
                ProgressText = p.Message;
            });

            await _overlay.StopAsync();
            await _updateService.DownloadAndInstallAsync(manifest, progress);

            _updateAvailable  = false;
            VersionText       = $"Installed {manifest.Version}  ·  Latest {manifest.Version}";
            PrimaryButtonText = "Launch Overlay";
            StatusText        = "Updated";
            ProgressText      = "Update installed. Restarting launcher...";
            Progress          = 100;
            RefreshStatusColors();
            RestartLauncher();
        });
    }

    private void LaunchOverlay()
    {
        try
        {
            OverlayState = _overlay.DescribeInstall();
            _overlay.Start();
            _overlayRunning = true;
            OverlayState = "Overlay is running.  Use Insert / F10 / F11 hotkeys in-game.";
            StatusText   = "Overlay running";
            PageTitle    = "Running";
            OnPropertyChanged(nameof(CanStopOverlay));
            RefreshStatusColors();
        }
        catch (Exception ex)
        {
            OverlayState = ex.Message;
            StatusText   = "Launch failed";
            RefreshStatusColors();
        }
    }

    private void RefreshStatusColors()
    {
        bool running = _overlayRunning;
        StatusBgColor     = running
            ? new SolidColorBrush(Color.FromRgb(0x0D, 0x26, 0x1A))
            : new SolidColorBrush(Color.FromRgb(0x13, 0x1E, 0x2E));
        StatusBorderColor = running
            ? new SolidColorBrush(Color.FromRgb(0x18, 0x5C, 0x3A))
            : new SolidColorBrush(Color.FromRgb(0x1E, 0x2E, 0x42));
        StatusDotColor    = running
            ? new SolidColorBrush(Color.FromRgb(0x22, 0xD9, 0x7E))
            : new SolidColorBrush(Color.FromRgb(0x3D, 0x55, 0x70));
        StatusFgColor     = running
            ? new SolidColorBrush(Color.FromRgb(0x22, 0xD9, 0x7E))
            : new SolidColorBrush(Color.FromRgb(0x7A, 0x96, 0xB2));
        StatusText        = running ? "Running" : StatusText;
    }

    private void RestartLauncher()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "Launcher.exe");
        if (File.Exists(path))
        {
            Process.Start(new ProcessStartInfo
            {
                FileName         = path,
                WorkingDirectory = AppContext.BaseDirectory,
                UseShellExecute  = true
            });
        }
        Application.Current.Shutdown();
    }

    private async Task RunBusyAsync(Func<Task> action)
    {
        if (_busy) return;
        _busy = true;
        OnPropertyChanged(nameof(CanRunPrimaryAction));
        OnPropertyChanged(nameof(CanStopOverlay));
        try
        {
            await action();
        }
        catch (Exception ex)
        {
            StatusText   = "Action failed";
            ProgressText = ex.Message;
        }
        finally
        {
            _busy = false;
            OnPropertyChanged(nameof(CanRunPrimaryAction));
            OnPropertyChanged(nameof(CanStopOverlay));
        }
    }

    private void SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return;
        field = value;
        OnPropertyChanged(propertyName);
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
}
