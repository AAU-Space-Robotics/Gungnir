import myactuator_rmd_py as rmd

driver = rmd.CanDriver("can0")
actuator1 = rmd.ActuatorInterface(driver, 3)
print(actuator1.getVersionDate())
actuator1.sendPositionAbsoluteSetpoint(180.0, 200.0)

