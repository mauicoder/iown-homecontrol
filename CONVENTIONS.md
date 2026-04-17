# Build Instructions
To build the project, use the command: `~/.platformio/penv/bin/pio run`. 
Always run this command after making C++ changes to verify that the code compiles.
Add tests when adding funtionalities
Adapt the tests for changed functionalities
if the tests are failing, add debugging log and then fix the tests with the new information

# Project Constraints
- Target Hardware: ESP32
- Framework: Arduino / PlatformIO
- Security: io-homecontrol AES-128
- Files to Ignore: Never modify anything in the `docs/` directory. Use them for reference only.

