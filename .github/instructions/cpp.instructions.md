---
applyTo: '**/*.cpp, **/*.h'
---

# C++/Qt guidelines

- Follow [KDE C++ coding style](https://community.kde.org/Policies/Frameworks_Coding_Style), with
  the changes following this point.
- Use `#pragma once` in all header files instead of include guards.
- Use Google-style naming for members: `memberVariable_`.
- Use `nullptr` instead of `NULL`.
- Feel free to use `auto`. Add qualifiers like `const`, `*`, and `&` as appropriate to improve
  clarity.
- Use `QString`, `QList`, etc., instead of STL types for Qt/KDE APIs unless interoperability is
  required.
- All UI strings must be wrapped in the appropriate translation macro (e.g., `i18n()`, `tr`).
- Do not use C-style casts; use C++ casts (`static_cast`, `dynamic_cast`, etc.).
- All new classes must have a corresponding header and source file, and must be added to the
  relevant `CMakeLists.txt`.
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers unless required by
  Qt/KDE APIs.
- Document all public classes and methods using Doxygen-style comments.
- Private methods and members should have brief comments if their purpose is not obvious.
- All new logic must be covered by unit tests in `autotests`.
- `QT_NO_CAST_FROM_ASCII` and similar flags are always enabled, so use `QStringLiteral()` where
  necessary.
- To convert from `char *` to `QString`, use `QString::fromUtf8()`.
- Prefer to use `.constData()` instead of `.data()` if both methods are available.
- `QT_NO_SIGNALS_SLOTS_KEYWORDS` is always enabled, so do not use `signals:` or `slots:` keywords.
- All public signals should be under the `Q_SIGNALS:` macro.
- Use `QSpan` or `std::span` instead of raw pointers and sizes for array parameters.
