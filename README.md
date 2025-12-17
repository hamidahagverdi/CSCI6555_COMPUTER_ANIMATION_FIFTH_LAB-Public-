# CSCI6555_COMPUTER_ANIMATION_FIFTH_LAB-Public-

Lab 5 – Fancy Spline-Based Animation
CS 6555 – Computer Animation
George Washington University
1. Overview
This project implements a fancy spline-based animation system using OpenGL and GLUT.
The animation demonstrates constant-speed motion along a spline path, combined with a cinematic follow camera and orientation aligned with the path tangent.
The goal of this lab is to go beyond basic spline motion by adding realistic animation techniques commonly used in computer animation systems.
2. Platform and Environment
Operating System: macOS
Language: C++
Graphics API: OpenGL
Windowing Toolkit: GLUT
Note: OpenGL is deprecated on macOS but still supported for coursework.
3. Compilation and Execution (macOS)
Compile
g++ lab5.cpp -o lab5 -framework OpenGL -framework GLUT
Run
./lab5
4. Core Features
4.1 Spline Motion
The animated object moves along a closed 3D spline path defined by a set of control points.
Two spline types are supported:
Catmull-Rom spline
Uniform B-spline
The user can switch between spline types at runtime.
4.2 Arc-Length Parameterization (Fancy Feature)
Instead of using the spline parameter directly, the system computes an arc-length lookup table.
This ensures that the object moves at a constant speed, even in regions of high curvature.
This technique avoids the common problem of objects speeding up and slowing down along curves and is widely used in professional animation systems.
4.3 Orientation and Banking
The object’s orientation is aligned with the tangent of the spline
The object visually “faces” the direction of motion
Subtle banking is applied during turns to enhance realism
4.4 Cinematic Follow Camera
A spring-based camera smoothly follows the animated object:
Positioned behind and above the object
Uses damping and stiffness parameters for smooth motion
Avoids abrupt camera changes
This creates a cinematic and professional visual effect.
5. User Interaction
Keyboard Controls
1 – Switch to Catmull-Rom spline
2 – Switch to B-spline
Space – Pause / resume animation
+ / - – Increase / decrease speed
P – Toggle spline path visibility
G – Toggle ground grid
R – Reset animation
Esc – Exit program
6. Rendering
Object rendered as a simple boid-like shape
Optional ground grid for spatial reference
Optional spline path visualization
Depth testing enabled for proper 3D rendering
7. Input and Output
Input
Predefined spline control points
User keyboard interaction
Output
Real-time animated motion along a spline with constant speed
Smooth camera motion and orientation changes
8. “Fancy” Extensions Implemented
Arc-length parameterized spline motion
Runtime switching between spline types
Cinematic follow camera
Tangent-based orientation and banking
Interactive controls and visualization toggles
9. Educational Value
This lab demonstrates key animation concepts:
Parametric curves
Arc-length reparameterization
Camera dynamics
Orientation from motion
Real-time interactive animation systems
10. References
Foley et al., Computer Graphics: Principles and Practice
Catmull & Rom, A Class of Local Interpolating Splines
Farin, Curves and Surfaces for Computer-Aided Geometric Design
