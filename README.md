# Smart-Power-Management 
<img width="744" height="523" alt="image" src="https://github.com/user-attachments/assets/b0c590d4-fb6e-440f-8940-4836b0548577" />


# Energy Management System (EMS) 
The EMS operates based on a hierarchy of power sources and user-defined priorities (e.g., maximize self-consumption, prioritize battery charging, time-of-use optimization, reliable backup power). The general logic flow follows these steps: 
# Prioritize Solar Use: 
When solar power is sufficient, the EMS directs it to power the AC loads directly via the inverter.
# Manage Excess Power:
If there is surplus solar power beyond the load demand, the EMS directs it to charge the battery bank via the bidirectional DC-DC converter.
If the battery is fully charged, and allowed by regulations, the remaining excess power is exported to the utility grid.
# Manage Insufficient Power (Daytime):
If solar power is insufficient to meet the load demand, the EMS first draws power from the battery bank to cover the shortfall.
If the battery SOC is low, or the demand is too high for the battery output, the EMS draws the remaining required power from the utility grid.
# Manage Insufficient Power (Nighttime/Outage):
When solar power is unavailable, the EMS supplies the loads using stored battery power.
If the battery depletes or during a grid outage, the transfer switch isolates the system from the grid and the inverter continues to power critical loads from the battery (off-grid mode/UPS mode).
The EMS may use an AC/DC converter to charge the battery from the AC mains during off-peak hours if programmed to do so.
# Monitoring and Protection:
Throughout operation, the EMS continuously monitors parameters like voltage, current, frequency, and temperature. It activates protection circuits (e.g., overcurrent, overvoltage, islanding protection) and provides status updates via a display or monitoring software.
