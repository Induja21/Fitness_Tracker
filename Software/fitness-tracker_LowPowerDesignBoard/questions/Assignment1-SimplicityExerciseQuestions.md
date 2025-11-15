Note: For all assignments and Energy Profiler measurements you’ll be taking this semester,  Peak measurements are instantaneous measurements taken at a specific point in time. In the Energy Profiler, this is accomplished by left-clicking at a location along the time axis.
Average measurements are measurements that are taken over a time-span. In the Energy Profiler, this is accomplished by left-clicking and dragging a region along the time axis.

Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
   **Answer:** 5.49mA


**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
   **Answer:** 5.48mA


**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, referencing the main board schematic, WSTK-Main-BRD4001A-A01-schematic.pdf or WSTK-Main-BRD4002A-A06-schematic.pdf, and AEM Accuracy in the ug279-brd4104a-user-guide.pdf. Both of these PDF files are available in the ECEN 5823 Student Public Folder in Google drive at: https://drive.google.com/drive/folders/1ACI8sUKakgpOLzwsGZkns3CQtc7r35bB?usp=sharing . Extra credit is available for this question and depends on your answer.**
   **Answer:** My board is 10004A version which has 3k resistor connected in series with the LED.Say voltage drop across LED is 2V, Total current which flows through the resistor and LED is (3.3-2)/3k ~= 0.433mA. The LED consumes only 0.43 mA in both cases (regardless of whether the drive strength is strong or weak).There is no meaningful difference in current because the LED’s current requirement is much smaller than the available current in both drive strength scenarios. The total current drawn by the system is determined by the LED and resistor combination, and it remains at 0.43 mA regardless of whether the GPIO pin is set to Strong or Weak drive strength.


**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   **Answer:** Max current = 5.17mA , Min current = 4.71mA . So Average Current = (5.17mA+4.71mA)/2 = 4.94mA


**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   **Answer:** Max current = 5.75mA , Min current = 4.70mA . So Average Current = (5.17mA+4.71mA)/2 = 4.94mA, Average current = (5.75+4.7)/2 = 5.225mA



