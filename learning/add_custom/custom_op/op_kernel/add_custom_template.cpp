#include "kernel_operator.h"
#include "add_custom_template_tiling.h"

extern "C" __global__ __aicore__ void add_custom_template(GM_ADDR x, GM_ADDR y, GM_ADDR z, GM_ADDR workspace, GM_ADDR tiling) {
    REGISTER_TILING_DEFAULT(AddCustomTemplateTilingData);
    GET_TILING_DATA(tilingData, tiling);
    // TODO: user kernel impl
}