<!-- markdownlint-configure-file {"MD024": { "siblings_only": true } } -->

# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Description.

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

[unreleased]: https://github.com/Tatsh/kate-wakatime/compare/v1.5.2...HEAD
