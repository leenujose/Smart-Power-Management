# Smart-Power-Management 
<img width="744" height="523" alt="image" src="https://github.com/user-attachments/assets/b0c590d4-fb6e-440f-8940-4836b0548577" />


# Energy Management System (EMS) 
The EMS operates based on a hierarchy of power sources and user-defined priorities (e.g., maximize self-consumption, prioritize battery charging, time-of-use optimization, reliable backup power).
# I built an ESP32-based EMS that:
🔹 Reads real-time voltages from solar, battery, and grid sources
🔹 Fetches weather forecasts using an API (so it “knows” if it’s going to be cloudy or sunny!)
🔹 Automatically adjusts battery reserve levels depending on the forecast
🔹 Selects the best power source for that moment
🔹 Sends instant SMS alerts through GSM when there’s a power outage ⚠️
🔹 And displays everything neatly on an OLED screen, while also uploading data to Adafruit IO for remote monitoring 🌐
