using System.IO;
using System.Text.Json;

namespace Launcher.Services;

public sealed class LauncherSettings
{
    public string ManifestUrl { get; set; } = "https://github.com/r4dzioo/LuminaOverlay/releases/latest/download/latest.json";
    public string OverlayExecutable { get; set; } = "Overlay.exe";
    public string LocalVersionFile { get; set; } = "version.txt";
    public bool RestartOnCrash { get; set; }

    public static LauncherSettings Load(string appDirectory)
    {
        string path = Path.Combine(appDirectory, "appsettings.json");
        if (!File.Exists(path))
        {
            return new LauncherSettings();
        }

        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true
        };

        string json = File.ReadAllText(path);
        return JsonSerializer.Deserialize<LauncherSettings>(json, options) ?? new LauncherSettings();
    }
}
