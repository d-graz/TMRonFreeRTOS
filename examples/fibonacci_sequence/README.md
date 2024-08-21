If you are not familiar with the Fibonacci sequence take a look at this
Fibonacci sequence first.\

1.  Definition of the task:

    ``` {.objectivec language="C"}
    // global scope

    // using default CMSIS v2 interface to define a new task
    osThreadId_t taskFibonacci;
    const osThreadAttr_t taskFibonacciAttributes = {
      .name = "fiboTask",
      .stack_size = 500 * 4,
      .priority = (osPriority_t) osPriorityNormal,
    };

    // define input and output structure
    typedef struct inputFibonacci {
      int n_previous;
      int n_current;
    } inputFibonacci_t;

    typedef struct outputFibonacci {
      int n_next;
    } outputFibonacci_t;

    //define a commit function
    void commitFibonacci(){
      outputFibonacci_t * output = (outputFibonacci_t*) xGetOutput();
      printf("output FIBONACCI [COMMIT] : %d\n", output->n_next);
    }

    // declare the main body of the task
    void taskFibonacciBody(void *argument);
    ```

    As you can see here we have defined 2 structure, one for the input
    and one for the output. In the input structure stores $F_{n-1}$ and
    $F_{n-2}$ meanwhile the output structure stores $F_n$. It's
    important to be noted, as discussed in section
    2, that at each iteration the
    output should be entirely generated again. We will better this
    concept in a few steps. We also define the `commitFibonacci`
    function, a method executed only after each successful commit. In
    this case the function retrieves the output by calling
    `(outputFibonacci_t*) xGetOutput()` (note the required cast!) and
    subsequently proceeds to print the output.

2.  Creation of the task:

    ``` {.objectivec language="C"}
    // in the main function

    /* Init scheduler */
    osKernelInitialize();
      
    //initializing taskFibonacci
    // here osThreadNewRedundant it's only a wrapper of xTaskCreateRedundant, code available on References and Material
    taskFibonacci=osThreadNewRedundant(taskFibonacciBody, NULL, &taskFibonacciAttributes);

    //initializing the input of the task
    inputFibonacci_t * input_og = pvPortMalloc(sizeof(inputFibonacci_t));
    input_og->n_previous = 0;
    input_og->n_current = 1;
    xSetInput(taskFibonacci, input_og, sizeof(inputFibonacci_t));

    //setting the output
    xSetOutput(taskFibonacci, sizeof(outputFibonacci_t));

    //setting the commit function
    xSetCommitFunction(taskFibonacci, commitFibonacci, NULL);

    /* Start scheduler */
    osKernelStart();
    ```

    Here we create the task using `osThreadNewRedundant`, a wrapper function for
    `xTaskCreateRedundant`. Once the task is created, we proceed to
    initialize the input as required by the Fibonacci sequence
    ($F_0 = 0$ and $F_1 = 1$) and then setting the output. We tell the
    kernel not the actual values of the output (in fact as you can see
    no initialization of the structure is required) but instead only the
    size the kernel should reserve for the output itself. Subsequently
    by calling `xSetCommitFunction` we link the commit function to the
    task itself. `osKernelStart` starts FreeRTOS.\
    **N.B.**: it's imperative to call `xSetInput` once and before
    `osKernelStart`, `xSetOutput` it's optional but should be included
    in the same scope of `xSetInput`. `xSetCommitFunction` instead can
    be called multiple times either in the `main` scope or in the
    `task-body` scope.

3.  Task body:

    ``` {.objectivec language="C"}
    void taskFibonacciBody(void *argument){

      //-- header of the task --
      int result;
      outputFibonacci_t * output;
      inputFibonacci_t * input;

      //-- main for loop --
      for(;;){

        // just making sure every loop cycle the correct pointers are fetched
        // this is not strictly necessary, could be moved to the header of the task
        input = (inputFibonacci_t*) xGetInput();
        output= (outputFibonacci_t*) xGetOutput();

        // calculating new output
        result = input->n_previous + input->n_current;
        output->n_next = result;

        //updating previous input for the next cycle
        input->n_previous = input->n_current;
        input->n_current = output->n_next;

        // calling osDelay
        // at this point the task is suspended. Once the validation task reaches the same point in execution
        // of this task the health check is performed. Upon successful check, commit function taskFibonacci is executed.
        osDelay(5000);
      }
    }      
    ```

    Let's take a look to the main body of the Fibonacci task. In this
    example you can clearly distinguish two parts: the header of the
    task, the part of the function prior to the for-loop, and the main
    for-loop itself. It's important to clearly define this two areas of
    the task's body in order to assure a correct recovery process in case of failure.

    -   **Header of the task**: in this part of the function it's
        important [to not to]{.underline} [write any static,
        non-conditional statements]{.underline}. This is because the
        header of the task is a soft initialization zone, that gets
        executed every time a new task is spawned, hence also in case of
        error in the task. To understand better this concept we analyze
        our code:

        ``` {.objectivec language="C"}
        //-- header of the task --
        int result;
        outputFibonacci_t * output;
        inputFibonacci_t * input;        
        ```

        Here we only declare our variables but we do not initialize them
        until the body of the for-loop. If we were to initialize them we
        would need to make sure that, in case of error, the spawned task
        should initialize in a state which is at the same point of
        execution of the original task. From this consideration we
        should acknowledge:

        1.  The use of the input and output structure is fundamental as
            it has be designed to solve this specific problem

        2.  Avoid local variables outside input and output structure if
            possible

        3.  Pointers behave differently since they are stored in the
            heap so it's always possible to reference them, but you
            should then consider data integrity problem. Once again a
            possible solution is to use input and output structure.

        For the sake of completeness let's make a **WRONG example**:

        ``` {.objectivec language="C"}
        // WRONG EXAMPLE
        void taskFibonacciBody(void *argument){

          //-- header of the task --
          int result;
          outputFibonacci_t * output;
          inputFibonacci_t * input;

          // this is fine.
          input = (inputFibonacci_t*) xGetInput();
          output= (outputFibonacci_t*) xGetOutput();

          // hard coded intialization BROKE RECOVERY
          input->n_previous = 0;
          input->n_current = 1;

          //-- main for loop --
          for(;;){
            //for loop code
          }
        }      
        ```

        In this example we first declare and get the input and output
        structure. You can safely call `xGetInput` and `xGetOutput`
        inside the header of the task as those are special function that
        do handle data integrity checks during recovery period (meaning
        that if an error occurs during the task execution this function
        will return prior-failure data).

        ``` {.objectivec language="C"}
        // hard coded intialization BROKE RECOVERY
        input->n_previous = 0;
        input->n_current = 1;
        ```

        This however breaks the recovery process. This is due to the
        fact that at iteration $x$, when a failure happens, the value of
        the input structure should be restored to $x-1$ iteration, but
        they are overridden by those two upper lines of code resulting
        in a broken recovery process.

    -   **Main For-Loop of the task**: in this part of the function the
        implementation of the task's logic should be written. No
        particular recommendation should be made here, aside from the
        following:

        1.  `osDelay` should be called every time a check with the
            validation task is needed, this is ideally once per loop
            cycle [at the end of such a cycle]{.underline}. This is
            because, in case of failure, the task should be able to go
            through at least a full iteration.

        2.  The output structure should be entirely populated every
            iteration of the task, because in case of error during the
            execution, the [entire]{.underline} [output
            structure]{.underline} of the task is
            [dirty]{.underline}[^5]. Let's take a look at this **WRONG
            example**:

            ``` {.objectivec language="C"}
            typedef struct inn {
                int a;
                int b;
                int counter;
            } inn_t;

            typedef struct outt {
                int a;
                int b;
            } outt_t;

            // in the task body
            // assume a and b are initialized to 0
            for(;;){
                if (i%2 == 0){
                    inn->a++;
                    out->a = inn->a;
                } else {
                    inn->b++;
                    out->b = inn->b;
                }
                inn->counter++;
                osDelay(1000);
            }
            ```

            In this example at iteration $x=2$ output structure holds
            `out->a = 1, out->b = 1`. In case of failure on the next
            step the output is wiped, and a re-execution would result in
            `out->a = 2, out->b = RANDOM_ERROR` which is different from
            the correct result `out->a = 2, out->b = 1`, hence breaking
            recovery process. A solution to this problem would be this:

            ``` {.objectivec language="C"}
            // in the task body
            // assume a and b are initialized to 0
            for(;;){
                if (inn->counter%2 == 0){
                    inn->a++;
                } else {
                    inn->b++;
                }
                //always update the output at every iteration
                out->a = inn->a;
                out->b = inn->b;
                
                inn->counter++;
                osDelay(1000);
            }
            ```