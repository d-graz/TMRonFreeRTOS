# Triple Redundancy Module On FreeRTOS

Davide Grazzani, Riccardo Grossoni

Professors : William Fornaciari, Federico Reghenzani, Davide Baroffio



<div align="center">
  <img src="/docs/tex/Logo_Politecnico_Milano.svg_.png" width="200" />
</div>

**N.B.** : this page is a strip down version of the original documentation and has been created using `pandoc`. Find in `docs/` the complete version.

## Introduction

FreeRTOS is a lightweight real-time operating system designed for
embedded systems. It manages tasks, which are independent units of
execution, allowing multitasking by switching between them based on
priority. The objective of this project is to introduce a task-level,
software-based method of recovering the correct execution after a
single-time failure that extends the original FreeRTOS kernel.

Such recovery method in FreeRTOS is essential for ensuring system
reliability and resilience, especially in environments where single-time
failures can lead to critical malfunctions or data loss. By implementing
recovery mechanisms within tasks, the system can detect and address
failures locally, reducing the need for full system resets and
maintaining overall system stability.

This project has been developed for the course *Embedded Systems* at
Politecnico di Milano.

## User Interface

In this section the interface available to end-user will be presented.
Methods will be presented by usage domain.\
**N.B.**: to use any redundant feature present in this kernel you must
set `configUSE_REDUNDANT_TASK` to `1` in your `FreeRTOSConfig.h` config
file.

### Task Creation & Initialization

In this sub-section function related to task creation and initialization
are reported. It's important to note that these functions
should be called once and only once per task and should
anyway be called before `vTaskStartScheduler`
function. Any eventual deviation from these guidelines
will be specified in the function description itself.

-   ``` {.objectivec language="C"}
    BaseType_t xTaskCreateRedundant( TaskFunction_t pxTaskCode,
        const char * const pcName,
        const configSTACK_DEPTH_TYPE usStackDepth,
        void * const pvParameters,
        UBaseType_t uxPriority,
        TaskHandle_t * const pxCreatedTask ) PRIVILEGED_FUNCTION;
    ```

    This function creates a new redundant task. As you may notice, the
    signature of this function is exactly the same as `xTaskCreate`,
    hence the data needed to initialize a redundant task is no different
    from a default task.

-   ``` {.objectivec language="C"}
    BaseType_t xSetInput(TaskHandle_t task, void* pxStruct, UBaseType_t uxSize);
    ```

    This function sets the input
    structure `pxStruct` into a task given its
    own TaskHandle `task` and the size of the input structure `uxSize`.
    The input structure represent a custom user-defined data structure
    containing all the information needed for such a task to execute
    correctly a job. The input structure should be defined by the user
    based on the need of the task and should be correctly initialized
    before passing it to this function.

-   ``` {.objectivec language="C"}
    BaseType_t xSetOutput(TaskHandle_t task, UBaseType_t uxSize);
    ```

    Similar to `xSetInput`, this function sets the output
    structure size `uxSize` into a task given
    its own TaskHandle `task`.\
    **N.B.**: here the structure initialization is not needed as the
    kernel already handles the initialization. Moreover, this call is
    optional.

-   ``` {.objectivec language="C"}
    BaseType_t xSetCommitFunction(TaskHandle_t task, void (*pxCommitFunction)(void*), void* pxCommitFunctionArgs);
    ```

    This function sets a custom function `pxCommitFunction` ( and
    eventually the function parameters `pxCommitFunctionArgs` ) to a
    given task `task` given the TaskHandle. The commit function is a
    method executed only after a successful check of the integrity of
    the task and should be used only for state-changing actions and
    operations.

### Task Control

In this section all other APIs available to a redundant task are
presented. Keep in mind that this APIs are all optional and are designed
to facilitate the end-user workflow while building its application.

-   ``` {.objectivec language="C"}
    void* xGetInput();
    ```

    This function returns the input structure of a redundant task (the
    context is the current running task when the call is made). This
    method is useful when working inside the main body of a task and the
    user wants to retrieve the input structure of such a task.

-   ``` {.objectivec language="C"}
    void* xGetOutput();
    ```

    Similar to `xGetInput`, it returns the output structure of a
    redundant task.

-   ``` {.objectivec language="C"}
    void xSetTaskInput(TaskHandle_t task, void* pxStruct);
    ```

    Method used to modify the flow of a redundant task. Given a
    TaskHandle `task` and a compatible input structure `pxStruct`, the
    kernel sets such a structure as the the next input of the
    task. It's worth to be noted that the input is set
    only after a successful commit has happened, not
    immediately. This function is useful to synchronize
    operations between tasks.

-   ``` {.objectivec language="C"}
    void* xGetTaskOutput(TaskHandle_t task);
    ```

    Given a `task`, this function returns the output structure of a
    redundant task [updated to the last commit]. This method
    is useful to synchronize operations between tasks.

### Advanced function

In this section advanced function that should only be touched by
experienced FreeRTOS users are presented.
Keep in mind that every modification or override of this function may
leave the system in a non-functional state or break redundancy
mechanisms.

-   ``` {.objectivec language="C"}
    BaseType_t defaultRecoveryHandler();
    ```

    Default implementation of the recovery behavior during a system
    event (namely every vTask call that happens during a recovery
    phase). It's possible to override this function by setting
    `traceTASK_RECOVERY_MODE()` in `FreeRTOS.h` file.

-   ``` {.objectivec language="C"}
    void xSuspendTaskAhead();
    ```

    Default implementation of the `traceTASK_SWITCHED_IN()` that prevent
    racing condition in the context of the same redundant task
    (effectively preventing starving). It's possible to extend (or even
    override, discouraged) this function in `tasks.c` file.

-   ``` {.objectivec language="C"}
    void defaultDelayFailureHandler();
    ```

    Default implementation of how the system behaves in case of a major
    failure (a second error occurring before the first one is fixed).
    The default implementation stalls the system, override is possible.