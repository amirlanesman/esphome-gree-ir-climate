# ESPHome Gree IR Climate

This is a custom ESPHome component for controlling Gree air conditioners via IR.  
**Unlike the built-in ESPHome Gree component, this integration supports both sending and receiving IR signals**, allowing for state synchronization when using the original remote.

## Features

- Full control of Gree AC units via IR (send/receive)
- State updates when using the original remote
- Supports all standard climate modes, fan speeds, and swing options

## Why use this component?

The default ESPHome Gree component only sends IR commands. This custom component can also **receive** IR signals from the original remote, keeping the ESPHome state in sync with the AC unit.

## Example Configuration

```yaml
...

remote_transmitter:
 pin: (your pin)
 carrier_duty_percent: 50%  #test with different values to see what works best
 id: ir_transmitter

 remote_receiver:
  pin: 
    number: (your pin)
    inverted: true # (or false, try both to see what works)
    mode: INPUT_PULLUP
  id: ir_receiver
  idle: 30ms  # super important: the gree protocol has a 19800 microsecond break between frames, so we need to set the idle time to at least that

climate:
- platform: greeir
  model: yac1fb9  # Optional: specify your Gree remote model (default: generic)
  set_modes: false
  repeat: 1
  name: Bedroom AC
  id: bedroom_ac
  transmitter_id: ir_transmitter
  receiver_id: ir_receiver

...
```

## Configuration Options

| Option           | Required | Type    | Description                                                                 |
|------------------|----------|---------|-----------------------------------------------------------------------------|
| `platform`       | Yes      | string  | Must be `greeir`                                                            |
| `name`           | Yes      | string  | Name of the climate entity                                                  |
| `model`          | No       | enum    | Gree remote model (see below). Defaults to `generic`                        |
| `set_modes`      | No       | bool    | If true, exposes supported modes to Home Assistant. Default: `false`        |
| `repeat`         | No       | int     | Number of times to repeat IR transmission. Default: `1`                     |
| `id`             | No       | id      | Optional ID for the climate component                                       |
| `transmitter_id` | Yes      | id      | ID of the remote_transmitter component                                      |
| `receiver_id`    | Yes      | id      | ID of the remote_receiver component                                         |

### Supported `model` values

- `generic` (default)
- `yaw1f`
- `ybofb`
- `yac1fb9`
- `yt1f`

## Notes

- Only tested with available model: **yac1fb9**
- Make sure your IR receiver and transmitter are connected to the correct GPIO pins.
- This component is designed for Gree AC units using the standard IR protocol.

## Credits

Based on the ESPHome climate platform and extended for IR receive support.