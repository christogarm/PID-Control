# PID Controller

A Proportional-Integral-Derivative (PID) controller is a feedback-based control loop mechanism used to regulate systems that require continuous control and automatic adjustment.

## Continuous-Time Model

The PID controller in continuous time is defined as:

$$
u(t) = K_p e(t) + K_i \int_0^t e(\tau)\, d\tau + K_d \frac{de(t)}{dt}
$$

where:

- $u(t)$ is the control signal  
- $e(t)$ is the error signal  
- $K_p$, $K_i$, and $K_d$ are the proportional, integral, and derivative gains.  

## Discrete-Time Model

Digital systems implement the PID controller in discrete time:

$$
u[k] = K_p e[k] + K_i \cdot T \sum_{j=0}^{k} e[j] + K_d \frac{e[k] - e[k-1]}{T}
$$

where:

- $T$ is the sampling period  
- $k$ is the discrete time index  

## Error Definition

The error signal is defined as the difference between the setpoint and the measured value:

$$
e[k] = SP - V_{measurement}
$$

---

# How to Use This API

You only need two main functions:

### Initialization
```c
PID_Init(PID_Control *pid_, PID_Parameters *param_);
```

### Update
```c
PID_Update(PID_Control *Ppid_, float input_, float dt_);
```

Call **`PID_Update`** only when a new input sample is available.

---

# PID Configuration

The **`PID_Init(PID_Control *pid, PID_Parameters *param)`** function initializes and configures the PID controller with the following parameters:

- **`setPoint`**: desired value  
- **`Kp, Ki, Kd`**: proportional, integral, and derivative gains  
- **`dt`**: sampling period  
- **`minOutput, maxOutput`**: output limits  
- **`clampIntMin, clampIntMax`**: integral limits for anti-windup  
- **`rateLimit`**: maximum rate of change of the output  
- **`deadband`**: error threshold  
- **`alpha`**: IIR filter coefficient  

---

## Output Limits

Actuators usually have minimum and maximum limits. For example, a fan may operate between 0% and 100% speed.

You can use **`minOutput`** and **`maxOutput`** to constrain the controller output.

Additionally, you can configure these limits using:

```c
PID_setLimitOut(PID_Control *pid_, float minOut_, float maxOut_);
```

---

## Integral Limits (Anti-Windup)

The integral term can accumulate excessive error (windup), which may cause poor system behavior.

To prevent this, the integral term is clamped between predefined limits.

You can configure these limits using:

```c
PID_setClampInt(PID_Control *pid_, float minInt_, float maxInt_);
```

---

## Rate Limit

Large and sudden changes in the actuator output may damage the system or cause instability.

The rate limit restricts how fast the output can increase or decrease.

- This parameter must be greater than 0

Configuration function:

```c
PID_setRateLimit(PID_Control *pid_, float rateLimit_);
```

---

## Deadband

When the measured value is very close to the setpoint, small changes may cause unnecessary oscillations.

The deadband defines a range where the controller does not react.

Configuration function:

```c
PID_setDeadband(PID_Control *pid_, float deadband_);
```

---

## IIR Filter (Derivative Term)

The derivative term is very sensitive to noise and can destabilize the system.

To reduce this effect, a low-pass IIR filter is applied:

$$
e_{Fd}[k] = \alpha \cdot e_d[k] + (1 - \alpha) \cdot e_d[k-1]
$$

where:

- $e_{Fd}[k]$: filtered derivative error  
- $e_d[k]$: current derivative error  
- $e_d[k-1]$: previous derivative error  
- $\alpha$: filter coefficient, with $0 < \alpha < 1$

Interpretation:

- If $\alpha \rightarrow 1$: less filtering, faster response  
- If $\alpha \rightarrow 0$: more filtering, smoother response  

Configuration function:

```c
PID_setAlpha(PID_Control *pid_, float alpha_);
```

---

# Other Implemented Features

## Derivative Kick Reduction

A sudden change in the setpoint can cause instability due to the derivative term.

To avoid this, the derivative is computed using only the measurement:

$$
e_d[k] = \frac{v[k-1] - v[k]}{T}
$$

where:

- $v[k]$: current measurement  
- $v[k-1]$: previous measurement  
- $T$: sampling period  

---

## Integral Freeze at Saturation

When the output reaches its maximum or minimum limit, the integral term is stopped to prevent further accumulation.

This helps maintain a smoother system response.

