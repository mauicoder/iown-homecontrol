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
        // Add other Serial methods (e.g., print, println) here if they are used
        // within #ifdef DEBUG_IOHOME blocks in non-Arduino files.
        // For the current IoHome.cpp, only Serial.printf is used in such blocks.
    };
    extern MockSerialClass Serial; // Declare a global mock Serial object
  #endif
#endif
