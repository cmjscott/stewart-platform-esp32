/**
 *
 * Stewart Platform on ESP32
 *
 * Kinematics from xoxota99: https://github.com/xoxota99/stewy
 *
 * ouilogique.com
 * March 2019
 *
 */

#include <Arduino.h>
#include <P19.h>
#include <HexapodKinematics.h>
#include <ESP32Servo.h>
#include <ouilogique_Joystick.h>

#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

// Joystick
#define X_PIN 26
#define Y_PIN 12
#define Z_PIN 32
ouilogique_Joystick joystick(X_PIN, Y_PIN, Z_PIN);

// Stewart Platform
HexapodKinematics hk; // Stewart platform object.
Servo servos[6];      // servo objects.
float sp_servo[6];    // servo setpoints in degrees, between SERVO_MIN_ANGLE and SERVO_MAX_ANGLE.

float _toUs(int value)
{
    return map(value, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE, SERVO_MIN_US, SERVO_MAX_US);
}

/**
 * Set servo values to the angles represented by the setpoints in sp_servo[].
 * DOES: Apply trim values.
 * DOES: Automatically reverse signal for reversed servos.
 * DOES: Write signals to the physical servos.
 */
void updateServos()
{
    static float sValues[6];

    for (int i = 0; i < 6; i++)
    {
        // sp_servo holds a value between SERVO_MIN_ANGLE and SERVO_MAX_ANGLE.
        // apply reverse.
        float val = sp_servo[i];
        if (SERVO_REVERSE[i])
        {
            val = SERVO_MIN_ANGLE + (SERVO_MAX_ANGLE - val);
        }

        //translate angle to pulse width
        val = _toUs(val);

        if (val != sValues[i])
        {
            //don't write to the servo if you don't have to.
            sValues[i] = val;
            Serial.printf("SRV: s%d = %.2f + %d (value + trim)\n", i, val, SERVO_TRIM[i]);
            servos[i].writeMicroseconds((int)constrain(val + SERVO_TRIM[i], SERVO_MIN_US, SERVO_MAX_US));
        }
    }
}

/**
 * Calculates and assigns values to sp_servo.
 * DOES: Ignore out-of-range values. These will generate a warning on the serial monitor.
 * DOES NOT: Apply servo trim values.
 * DOES NOT: Automatically reverse signal for reversed servos.
 * DOES NOT: digitally write a signal to any servo. Writing is done in updateServos();
 */
void setServo(int i, int angle)
{
    int val = angle;
    if (val >= SERVO_MIN_ANGLE && val <= SERVO_MAX_ANGLE)
    {
        sp_servo[i] = val;
        Serial.printf("setServo %d - %.2f degrees\n", i, sp_servo[i]);
    }
    else
    {
        Serial.printf("setServo: Invalid value '%d' specified for servo #%d. Valid range is %d to %d degrees.\n", val, i, SERVO_MIN_ANGLE, SERVO_MAX_ANGLE);
    }
}

/**
 *
 */
void setupServos()
{
    for (int i = 0; i < 6; i++)
    {
        servos[i].attach(SERVO_PINS[i], SERVO_MIN_US, SERVO_MAX_US);
        setServo(i, SERVO_MID_ANGLE);
    }
    updateServos();
    delay(500);
}

/**
 *
 */
void demoMovements1()
{
    for (int pos = SERVO_MIN_ANGLE; pos < SERVO_MAX_ANGLE; pos += 4)
    {
        for (int i = 0; i < 6; i++)
        {
            setServo(i, pos);
        }
        updateServos();
        delay(100);
    }
    // hk.home(sp_servo);
    Serial.println("demoMovements1 DONE");
}

/**
 *
 */
void demoMovements2()
{
    for (size_t cnt = 0; cnt < 50; cnt += 5)
    {
        Serial.print("cnt = ");
        Serial.println(cnt);
        hk.moveTo(sp_servo, cnt, 0, 0, 0, 0, 0);
        updateServos();
        delay(500);
    }
    Serial.println("demoMovements2 DONE");
}

/**
 *
 */
void demoMovements3()
{
    Serial.println("demoMovements3 START");
    const int dval[][6] = {
        //sway
        {MAX_SWAY, 0, 0, 0, 0, 0},
        {MIN_SWAY, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},

        //surge
        {0, MAX_SURGE, 0, 0, 0, 0},
        {0, MIN_SURGE, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},

        //heave
        {0, 0, MAX_HEAVE, 0, 0, 0},
        {0, 0, MIN_HEAVE, 0, 0, 0},
        {0, 0, 0, 0, 0, 0},

        //pitch
        {0, 0, 0, MAX_PITCH, 0, 0},
        {0, 0, 0, MIN_PITCH, 0, 0},
        {0, 0, 0, 0, 0, 0},

        //roll
        {0, 0, 0, 0, MAX_ROLL, 0},
        {0, 0, 0, 0, MIN_ROLL, 0},
        {0, 0, 0, 0, 0, 0},

        //yaw
        {0, 0, 0, 0, 0, MAX_YAW},
        {0, 0, 0, 0, 0, MIN_YAW},
        {0, 0, 0, 0, 0, 0}};

    int ccount = (int)sizeof(dval) / sizeof(dval[0]);

    for (int cnt = 0; cnt < ccount; cnt++)
    {
        Serial.print("cnt = ");
        Serial.println(cnt);
        hk.moveTo(sp_servo, dval[cnt][0], dval[cnt][1], dval[cnt][2], dval[cnt][3], dval[cnt][4], dval[cnt][5]);
        updateServos();
        delay(1000);
    }
    Serial.println("demoMovements3 DONE");
}

/**
 *
 */
void demoMovements4()
{
    setServo(0, SERVO_MIN_ANGLE);
    setServo(1, SERVO_MIN_ANGLE);
    setServo(2, SERVO_MIN_ANGLE);
    setServo(3, SERVO_MIN_ANGLE);
    setServo(4, SERVO_MAX_ANGLE + 1);
    setServo(5, SERVO_MAX_ANGLE + 1);
    updateServos();

    Serial.println("demoMovements4 DONE");
}

/**
 *
 */
void joystickControl()
{
    int16_t joyX = joystick.getX();
    int16_t joyY = joystick.getY();
    int16_t joyZ = joystick.getZ();
    static int16_t lastJoyX = 0;
    static int16_t lastJoyY = 0;
    static int16_t lastJoyZ = 0;
    static uint8_t joyMode = 0;
    static const uint8_t nbJoyMode = 3;

    // Exit if joystick X, Y and Z did not change.
    bool joyStill;
    joyStill = ((joyX == lastJoyX) && (joyY == lastJoyY) && (joyZ == lastJoyZ));
    if (joyStill)
    {
        // Don’t check the joystick states too fast.
        // This is needed to remove noise.
        delay(10);
        return;
    }

    // Change joystick mode if button pressed.
    if (joyZ != 0)
    {
        joyMode = (joyMode + 1) % nbJoyMode;
        // Debounce.
        while (joystick.getZ())
        {
        }
        delay(250);
        return;
    }

    // Move according to joyMode.
    // TODO: Remember the previous joyMode positions when
    // changing joyMode instead of setting them to 0.
    if (joyMode == 0)
        // X, Y
        hk.moveTo(sp_servo, joyX, joyY, 0, 0, 0, 0);
    else if (joyMode == 1)
        // Z, tiltZ
        hk.moveTo(sp_servo, 0, 0, joyY, 0, 0, joyX);
    else if (joyMode == 2)
        // tilt X, tilt Y
        hk.moveTo(sp_servo, 0, 0, 0, joyX, joyY, 0);

    updateServos();

    // Record last joystick values.
    lastJoyX = joyX;
    lastJoyY = joyY;
    lastJoyZ = joyZ;

#if SEND_JOYSTICK_INFO_TO_SERIAL
    // Send joystick info to serial.
    Serial.print("\n\njoyX     = ");
    Serial.print(joyX);
    Serial.print(" | joyY     = ");
    Serial.print(joyY);
    Serial.print(" | joyZ     = ");
    Serial.println(joyZ);

    Serial.print("lastJoyX = ");
    Serial.print(lastJoyX);
    Serial.print(" | lastJoyY = ");
    Serial.print(lastJoyY);
    Serial.print(" | lastJoyZ = ");
    Serial.println(lastJoyZ);

    Serial.print("getRawX = ");
    Serial.print(joystick.getRawX());
    Serial.print(" | getRawY = ");
    Serial.println(joystick.getRawY());

    Serial.print("joyX == lastJoyX = ");
    Serial.println(joyX == lastJoyX);
    Serial.print("joyY == lastJoyY = ");
    Serial.println(joyY == lastJoyY);

    Serial.print("joyStill = ");
    Serial.println(joyStill);

    Serial.print("joyMode = ");
    Serial.println(joyMode);
#endif
}

/**
 *
 */
void setupSerial()
{
    Serial.begin(115200);
    Serial.print("\n\n##########################");
    Serial.print("\nCOMPILATION DATE AND TIME:\n");
    Serial.print(__DATE__);
    Serial.print("\n");
    Serial.print(__TIME__);
    Serial.print("\n##########################\n\n");
}

/**
 *
 */
void setupJoystick()
{
    Serial.print("millis = ");
    Serial.print(millis());
    Serial.print(" | getRawValueMidX = ");
    Serial.print(joystick.getRawValueMidX());
    Serial.print(" | getRawValueMidY = ");
    Serial.println(joystick.getRawValueMidY());

    joystick.calibrate();
    delay(10);
    hk.moveTo(sp_servo, 0, 0, 0, 0, 0, 0);
    updateServos();

    Serial.print("millis = ");
    Serial.print(millis());
    Serial.print(" | getRawValueMidX = ");
    Serial.print(joystick.getRawValueMidX());
    Serial.print(" | getRawValueMidY = ");
    Serial.println(joystick.getRawValueMidY());
}

/**
 *
 */
void setup()
{
    setupSerial();
    setupServos();
    setupJoystick();
    // demoMovements1();
    // demoMovements2();
    demoMovements3();
    // demoMovements4();
}

/**
 *
 */
void loop()
{
    joystickControl();
}
