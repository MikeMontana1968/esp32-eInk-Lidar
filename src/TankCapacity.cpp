#pragma once
#include <Arduino.h>

class TankCapacity {
    float tank_max_height_mm;
    float tank_capacity_l;

    // Polynomial coefficients: volume from height
    static constexpr float V_COEF[6] = {-0.487443f, -1.674212f, 1.908638f, 1.333798f, -0.100086f, 0.001364f};

    // Polynomial coefficients: height from fill percentage  
    static constexpr float H_COEF[6] = {13.174324f, -33.189784f, 30.914695f, -13.198428f, 3.186544f, 0.063000f};

    public:
    TankCapacity(float max_height_mm = 261.9f, float capacity_l = 17.50f) : 
        tank_max_height_mm(max_height_mm), 
        tank_capacity_l(capacity_l) 
     { }
    

    // Evaluate 5th degree polynomial: c0*x^5 + c1*x^4 + c2*x^3 + c3*x^2 + c4*x + c5
    float polyEval(const float coef[], float x) {
        float x2 = x * x;
        float x3 = x2 * x;
        float x4 = x3 * x;
        float x5 = x4 * x;
        return coef[0]*x5 + coef[1]*x4 + coef[2]*x3 + coef[3]*x2 + coef[4]*x + coef[5];
    }

    // Get fuel volume (liters) from height sensor reading (mm)
    float getVolumeLiters(float height_mm) {
        float h = constrain(height_mm / tank_max_height_mm, 0.0f, 1.0f);
        float v = polyEval(V_COEF, h);
        return constrain(v * tank_capacity_l, 0.0f, tank_capacity_l);
    }

    // Get fuel volume (gallons) from height sensor reading (mm)
    float getVolumeGallons(float height_mm) {
        return getVolumeLiters(height_mm) / 3.78541f;
    }

    // Get fill percentage (0-100) from height (mm)
    float getFillPercent(float height_mm) {
        return (getVolumeLiters(height_mm) / tank_capacity_l) * 100.0f;
    }

    // Get expected height (mm) for a target fill percentage (0-100)
    float getHeightFromPercent(float fill_pct) {
        float p = constrain(fill_pct / 100.0f, 0.0f, 1.0f);
        float h = polyEval(H_COEF, p);
        return constrain(h * tank_max_height_mm, 0.0f, tank_max_height_mm);
    }
}; // class