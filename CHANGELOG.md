<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.5.4] - 2026-05-07

### Changed

- General maintenance release. No user-visible behaviour changes.
- CI: build workflows now upload each install prefix as a downloadable artefact, and a Release
  workflow plus an MSYS2 PKGBUILD-publishing workflow have been added so tagged releases can flow
  through the GitHub Actions UI.
- Project hygiene: `yarn format` no longer fails on an unsupported `clang-format` flag, the unused
  `cmake-format` configuration has been removed, dependencies have been refreshed via Dependabot,
  and the project metadata generator has been kept current.

## [1.5.3] - 2025-09-14

### Changed

- The plugin now searches for several different binaries depending on OS and CPU architecture.
- Plugin will now function on Windows (tested on MSYS2's Kate).

## [1.5.2] - 2025-08-05

### Added

- Plugin description (appears in the Settings dialog under Plugins).

### Changed

- API URL in settings may be left blank or unset. Default will then be used.

### Fixed

- Duplicate call to `disconnect()`.
- Project name detection.

## [1.5.1] - 2025-06-26

Minor release.

## [1.5.0] - 2025-06-26

### Changed

- Use `wakatime-cli` or `wakatime` binary on system instead of directly calling API. Closes #21,
  #30, #31, #32.

### Fixed

- Configuration dialog was not able to be opened on Welcome page. Closes #34.

## [1.4.1] - 2025-06-03

### Changed

- Install plugin via `kcoreaddons` so it is picked up by Kate 6 properly. Closes #36.

### Fixed

- Validation of WakaTime API URL and API key. Closes #37.

## [1.4.0] - 2024-06-16

### Added

- CodeQL workflow.
- README dependencies section.

### Changed

- Ported plugin to KDE Frameworks 6 (Kate 6).

## [1.3.10] - 2022-09-16

### Fixed

- Include path so the plugin builds against current KTextEditor headers. Closes #23.

### Changed

- Removed `.vscode/c_cpp_properties.json`; tightened immutability of internal data; refreshed
  VS Code workspace settings.

## [1.3.9] - 2021-06-25

### Fixed

- Bulk-only API path: status code 202 is now accepted as success so the offline queue actually
  pops on a successful flush.

### Changed

- Sorted `.clang-format` keys.

## [1.3.8] - 2021-06-25

### Changed

- User agent string now matches what Wakapi expects.
- `X-Hostname` header replaced with `X-Machine-Name` per upstream API change.
- Replaced `QNetworkAccessManager::networkAccessible()` (deprecated in newer Qt) with a
  constructor-time availability check.
- `QLatin1String` literals migrated to `QStringLiteral`; `QByteArrayLiteral` used for short byte
  strings.

### Fixed

- Several smaller bugs uncovered while refactoring.

## [1.3.7] - 2021-06-21

### Changed

- Switched to using only the bulk heartbeats API endpoint.
- Replaced `NDEBUG` checks with `QT_DEBUG`.

## [1.3.6] - 2021-06-21

### Changed

- Updated heartbeat URL for the non-bulk API endpoint.
- Minor case adjustments in the configuration dialog.

## [1.3.5] - 2021-06-15

### Added

- Support for a configurable `api_url`. Closes #17.

### Changed

- Refreshed VS Code include path and workspace settings.

## [1.3.4] - 2019-10-21

### Changed

- Offline queue now uses `QSqlDatabase::transaction()` instead of an explicit
  `BEGIN IMMEDIATE` statement.
- Header includes made consistent across translation units.
- Plugin description added.

### Fixed

- Comparing the last file sent.
- Debug builds no longer fail.

## [1.3.3] - 2019-03-05

### Changed

- Migrated `foreach` loops to range-based `for`.
- Removed an unnecessary helper method.

### Fixed

- Memory leak in heartbeat construction.
- Crash when `getenv("PATH")` returns `NULL`.

## [1.3.2] - 2019-03-04

### Changed

- Use `QLatin1String()` where possible to reduce binary size.

### Fixed

- Reported version number.

## [1.3.1] - 2019-03-02

### Added

- Notify the user when a heartbeat request fails to authenticate. Closes #6.

## [1.3.0] - 2019-03-02

### Changed

- `QSettings` instance is now shared across the plugin rather than reconstructed per call.

## [1.2.0] - 2019-03-01

### Added

- Offline queue: heartbeats accumulated while offline are now sent once connectivity returns,
  matching upstream WakaTime client behaviour. Closes #4.

### Changed

- `QStringLiteral` adopted across the plugin where applicable.
- Sending a heartbeat extracted into its own method for clarity.

## [1.1.0] - 2019-02-19

### Added

- Configuration dialog. Closes #5.
- Configuration menu item.
- `hidefilenames` option support. Closes #1.
- `entity`, `category`, `lines`, `lineno`, and `cursorpos` heartbeat fields.
- Branch-name detection via Git. Closes #3.
- Plugin icon.

### Changed

- Several internal strings made `static` to avoid repeated allocation.

### Fixed

- `.git`-directory detection.

## [1.0] - 2017-01-02

### Changed

- Ported plugin to Kate 5.

### Fixed

- API key validation in `readConfig`.
- `getUserAgent()` signature; `trim()` length handling.

## [0.6] - 2014-11-16

### Added

- Initial public release as a Kate 4 plugin.
- `TimeZone` HTTP header on heartbeats.
- Status code in error log output.
- MacPorts installation instructions.

### Changed

- User-agent string aligned with other WakaTime plugins.
- Time units in log messages clarified (milliseconds versus seconds).

### Fixed

- Project-name detection when the source file is in the same directory as `.git` or `.svn`.
- Signals connected to document objects only once.

[unreleased]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.4...HEAD
[1.5.4]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.3...v1.5.4
[1.5.3]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.2...v1.5.3
[1.5.2]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.1...v1.5.2
[1.5.1]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/Tatsh/kate-wakatime/compare/v1.4.1...v1.5.0
[1.4.1]: https://github.com/Tatsh/kate-wakatime/compare/v1.4.0...v1.4.1
[1.4.0]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.10...v1.4.0
[1.3.10]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.9...v1.3.10
[1.3.9]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.8...v1.3.9
[1.3.8]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.7...v1.3.8
[1.3.7]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.6...v1.3.7
[1.3.6]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.5...v1.3.6
[1.3.5]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.4...v1.3.5
[1.3.4]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.3...v1.3.4
[1.3.3]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.2...v1.3.3
[1.3.2]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.1...v1.3.2
[1.3.1]: https://github.com/Tatsh/kate-wakatime/compare/v1.3.0...v1.3.1
[1.3.0]: https://github.com/Tatsh/kate-wakatime/compare/v1.2.0...v1.3.0
[1.2.0]: https://github.com/Tatsh/kate-wakatime/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/Tatsh/kate-wakatime/compare/v1.0...v1.1.0
[1.0]: https://github.com/Tatsh/kate-wakatime/compare/v0.6...v1.0
[0.6]: https://github.com/Tatsh/kate-wakatime/releases/tag/v0.6
