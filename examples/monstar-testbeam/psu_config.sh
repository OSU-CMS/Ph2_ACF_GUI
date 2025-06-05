# Device resource definitions
alias opto_psu="tti -O 2 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT73B5W7-if00-port0::INSTR"
alias itkpix_psu="tti -O 1 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT73B5W7-if01-port0::INSTR"
alias itkpix_hv="keithley2410 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT73B5W7-if03-port0::INSTR"

alias itkpix_temp="keithley2000 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT7YBKLN-if00-port0::INSTR"
alias alu_temp="keithley2000 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT7YBKLN-if01-port0::INSTR"
alias peltier="tti -O 2 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT7YBKLN-if02-port0::INSTR"
alias humidity="keithley2000 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT7YBKLN-if03-port0::INSTR"
alias itkpix_fan="tti -O 1 -R ASRL/dev/serial/by-id/usb-FTDI_USB__-__Serial_Cable_FT73B5W7-if02-port0::INSTR"

alias itkpix_horizontal="ximc -R xi-com:///dev/ximc/00007E24"
alias itkpix_vertical="ximc -R xi-com:///dev/ximc/00007E61"

# MQTT instance settings
export ICICLE_MQTT_BROKER='***REDACTED***.cern.ch'
export ICICLE_MQTT_PORT='8883'
export ICICLE_MQTT_USERNAME='***REDACTED***'
export ICICLE_MQTT_PASSWORD='***REDACTED***'
export ICICLE_MQTT_SSL=1
