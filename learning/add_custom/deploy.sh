#!/usr/bin/env bash
set -euo pipefail

# deploy.sh 所在目录，也就是 add_custom 根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

CUSTOM_OP_DIR="${PROJECT_ROOT}/custom_op"
OUT_DIR="${PROJECT_ROOT}/out"
SRC_FILE="${PROJECT_ROOT}/src/aclnn_test.cpp"
EXEC_FILE="${OUT_DIR}/execute_add_op"

echo "PROJECT_ROOT = ${PROJECT_ROOT}"

# 检查 ASCEND_TOOLKIT_HOME
if [[ -z "${ASCEND_TOOLKIT_HOME:-}" ]]; then
    echo "[ERROR] ASCEND_TOOLKIT_HOME is not set"
    echo "Please source Ascend environment first, for example:"
    echo "source /usr/local/Ascend/ascend-toolkit/set_env.sh"
    exit 1
fi

# 1. 编译算子工程
echo "========== Build custom op project =========="
cd "${CUSTOM_OP_DIR}"
bash build.sh

# 2. 安装部署算子包
echo "========== Install custom op package =========="
RUN_PACKAGES=( "${CUSTOM_OP_DIR}"/build_out/custom_opp*.run )

if [[ ! -f "${RUN_PACKAGES[0]}" ]]; then
    echo "[ERROR] custom_opp*.run not found in ${CUSTOM_OP_DIR}/build_out"
    exit 1
fi

bash "${RUN_PACKAGES[0]}" --install-path="${PROJECT_ROOT}"

# 3. 编译 host 调用代码
echo "========== Build host test code =========="
mkdir -p "${OUT_DIR}"

g++ \
    -I"${ASCEND_TOOLKIT_HOME}/include" \
    -I"${PROJECT_ROOT}/vendors/customize/op_api/include" \
    -L"${ASCEND_TOOLKIT_HOME}/lib64" \
    -L"${PROJECT_ROOT}/vendors/customize/op_api/lib" \
    "${SRC_FILE}" \
    -lcust_opapi \
    -lnnopbase \
    -lacl_rt \
    -o "${EXEC_FILE}"

echo "========== Build success =========="
echo "Executable: ${EXEC_FILE}"

echo "========== Run test =========="
"${EXEC_FILE}"