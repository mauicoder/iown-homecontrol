#if defined(DEBUG_IOHOME)
  #if defined(ARDUINO)
    #include <Arduino.h>
  #else
    #include <cstdio>  // For std::printf
    #include <cstdarg> // For va_list, va_start, va_end

    // Mock Serial class/object for non-Arduino environments
    class MockSerialClass {
    public:
        void printf(const char* format, ...) {
            va_list args;
            va_start(args, format);
            std::vprintf(format, args);
            va_end(args);
        }

        // Basic println for string literals or values
        void println(const char* msg = "") {
            printf("%s\n", msg);
        }

        // Overload for Serial.println() with integer/long, etc.
        template<typename T>
        void println(T value) {
            printf("%lld\n", (long long)value); // Cast to long long for generic integral printing
        }

        // Add other Serial methods (e.g., print) here if they are used
        // within #ifdef DEBUG_IOHOME blocks in non-Arduino files.
    };
    extern MockSerialClass Serial; // Declare a global mock Serial object
  #endif
#endif
