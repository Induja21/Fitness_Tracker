Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

*Be sure to take measurements with logging disabled to ensure your logging logic is not impacting current/time measurements.*

*Please include screenshots of the profiler window detailing each current measurement captured.  See the file Instructions to add screenshots in assignment.docx in the ECEN 5823 Student Public Folder.* 

1. What is the average current per period? (Remember, once you have measured your average current, average current is average current over all time. Average current doesn’t carry with it the units of the timespan over which it was measured).
   Answer:16.38uA
   <br>Screenshot:  
   ![Avg_current_per_period](screenshots/assignment4/avg_current_per_period.png)  

2. What is the ave current from the time we sleep the MCU to EM3 until we power-on the 7021 in response to the LETIMER0 UF IRQ?
   Answer:2.11uA
   <br>Screenshot:  
   ![Avg_current_LPM_Off](screenshots/assignment4/avg_current_lpm_off.png)  

3. What is the ave current from the time we power-on the 7021 until we get the COMP1 IRQ indicating that the 7021's maximum time for conversion (measurement) has expired.
   Answer:
   <br>Screenshot:  118.2uA
   ![Avg_current_LPM_Off](screenshots/assignment4/avg_current_lpm_on.png)  

4. How long is the Si7021 Powered On for 1 temperature reading?
   Answer: 98.7ms
   <br>Screenshot:  
   ![duration_lpm_on](screenshots/assignment4/power_on_time.png)  

5. Given the average current per period from Q1, calculate the operating time of the system for a 1000mAh battery? - ignoring battery manufacturers, efficiencies and battery derating - just a first-order calculation.
   Answer (in hours):61050.06hours
   
6. How has the power consumption performance of your design changed since the previous assignment?
   Answer: Since we are doing interrupt based i2c transfer, sleeping in between the transfers and waiting using timer interrupts has drastically reduced, the average current by 1/10th from almost 154uA to 16uA and so  the current from Power On until the data conversion is complete from 4.7mA to 118uA
   


