import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID, CONF_MODEL, CONF_REPEAT

# AUTO_LOAD = ["climate_ir"]
AUTO_LOAD = ["climate_ir"]
CODEOWNERS = ["@amirlanesman"]

greeir_ns = cg.esphome_ns.namespace("greeir")
GreeIRClimate = greeir_ns.class_("GreeIRClimate", climate_ir.ClimateIR)

# Gree model variants
GreeIRModel = greeir_ns.enum("GreeIRModel", is_class=True)
GREE_MODELS = {
    "GENERIC": GreeIRModel.GENERIC,
    "YAW1F": GreeIRModel.YAW1F,
    "YBOFB": GreeIRModel.YBOFB,
    "YAC1FB9": GreeIRModel.YAC1FB9,
    "YT1F": GreeIRModel.YT1F,
}

CONF_WIFI_FUNCTION = "wifi_function"
CONF_CHECK_CHECKSUM = "check_checksum"
CONF_SET_MODES = "set_modes"

CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(GreeIRClimate),
        cv.Optional(CONF_MODEL, default=GREE_MODELS["GENERIC"]): cv.enum(
            GREE_MODELS, upper=True
        ),
        cv.Optional(CONF_WIFI_FUNCTION, default=False): cv.boolean,
        cv.Optional(CONF_CHECK_CHECKSUM, default=False): cv.boolean,
        cv.Optional(CONF_SET_MODES, default=False): cv.boolean,
        cv.Optional(CONF_REPEAT, default=1): cv.int_range(min=1, max=100),
    }
)


async def to_code(config):

    var = cg.new_Pvariable(config[CONF_ID])

    cg.add(var.set_model(config[CONF_MODEL]))
    cg.add(var.set_wifi_function(config[CONF_WIFI_FUNCTION]))
    cg.add(var.set_check_checksum(config[CONF_CHECK_CHECKSUM]))
    cg.add(var.set_set_modes(config[CONF_SET_MODES]))
    cg.add(var.set_repeat(config[CONF_REPEAT]))

    await climate_ir.register_climate_ir(var, config)
