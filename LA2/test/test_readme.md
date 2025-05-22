# T-Watch Test Suite

This test suite provides comprehensive testing for the T-Watch step counter project using the Unity testing framework.

## Test Structure

The test suite is contained in a **single combined file** due to PlatformIO's Unity integration limitations:

### `test_combined.cpp` - Complete Test Suite
- **Storage Tests**: Test SPIFFS file operations, WiFi credentials, username, and step history
- **JSON Tests**: Test serialisation/deserialisation of step data
- **Logic Tests**: Test step counting, progress calculation, and formatting
- **Hardware Tests**: Test watch, display, sensor, and power management initialisation
- **Display Tests**: Test screen drawing, colors, text, and shapes
- **Interface Tests**: Test touch detection and WiFi hardware

*Note: We use a single file because PlatformIO compiles all test files together, causing multiple definition errors when split across files.*

## Running Tests

### Prerequisites
1. T-Watch 2020 V3 connected via USB
2. PlatformIO installed
3. Unity testing framework (already configured in `platformio.ini`)

### To run all tests:
```bash
pio test -e ttgo-t-watch-test
```

### Test Output
The tests run in this order:
1. **Storage and Logic Tests** - File operations, JSON handling, calculations
2. **Hardware Tests** - Display initialisation with visual feedback on screen
3. **Interface Tests** - Touch, WiFi, and sensor verification

## Test Coverage Analysis

### ✅ **Excellent Coverage (90-100%)**
- **Hardware Components**: Display, sensors, power management, RTC, touch interface
- **Data Storage**: SPIFFS operations, credential persistence, step history
- **Core Logic**: Step counting, formatting, progress calculations, date/time handling  
- **JSON Operations**: Serialisation/deserialisation for Firebase data
- **Basic Drawing**: Colours, text rendering, shape drawing

### 🟡 **Partial Coverage (50-70%)**
- **Touch System**: Tests interface exists, but not gesture detection algorithms
- **WiFi**: Tests hardware scanning, but not connection/Firebase integration
- **Screen System**: Tests constants and enums, but not actual screen navigation

### ❌ **Missing Coverage (Areas for Manual Testing)**
- **Screen Navigation**: Swipe detection logic, screen transitions, user workflows
- **Firebase Integration**: HTTP requests, data synchronisation, error handling
- **Web Server**: AP mode, HTML generation, WiFi configuration interface
- **Error Scenarios**: WiFi disconnection, low battery, sensor failures, day rollover
- **End-to-End Workflows**: Complete user journeys across multiple screens

### **Overall Coverage Estimate: ~65-70%**

## Test Organisation

### Storage and Logic Tests (13 tests)
- ✅ SPIFFS initialisation and file operations
- ✅ WiFi credential save/load/clear
- ✅ Username persistence
- ✅ Step history JSON storage
- ✅ Screen and gesture enum verification
- ✅ Step count formatting (with commas)
- ✅ Progress calculation and capping
- ✅ Date/time string formatting
- ✅ JSON serialisation/deserialisation
- ✅ Constants verification

### Hardware Tests (11 tests)
- ✅ T-Watch initialisation
- ✅ Display setup and basic drawing
- ✅ Backlight control and brightness
- ✅ Accelerometer configuration
- ✅ Step counter functionality
- ✅ Power management and battery reading
- ✅ RTC functionality and time validation
- ✅ Touch interface accessibility
- ✅ WiFi hardware and network scanning
- ✅ Colour display with visual feedback
- ✅ Text rendering in multiple sizes
- ✅ Shape drawing (rectangles, circles, lines)

## Test Results Interpretation

### Success Indicators
- All tests pass with `✓ PASS`
- Visual display of colors and shapes on T-Watch screen
- Hardware components initialise without errors
- Data persists correctly in SPIFFS
- Sensor readings within expected ranges

### Common Issues and Solutions

#### SPIFFS Issues
```
FAILED: SPIFFS initialization
```
**Solution**: Ensure SPIFFS partition is properly configured. Try `pio run -t erase` then re-upload.

#### Display Issues
```
FAILED: Display initialization
```
**Solution**: Check physical connections, ensure USB provides adequate power, verify T-Watch variant matches config.

#### Sensor Issues
```
FAILED: Step counter functionality
```
**Solution**: Verify BMA423 sensor connections, ensure proper configuration flags in `config.h`.

#### WiFi Issues
```
FAILED: WiFi hardware test
```
**Solution**: Check antenna connections, ensure WiFi is enabled in board configuration.

## Extending Tests

### Adding New Logic Tests
```cpp
void test_your_new_feature() {
    // Test setup
    int expected = 42;
    int actual = yourFunction();
    
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected, actual, "Your feature should work correctly");
}

// Add to setup() function:
RUN_TEST(test_your_new_feature);
```

### Adding New Hardware Tests
```cpp
void test_new_hardware_component() {
    // Initialize hardware if needed
    if (testWatch == nullptr) {
        test_watch_initialization();
    }
    
    // Test hardware functionality
    bool result = testWatch->newComponent->initialize();
    TEST_ASSERT_TRUE_MESSAGE(result, "New component should initialize");
}
```

### Test Configuration Options
You can modify the combined file to enable/disable test categories:

```cpp
// At top of test file:
#define RUN_STORAGE_TESTS true
#define RUN_HARDWARE_TESTS true
#define RUN_DISPLAY_TESTS false  // Skip visual tests if needed
```

## Manual Testing Checklist

Since automated tests don't cover everything, complement with manual testing:

### Navigation Testing
- [ ] Swipe left from home → step counter
- [ ] Swipe right from home → calendar  
- [ ] Swipe down from any screen → settings
- [ ] Swipe up from home → leaderboard
- [ ] All screen transitions work smoothly

### WiFi and Firebase Testing
- [ ] Can enter AP mode from settings
- [ ] Can connect to WiFi through web interface
- [ ] Step count syncs to Firebase
- [ ] Leaderboard loads and displays correctly
- [ ] Handles network disconnection gracefully

### Error Scenario Testing
- [ ] Behavior when WiFi is lost
- [ ] Display when battery is low
- [ ] Step counter reset at midnight
- [ ] Recovery from sensor errors

### Data Persistence Testing
- [ ] Settings survive power cycle
- [ ] Step history maintained across days
- [ ] WiFi credentials persist after restart

## Performance Notes

The test suite includes basic performance verification:
- Display operations complete in reasonable time
- File I/O operations succeed without timeout
- Sensor readings are stable and consistent
- Memory usage remains within acceptable bounds

## Safety Notes

⚠️ **Important Safety Information**: 
- Tests modify SPIFFS storage - backup important data first
- Hardware tests control display brightness and colours
- Some tests require the watch to be plugged in for stable power
- Visual feedback tests will show patterns on the display
- Tests clean up temporary files but preserve actual project data

## Troubleshooting

### Serial Monitor Issues
If you can't see test output:
1. Ensure correct baud rate (115200)
2. Check USB cable and driver installation
3. Verify correct COM port selection in PlatformIO

### Memory Issues
If tests fail with out-of-memory errors:
1. Check for memory leaks in main project code
2. Verify SPIFFS partition has adequate space
3. Monitor heap usage during test execution

### Hardware Initialisation Failures
If hardware tests consistently fail:
1. Verify T-Watch variant matches `config.h` settings
2. Check power supply stability (use quality USB cable)
3. Ensure all required libraries are properly installed
4. Try power cycling the device

## Continuous Integration

For automated testing in CI/CD environments:

```yaml
# Example GitHub Actions workflow
- name: Run T-Watch Tests
  run: |
    pio test -e ttgo-t-watch-test --verbose
  # Note: Hardware tests will fail in CI without physical device
```

Consider separating logic tests from hardware tests for CI environments where physical hardware isn't available.