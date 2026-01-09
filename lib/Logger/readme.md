# Logger Library

This is a simple logger to have a standard way to print data

## Usage

Include the header in your source file:

```cpp
#include <Logger/Logger.h>
```

Before using the logger, make sure to initialize the Serial port in your `setup()` function:

```cpp
void setup() {
    Serial.begin(115200);
    // ...
}
```

### Log Examples

```cpp
// without class name
Logger::debug("Debug message");
Logger::info("Info message");
Logger::warn("Warning message");
Logger::error("Error message");

// with class/component name
Logger::info("Initialization completed", "MyClass");
Logger::error("Failed to open file", "ConfigManager");
```

### Output Format

```
[HH:MM:SS](LEVEL)::ClassName message
```

-   LEVEL: DEBUG, INFO, WARN, ERROR
