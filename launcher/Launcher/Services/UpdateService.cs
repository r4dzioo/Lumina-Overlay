using System;
using System.IO;
using System.IO.Compression;
using System.Net.Http;
using System.Security.Cryptography;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace Launcher.Services;

public sealed class UpdateService
{
    private readonly string _appDirectory;
    private readonly LauncherSettings _settings;
    private readonly HttpClient _http = new();

    public UpdateService(string appDirectory, LauncherSettings settings)
    {
        _appDirectory = appDirectory;
        _settings = settings;
        _http.DefaultRequestHeaders.UserAgent.ParseAdd("LuminaOverlayLauncher/1.0");
    }

    public async Task<Version> GetLocalVersionAsync()
    {
        string path = Path.Combine(_appDirectory, _settings.LocalVersionFile);
        if (!File.Exists(path))
        {
            return new Version(0, 0, 0);
        }

        string text = await File.ReadAllTextAsync(path);
        return Version.TryParse(text.Trim().TrimStart('v', 'V'), out var version)
            ? version
            : new Version(0, 0, 0);
    }

    public async Task<VersionManifest> GetLatestManifestAsync(CancellationToken cancellationToken = default)
    {
        using var response = await _http.GetAsync(_settings.ManifestUrl, cancellationToken);
        response.EnsureSuccessStatusCode();

        await using Stream stream = await response.Content.ReadAsStreamAsync(cancellationToken);
        var options = new JsonSerializerOptions { PropertyNameCaseInsensitive = true };
        VersionManifest? manifest = await JsonSerializer.DeserializeAsync<VersionManifest>(stream, options, cancellationToken);
        if (manifest is null || string.IsNullOrWhiteSpace(manifest.Download))
        {
            throw new InvalidOperationException("Remote latest.json is missing a download URL.");
        }

        return manifest;
    }

    public bool IsNewer(Version local, Version remote) => remote > local;

    public async Task DownloadAndInstallAsync(
        VersionManifest manifest,
        IProgress<UpdateProgress> progress,
        CancellationToken cancellationToken = default)
    {
        string tempRoot = Path.Combine(Path.GetTempPath(), "LuminaOverlay", Guid.NewGuid().ToString("N"));
        string zipPath = Path.Combine(tempRoot, "Overlay.zip");
        string stagingPath = Path.Combine(tempRoot, "staging");
        Directory.CreateDirectory(tempRoot);

        try
        {
            progress.Report(new UpdateProgress(10, "Downloading update package"));
            await DownloadFileAsync(manifest.Download, zipPath, progress, cancellationToken);

            if (!string.IsNullOrWhiteSpace(manifest.Sha256))
            {
                progress.Report(new UpdateProgress(72, "Validating package signature"));
                string actualHash = await ComputeSha256Async(zipPath, cancellationToken);
                if (!actualHash.Equals(manifest.Sha256, StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException("Downloaded update failed SHA-256 validation.");
                }
            }

            progress.Report(new UpdateProgress(80, "Extracting update"));
            ZipFile.ExtractToDirectory(zipPath, stagingPath, overwriteFiles: true);
            ValidateStaging(stagingPath);

            progress.Report(new UpdateProgress(88, "Applying files"));
            CopyDirectory(stagingPath, _appDirectory);

            string versionPath = Path.Combine(_appDirectory, _settings.LocalVersionFile);
            await File.WriteAllTextAsync(versionPath, manifest.Version, cancellationToken);

            progress.Report(new UpdateProgress(100, "Update complete"));
        }
        finally
        {
            TryDeleteDirectory(tempRoot);
        }
    }

    private async Task DownloadFileAsync(
        string url,
        string destination,
        IProgress<UpdateProgress> progress,
        CancellationToken cancellationToken)
    {
        using var response = await _http.GetAsync(url, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
        response.EnsureSuccessStatusCode();

        long? length = response.Content.Headers.ContentLength;
        await using Stream source = await response.Content.ReadAsStreamAsync(cancellationToken);
        await using FileStream target = File.Create(destination);

        byte[] buffer = new byte[1024 * 128];
        long totalRead = 0;
        int read;
        while ((read = await source.ReadAsync(buffer.AsMemory(0, buffer.Length), cancellationToken)) > 0)
        {
            await target.WriteAsync(buffer.AsMemory(0, read), cancellationToken);
            totalRead += read;

            if (length is > 0)
            {
                double percent = 10 + (totalRead / (double)length.Value) * 60;
                progress.Report(new UpdateProgress(percent, $"Downloaded {totalRead / 1024 / 1024} MB"));
            }
        }
    }

    private static async Task<string> ComputeSha256Async(string path, CancellationToken cancellationToken)
    {
        await using FileStream stream = File.OpenRead(path);
        byte[] hash = await SHA256.HashDataAsync(stream, cancellationToken);
        return Convert.ToHexString(hash).ToLowerInvariant();
    }

    private void ValidateStaging(string stagingPath)
    {
        string overlayPath = Path.Combine(stagingPath, _settings.OverlayExecutable);
        if (!File.Exists(overlayPath))
        {
            throw new InvalidOperationException($"Update package does not contain {_settings.OverlayExecutable}.");
        }
    }

    private static void CopyDirectory(string sourceDirectory, string targetDirectory)
    {
        Directory.CreateDirectory(targetDirectory);

        foreach (string directory in Directory.EnumerateDirectories(sourceDirectory, "*", SearchOption.AllDirectories))
        {
            string relative = Path.GetRelativePath(sourceDirectory, directory);
            Directory.CreateDirectory(Path.Combine(targetDirectory, relative));
        }

        foreach (string sourceFile in Directory.EnumerateFiles(sourceDirectory, "*", SearchOption.AllDirectories))
        {
            string relative = Path.GetRelativePath(sourceDirectory, sourceFile);
            if (relative.Equals("Launcher.exe", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            string targetFile = Path.Combine(targetDirectory, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(targetFile)!);
            File.Copy(sourceFile, targetFile, overwrite: true);
        }
    }

    private static void TryDeleteDirectory(string path)
    {
        try
        {
            if (Directory.Exists(path))
            {
                Directory.Delete(path, recursive: true);
            }
        }
        catch
        {
        }
    }
}
