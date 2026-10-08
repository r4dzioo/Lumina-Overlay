using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

namespace Launcher.Services;

public sealed class OverlayProcessSupervisor
{
    private readonly string _appDirectory;
    private readonly LauncherSettings _settings;
    private Process? _process;

    public OverlayProcessSupervisor(string appDirectory, LauncherSettings settings)
    {
        _appDirectory = appDirectory;
        _settings = settings;
    }

    public event EventHandler<string>? OverlayExited;

    public string DescribeInstall()
    {
        string overlayPath = Path.Combine(_appDirectory, _settings.OverlayExecutable);
        string versionPath = Path.Combine(_appDirectory, _settings.LocalVersionFile);
        string version = File.Exists(versionPath) ? File.ReadAllText(versionPath).Trim() : "unknown";

        if (!File.Exists(overlayPath))
        {
            return $"Overlay executable is missing: {overlayPath}";
        }

        var info = new FileInfo(overlayPath);
        return $"Overlay ready: {overlayPath}{Environment.NewLine}Version: {version}{Environment.NewLine}Size: {info.Length / 1024} KB";
    }

    public void Start()
    {
        if (_process is { HasExited: false })
        {
            return;
        }

        string path = Path.Combine(_appDirectory, _settings.OverlayExecutable);
        if (!File.Exists(path))
        {
            throw new FileNotFoundException($"Overlay executable was not found at {path}. Run update or reinstall Setup.exe.", path);
        }

        _process = new Process
        {
            StartInfo = new ProcessStartInfo
            {
                FileName = path,
                WorkingDirectory = _appDirectory,
                UseShellExecute = false
            },
            EnableRaisingEvents = true
        };

        _process.Exited += (_, _) =>
        {
            int exitCode = _process?.ExitCode ?? -1;
            string message = exitCode == 0
                ? "Overlay exited normally."
                : $"Overlay crashed or stopped unexpectedly. Exit code: {exitCode}.{ReadOverlayLogTail()}";
            AppendCrashLog(message);
            OverlayExited?.Invoke(this, message);

            if (_settings.RestartOnCrash && exitCode != 0)
            {
                Start();
            }
        };

        _process.Start();
    }

    public void StartUninstall()
    {
        string uninstaller = Directory.EnumerateFiles(_appDirectory, "unins*.exe")
            .OrderBy(path => path)
            .FirstOrDefault() ?? Path.Combine(_appDirectory, "unins000.exe");

        if (!File.Exists(uninstaller))
        {
            throw new FileNotFoundException($"Uninstaller was not found at {uninstaller}.", uninstaller);
        }

        Process.Start(new ProcessStartInfo
        {
            FileName = uninstaller,
            WorkingDirectory = _appDirectory,
            UseShellExecute = true
        });
    }

    public async Task StopAsync()
    {
        if (_process is not { HasExited: false })
        {
            return;
        }

        try
        {
            _process.CloseMainWindow();
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(3));
            await _process.WaitForExitAsync(timeout.Token);
        }
        catch
        {
            if (_process is { HasExited: false })
            {
                _process.Kill(entireProcessTree: true);
            }
        }
    }

    private void AppendCrashLog(string message)
    {
        string logDir = Path.Combine(_appDirectory, "logs");
        Directory.CreateDirectory(logDir);
        string line = $"[{DateTimeOffset.Now:O}] {message}{Environment.NewLine}";
        File.AppendAllText(Path.Combine(logDir, "launcher.log"), line);
    }

    private string ReadOverlayLogTail()
    {
        string path = Path.Combine(_appDirectory, "logs", "overlay.log");
        if (!File.Exists(path))
        {
            return string.Empty;
        }

        string[] lines = File.ReadLines(path).TakeLast(12).ToArray();
        return lines.Length == 0
            ? string.Empty
            : $"{Environment.NewLine}{string.Join(Environment.NewLine, lines)}";
    }
}
