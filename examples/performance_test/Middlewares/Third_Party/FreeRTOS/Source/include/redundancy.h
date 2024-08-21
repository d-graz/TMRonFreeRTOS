#ifndef __REDUNDANCY__
#define __REDUNDANCY__
#if ( configUSE_REDUNDANT_TASK == 1)
    typedef void (*xUpdateInput)(void*, void*);      /*< Used for the update input function of the task. */

    typedef struct xRedundantShared {
        void (*pxCommitFunction)(void*);               /*< Used for the commit function of the task. */
        void *pxCommitFunctionParameter;               /*< Used for the parameter of the commit function of the task. */
        void * pxSharedInputStruct;                  /*< Input at t-1 for task. */
        UBaseType_t uInputStructSize;                  /*< Used for the size of the input structure of the task. */
        UBaseType_t uOutputStructSize;                 /*< Used for the size of the output structure of the task. */
        BaseType_t isRecoveryProcess;                  /*< Used for the recovery process of the task. */
        TaskFunction_t taskCode;                       /*< Used for the task code of the task. */
        uint32_t stackDepth;                           /*< Used for the stack depth of the task. */
        void * pvParameters;                           /*< Used for the parameters of the task. */
        void * pxCommitInputStruct;                    /*< Used for the changing of the input structure from another task. */
        void * pxSharedOutputStruct;                  /*< Output of the task at last commit. */
    } xRedundantShared_t;

    typedef struct xRedundantStruct {
        xRedundantShared_t * pxRedundantShared;        /*< Used for the shared structure of the task. */
        struct tskTaskControlBlock * pxTaskValidation; /*< Used for validation of the TCB. */
        struct tskTaskControlBlock * pxTaskSUS;        /*< Used for error correction of the TCB in case of divergent results in the validation task. */
        uint64_t iterationCounter;                     /*< Used for controlling the execution status of the task. */
        void * pxInputStruct;                          /*< Used for the input structure of the task. */
        void * pxOutputStruct;                         /*< Used for the output structure of the task. */
    } xRedundantStruct_t;

#endif
#endif

