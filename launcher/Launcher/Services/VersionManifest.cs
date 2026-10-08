using System;
using System.Text.Json.Serialization;

namespace Launcher.Services;

public sealed class VersionManifest
{
    [JsonPropertyName("version")]
    public string Version { get; set; } = "0.0.0";

    [JsonPropertyName("download")]
    public string Download { get; set; } = string.Empty;

    [JsonPropertyName("sha256")]
    public string Sha256 { get; set; } = string.Empty;

    [JsonPropertyName("changelog")]
    public string Changelog { get; set; } = string.Empty;

    [JsonPropertyName("minLauncherVersion")]
    public string MinLauncherVersion { get; set; } = "1.0.0";

    [JsonIgnore]
    public System.Version VersionValue => System.Version.TryParse(Version.TrimStart('v', 'V'), out var value) ? value : new System.Version(0, 0, 0);

    [JsonIgnore]
    public string ChangelogText => string.IsNullOrWhiteSpace(Changelog) ? "No changelog was published for this release." : Changelog;
}
