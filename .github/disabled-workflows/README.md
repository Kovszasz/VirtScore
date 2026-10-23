# Temporarily disabled GitHub Actions

These workflows are parked outside `.github/workflows` to conserve Actions quota.
Only `.github/workflows/windows_release.yml` is active. The files are preserved
so they can be restored later:

```sh
git mv .github/disabled-workflows/*.yml .github/workflows/
```

Restore reusable workflows together with their callers. The macOS/Linux build jobs also have explicit `if: false`
guards; remove those guards if restoring those platform builds.

Existing runs use the workflow definitions from their original commits. This
change prevents future triggers from this branch; it does not cancel old runs.
After merging, the default branch will also use this reduced workflow set.
