# Recreating-Exoplanet-Detection-in-the-Classroom
This repository contains resources for building and utilizing a low-cost, interactive model simulating the transit method of exoplanet detection. Designed as a teaching aid, this model enables visualization and experimentation with light curves generated during planetary transits, fostering a deeper understanding of this widely used exoplanet detection technique.

## Features
- **Interactive Simulation:** Adjust parameters like star-planet distance and orbital period via a web-based interface.
- **Hands-On Learning:** Observe and analyze light curves to derive properties of exoplanetary systems.
- **Customizable:** Includes resources for 3D printing, circuit assembly, and microcontroller programming.

---

## Repository Contents

1. **CAD Models:**
   - SolidWorks files for mechanical parts.
2. **STL Files:**
   - Ready-to-print files for all mechanical components.
3. **Circuit Schematic:**
   - Circuit schematic for assembling the electronic components.
4. **Microcontroller Code:**
   - Code for the ESP-1 (master) and ESP-2 (slave).
5. **Assembly Pictures:**
   - Images of the fully assembled model for reference.

---

## How to Use

1. **3D Printing:**
   - Use the provided STL files to print the components. User is encouraged to 
2. **Mechanical Assembly:**
   - Follow the CAD models and assembly images to construct the model.
3. **Circuit Setup:**
   - Refer to the circuit schematic to connect the components.
4. **Programming the Microcontrollers:**
   - Load the `ESP1_Master` code onto the master ESP32.
   - Load the `ESP2_Slave` code onto the slave ESP32.
5. **Simulating Transits:**
   - Power up the model and connect to the web server hosted by the master ESP32.
   - Access the user interface on any device with a browser.
   - Adjust parameters and simulate exoplanet transits.
   - Record and analyze the generated light curves using a light sensor.

---
### Acknowledgments
This project was supported by the Small-Spacecraft Systems and Payload Centre (SSPACE) based at the Indian Institute of Space Science and Technology (IIST), Thiruvananthapuram, Kerala, India.
