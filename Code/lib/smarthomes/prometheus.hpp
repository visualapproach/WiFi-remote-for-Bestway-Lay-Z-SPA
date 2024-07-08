#include <Arduino.h>
#include "ArduinoJson.h"
#include "config.h"
#include "main.h"

/**
 * prometheus related functions and char buffers
 * @author svanscho
 */

/**
 * response for /metrics/
 */
void handlePrometheusMetrics()
{
size_t const BUFSIZE = 2048;
char response[BUFSIZE];
char const *response_template =
  PSTR("# HELP " PROM_NAMESPACE "_info Metadata about the device.\n"
  "# TYPE " PROM_NAMESPACE "_info gauge\n"
  "# UNIT " PROM_NAMESPACE "_info \n"
  PROM_NAMESPACE "_info{version=\"%s\",name=\"%s\"} 1\n"
  "# HELP " PROM_NAMESPACE "_temperature_celcius Water temperature.\n"
  "# TYPE " PROM_NAMESPACE "_temperature_celcius gauge\n"
  "# UNIT " PROM_NAMESPACE "_temperature_celcius \u00B0C\n"
  PROM_NAMESPACE "_temperature_celcius %d\n"
  "# HELP " PROM_NAMESPACE "_target_temperature_celcius Water target temperature.\n"
  "# TYPE " PROM_NAMESPACE "_target_temperature_celcius gauge\n"
  "# UNIT " PROM_NAMESPACE "_target_temperature_celcius \u00B0C\n"
  PROM_NAMESPACE "_target_temperature_celcius %d\n"
  "# HELP " PROM_NAMESPACE "_heater_state Heater state.\n"
  "# TYPE " PROM_NAMESPACE "_heater_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_heater_state\n"
  PROM_NAMESPACE "_heater_state %d\n"
  "# HELP " PROM_NAMESPACE "_pump_state Pump state.\n"
  "# TYPE " PROM_NAMESPACE "_pump_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_pump_state\n"
  PROM_NAMESPACE "_pump_state %d\n"
  "# HELP " PROM_NAMESPACE "_jets_state Jets state.\n"
  "# TYPE " PROM_NAMESPACE "_jets_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_jets_state\n"
  PROM_NAMESPACE "_jets_state %d\n"
  "# HELP " PROM_NAMESPACE "_bubbles_state Bubbles state.\n"
  "# TYPE " PROM_NAMESPACE "_bubbles_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_bubbles_state\n"
  PROM_NAMESPACE "_bubbles_state %d\n"
  "# HELP " PROM_NAMESPACE "_power_state Bubbles state.\n"
  "# TYPE " PROM_NAMESPACE "_power_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_power_state\n"
  PROM_NAMESPACE "_power_state %d\n"
  "# HELP " PROM_NAMESPACE "_locked_state Locked state.\n"
  "# TYPE " PROM_NAMESPACE "_locked_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_locked_state\n"
  PROM_NAMESPACE "_locked_state %d\n"
  "# HELP " PROM_NAMESPACE "_unit_state Unit state.\n"
  "# TYPE " PROM_NAMESPACE "_unit_state gauge\n"
  "# UNIT " PROM_NAMESPACE "_unit_state\n"
  PROM_NAMESPACE "_unit_state %d\n");

  snprintf_P(response, BUFSIZE, response_template, FW_VERSION, DEVICE_NAME,
            bwc->cio->cio_states.temperature,
            bwc->cio->cio_states.target,
            bwc->cio->cio_states.heat,
            bwc->cio->cio_states.pump,
            bwc->cio->cio_states.jets,
            bwc->cio->cio_states.bubbles,
            bwc->cio->cio_states.power,
            bwc->cio->cio_states.locked,
            bwc->cio->cio_states.unit);
  server->send(200, F("text/plain; charset=utf-8"), response);
}
