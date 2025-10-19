// ThrottleInputManager deprecated after migration to IInputDevice-based rotary/pot devices.
#pragma once
class ThrottleInputManager { public: void begin(){} void loop(){} void notifySpeedExternallySet(int){} };
