<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[unreleased]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.3...HEAD
[1.5.3]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.2...v1.5.3
[1.5.2]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.1...v1.5.2
[1.5.1]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.0...v1.5.1
[1.5.0]: https://github.com/Tatsh/kate-wakatime/compare/v1.4.1...v1.5.0
