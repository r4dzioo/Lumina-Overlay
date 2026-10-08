# Operations

## Logs

- Launcher log: `logs/launcher.log`
- Overlay log: `logs/overlay.log`
- Overlay crash dump: `logs/OverlayCrash-YYYYMMDD-HHMMSS.dmp`

## Release Assets

Every GitHub Release should include:

- `Setup.exe`
- `Overlay.zip`
- `latest.json`
- `changelog.txt`

## Rollback

The current launcher applies updates after ZIP validation and staging validation. A production rollback layer should:

1. Copy the current install to `backup/previous`.
2. Apply the update.
3. Launch overlay health check.
4. Restore backup if launch fails.

## Versioning

Use semantic versions and tags:

```powershell
git tag v1.0.0
git push origin v1.0.0
```

The release workflow derives the package version from the tag.

