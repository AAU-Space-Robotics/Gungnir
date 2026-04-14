import myactuator_rmd_py as rmd

driver = rmd.CanDriver("can0")

while True:
    j2 = rmd.ActuatorInterface(driver, 2)
    j2.reset()
    print(j2.getVersionDate())