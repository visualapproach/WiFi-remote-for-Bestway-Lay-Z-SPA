Instructions:

Add the following to you `configuration.yaml`:
```
homeassistant:
  packages: !include_dir_named packages
```
Create `packages` directory in your Home Assistant configuration directory (where your `configuration.yaml`is located) and save `layzspa.yaml` inside.
Restart Home Assistant.

If you make any changes in `layzspa.yaml` in the future, you can reload your changes without restarting by navigating to `Configuration -> Settings` and then click on `Manually configured MQTT entities`.
