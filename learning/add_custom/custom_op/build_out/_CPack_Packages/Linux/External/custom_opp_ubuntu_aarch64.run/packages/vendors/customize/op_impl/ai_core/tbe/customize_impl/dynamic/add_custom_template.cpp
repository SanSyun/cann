#include "kernel_operator.h"
#include "add_custom_template_tiling.h"
#include "kernel_operator_dump_tensor_intf_impl.h"


constexpr int32_t BUFFER_NUM = 1; 

template <class dtypeX, class dtypeY, class dtypeZ>
class KernelAdd{
public:
    __aicore__ inline KernelAdd() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, GM_ADDR z, uint32_t totalLength, uint32_t tileNum) {
        this->blockLength = totalLength / AscendC::GetBlockNum();
        this->tileNum = tileNum;
        this->tileLength = this->blockLength / this->tileNum / BUFFER_NUM;

        //设置全局内存地址
        xGm.SetGlobalBuffer((__gm__ dtypeX*)x + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
        yGm.SetGlobalBuffer((__gm__ dtypeY*)y + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
        zGm.SetGlobalBuffer((__gm__ dtypeZ*)z + this->blockLength * AscendC::GetBlockIdx(), this->blockLength);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, this->tileLength * sizeof(dtypeX));
        pipe.InitBuffer(inQueueY, BUFFER_NUM, this->tileLength * sizeof(dtypeY));
        pipe.InitBuffer(outQueueZ, BUFFER_NUM, this->tileLength * sizeof(dtypeZ));
    }

    __aicore__ inline void Process() {
        int32_t loopCount = this->tileNum * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) {
            this->CopyIn(i);
            this->Compute(i);
            this->CopyOut(i);
        }
        AscendC::printf("Core %d executed %d times in total\n",  AscendC::GetBlockIdx(), loopCount);
    }

private: //搬入-计算-搬出
    __aicore__ inline void CopyIn(int32_t progress)
    {
        //初始化本地内存
        AscendC::LocalTensor<dtypeX> xLocal = inQueueX.AllocTensor<dtypeX>();
        AscendC::LocalTensor<dtypeY> yLocal = inQueueY.AllocTensor<dtypeY>();

        //从全局内存搬入本地内存
        AscendC::DataCopy(xLocal, xGm[progress * this->tileLength], this->tileLength);
        AscendC::DataCopy(yLocal, yGm[progress * this->tileLength], this->tileLength);

        //将本地内存放入队列
        inQueueX.EnQue(xLocal);
        inQueueY.EnQue(yLocal);
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        //从队列中取出到本地内存
        AscendC::LocalTensor<dtypeX> xLocal = inQueueX.DeQue<dtypeX>();
        AscendC::LocalTensor<dtypeY> yLocal = inQueueY.DeQue<dtypeY>();
        AscendC::LocalTensor<dtypeZ> zLocal = outQueueZ.AllocTensor<dtypeZ>();

        //计算
        AscendC::Add(zLocal, xLocal, yLocal, this->tileLength);

        //将计算结果放入队列
        outQueueZ.EnQue<dtypeZ>(zLocal);
        inQueueX.FreeTensor(xLocal);
        inQueueY.FreeTensor(yLocal);
    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        //从队列中取出到本地内存
        AscendC::LocalTensor<dtypeZ> zLocal = outQueueZ.DeQue<dtypeZ>();

        //将本地内存搬出到全局内存
        AscendC::DataCopy(zGm[progress * this->tileLength], zLocal, this->tileLength);

        //释放本地内存
        outQueueZ.FreeTensor(zLocal);
    }

private:
    uint32_t blockLength; //每个block处理的总长度
    uint32_t tileNum; //分片数量
    uint32_t tileLength; //每片长度
    AscendC::GlobalTensor<dtypeX> xGm;
    AscendC::GlobalTensor<dtypeY> yGm;
    AscendC::GlobalTensor<dtypeZ> zGm;
    AscendC::TPipe pipe;
    AscendC::TQue<AscendC::TPosition::VECIN, BUFFER_NUM> inQueueX;
    AscendC::TQue<AscendC::TPosition::VECIN, BUFFER_NUM> inQueueY;
    AscendC::TQue<AscendC::TPosition::VECOUT, BUFFER_NUM> outQueueZ;

};

extern "C" __global__ __aicore__ void add_custom_template(GM_ADDR x, GM_ADDR y, GM_ADDR z, GM_ADDR workspace, GM_ADDR tiling) {
    REGISTER_TILING_DEFAULT(TilingDataTemplate); //获取 tiling数据
    GET_TILING_DATA_WITH_STRUCT(TilingDataTemplate, tiling_data, tiling);
    KernelAdd<DTYPE_X, DTYPE_Y, DTYPE_Z> op;
    op.Init(x, y, z, tiling_data.totalLength, tiling_data.tileNum);
    op.Process();
}