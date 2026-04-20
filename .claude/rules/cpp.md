# C++/Qt guidelines

- No tabs.
- 4 spaces instead of a single tab.
- Each variable should be declared on a new line.
- Each new word in a variable name starts with a capital letter (so-called camelCase).
- Avoid abbreviations.
- Use indicative/useful names. No short names, except:
  - Single character variable names can denote counters and temporary variables whose purpose is
    obvious.
- Variables and functions start with a lowercase letter.
- Use blank lines to group statements.
- Use only one empty line.
- Use one space after each keyword.
  - Exception: No space between return and `;`.
- No space after left parentheses/before right parentheses.
- Single spaces around binary arithmetic operators `+`, `-`, `*`, `/`, `%`.
- Use `#pragma once` in all header files instead of include guards.
- Use Google-style naming for private class members: `memberVariable_`.
- Use `kConstantName`-style constant names.
- Use `nullptr` instead of `NULL`.
- Feel free to use `auto`. Add qualifiers like `const`, `*`, and `&` as appropriate to improve
  clarity.
- Use `QString`, `QList`, etc., instead of STL types for Qt APIs unless interoperability is
  required.
- Do not use C-style casts; use C++ casts (`static_cast`, `dynamic_cast`, etc.).
- All new classes must have a corresponding header and source file, and must be added to the
  relevant `CMakeLists.txt`.
- Prefer smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers unless required by
  Qt APIs.
- Document all public classes and methods using Doxygen-style comments.
- Private methods and members should have brief comments if their purpose is not obvious.
- `QT_NO_CAST_FROM_ASCII` and similar flags are always enabled, so use `QStringLiteral()` where
  necessary.
- To convert from `char *` to `QString`, use `QString::fromUtf8()`.
- Prefer to use `.constData()` instead of `.data()` if both methods are available.
- `QT_NO_SIGNALS_SLOTS_KEYWORDS` is always enabled, so do not use `signals:` or `slots:` keywords.
- All public signals should be under the `Q_SIGNALS:` macro.
- Use `QSpan` or `std::span` instead of raw pointers and sizes for array parameters.
- Always use curly braces/brackets for logic control statements (`if`, `while`, `for`) even if there
  is only 1 statement.
- Left curly braces/brackets go on the same line as the start of the statement.
- Use `clang-format` regularly to format the file after ensuring syntax is correct.
- Includes should be grouped with a single blank line between the groups, in this order:
  - System headers like `<vector>`
  - `windows.h`
  - Other Windows headers that must come after `windows.h`
  - 3rd party headers like Qt
  - Project includes
- Include groups are always sorted alphabetically.
