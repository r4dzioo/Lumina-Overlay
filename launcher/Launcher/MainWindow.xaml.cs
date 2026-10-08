using Launcher.Services;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Threading.Tasks;
using System.Windows;

namespace Launcher;

public partial class MainWindow : Window, INotifyPropertyChanged
{
    private readonly LauncherSettings _settings;
    private readonly UpdateService _updateService;
    private readonly OverlayProcessSupervisor _overlay;
    private VersionManifest? _remoteManifest;
    private bool _updateAvailable;
    private bool _busy;
    private string _statusText = "Starting";
    private string _versionText = "Local version unknown";
    private string _changelog = "Checking for release notes...";
    private string _overlayState = "Overlay is not running.";
    private string _primaryButtonText = "Launch overlay";
    private string _progressText = "Idle";
    private double _progress;

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
                OverlayState = message;
                StatusText = "Overlay stopped";
            });
        };

        Loaded += async (_, _) => await CheckForUpdatesAsync();
    }

    public event PropertyChangedEventHandler? PropertyChanged;

    public string StatusText { get => _statusText; set => SetField(ref _statusText, value); }
    public string VersionText { get => _versionText; set => SetField(ref _versionText, value); }
    public string Changelog { get => _changelog; set => SetField(ref _changelog, value); }
    public string OverlayState { get => _overlayState; set => SetField(ref _overlayState, value); }
    public string PrimaryButtonText { get => _primaryButtonText; set => SetField(ref _primaryButtonText, value); }
    public string ProgressText { get => _progressText; set => SetField(ref _progressText, value); }
    public double Progress { get => _progress; set => SetField(ref _progress, value); }
    public bool CanRunPrimaryAction => !_busy;

    private async void PrimaryButton_Click(object sender, RoutedEventArgs e)
    {
        if (_updateAvailable && _remoteManifest is not null)
        {
            await InstallUpdateAsync(_remoteManifest);
            return;
        }

        LaunchOverlay();
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

    private async Task CheckForUpdatesAsync()
    {
        await RunBusyAsync(async () =>
        {
            StatusText = "Checking updates";
            ProgressText = "Downloading latest.json";
            Progress = 8;

            Version local = await _updateService.GetLocalVersionAsync();
            _remoteManifest = await _updateService.GetLatestManifestAsync();
            _updateAvailable = _updateService.IsNewer(local, _remoteManifest.VersionValue);

            VersionText = $"Installed {local} | Latest {_remoteManifest.Version}";
            Changelog = _remoteManifest.ChangelogText;
            PrimaryButtonText = _updateAvailable ? "Install update" : "Launch overlay";
            StatusText = _updateAvailable ? "Update available" : "Up to date";
            ProgressText = "Ready";
            Progress = 100;
        });
    }

    private async Task InstallUpdateAsync(VersionManifest manifest)
    {
        await RunBusyAsync(async () =>
        {
            StatusText = "Updating";
            PrimaryButtonText = "Updating...";
            var progress = new Progress<UpdateProgress>(p =>
            {
                Progress = p.Percent;
                ProgressText = p.Message;
            });

            await _overlay.StopAsync();
            await _updateService.DownloadAndInstallAsync(manifest, progress);

            _updateAvailable = false;
            VersionText = $"Installed {manifest.Version} | Latest {manifest.Version}";
            PrimaryButtonText = "Launch overlay";
            StatusText = "Updated";
            ProgressText = "Update installed. Restarting launcher...";
            Progress = 100;

            RestartLauncher();
        });
    }

    private void LaunchOverlay()
    {
        try
        {
            OverlayState = _overlay.DescribeInstall();
            _overlay.Start();
            OverlayState = "Overlay is running. Use Insert/F10/F11 hotkeys in-game.";
            StatusText = "Overlay running";
        }
        catch (Exception ex)
        {
            OverlayState = ex.Message;
            StatusText = "Launch failed";
        }
    }

    private void RestartLauncher()
    {
        string launcherPath = Path.Combine(AppContext.BaseDirectory, "Launcher.exe");
        if (File.Exists(launcherPath))
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = launcherPath,
                WorkingDirectory = AppContext.BaseDirectory,
                UseShellExecute = true
            });
        }

        Application.Current.Shutdown();
    }

    private async Task RunBusyAsync(Func<Task> action)
    {
        if (_busy)
        {
            return;
        }

        _busy = true;
        OnPropertyChanged(nameof(CanRunPrimaryAction));
        try
        {
            await action();
        }
        catch (Exception ex)
        {
            StatusText = "Action failed";
            ProgressText = ex.Message;
        }
        finally
        {
            _busy = false;
            OnPropertyChanged(nameof(CanRunPrimaryAction));
        }
    }

    private void SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
        {
            return;
        }

        field = value;
        OnPropertyChanged(propertyName);
    }

    private void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }
}
